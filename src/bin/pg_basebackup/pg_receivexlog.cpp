/* -------------------------------------------------------------------------
 *
 * pg_receivexlog.c - receive streaming transaction log data and write it
 *					  to a local file.
 *
 * Author: Magnus Hagander <magnus@hagander.net>
 *
 * Portions Copyright (c) 1996-2012, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *		  src/bin/pg_basebackup/pg_receivexlog.c
 * -------------------------------------------------------------------------
 */

/*
 * We have to use postgres.h not postgres_fe.h here, because there's so much
 * backend-only stuff in the XLOG include files we need.  But we need a
 * frontend-ish environment otherwise.	Hence this ugly hack.
 */
#define FRONTEND 1
#include "postgres.h"
#include "knl/knl_variable.h"
#include "libpq/libpq-fe.h"
#include "libpq/pqsignal.h"
#include "access/xlog_internal.h"

#include "receivelog.h"
#include "streamutil.h"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "getopt_long.h"
#include "bin/elog.h"

/* Time to sleep between reconnection attempts */
#define RECONNECT_SLEEP_TIME 5

/* Global options */
char* basedir = NULL;
int verbose = 0;
int noloop = 0;
volatile bool time_to_abort = false;

extern int standby_message_timeout; /* 10 sec = default */

static void usage(void);
static XLogRecPtr FindStreamingStart(XLogRecPtr currentpos, uint32 currenttimeline);
static void StreamLog();
static bool stop_streaming(XLogRecPtr segendpos, uint32 timeline, bool segment_finished);
static void free_receivexlog();

static void usage(void)
{
    printf(_("%s receives openGauss streaming transaction logs.\n\n"), progname);
    printf(_("Usage:\n"));
    printf(_("  %s [OPTION]...\n"), progname);
    printf(_("\nOptions:\n"));
    printf(_("  -D, --directory=DIR    receive transaction log files into this directory\n"));
    printf(_("  -n, --no-loop          do not loop on connection lost\n"));
    printf(_("  -v, --verbose          output verbose messages\n"));
    printf(_("  -V, --version          output version information, then exit\n"));
    printf(_("  -?, --help             show this help, then exit\n"));
    printf(_("\nConnection options:\n"));
    printf(_("  -h, --host=HOSTNAME    database server host or socket directory\n"));
    printf(_("  -p, --port=PORT        database server port number\n"));
    printf(_("  -s, --status-interval=INTERVAL\n"
             "                         time between status packets sent to server (in seconds)\n"));
    printf(_("  -U, --username=NAME    connect as specified database user\n"));
    printf(_("  -w, --no-password      never prompt for password\n"));
    printf(_("  -W, --password         force password prompt (should happen automatically)\n"));
    printf(_("      --slot             replication slot to use\n"));
#if ((defined(ENABLE_MULTIPLE_NODES)) || (defined(ENABLE_PRIVATEGAUSS)))
    printf(_("\nReport bugs to GaussDB support.\n"));
#else
    printf(_("\nReport bugs to community@opengauss.org> or join opengauss community <https://opengauss.org>.\n"));
#endif
}

static bool stop_streaming(XLogRecPtr segendpos, uint32 timeline, bool segment_finished)
{
    if (verbose && segment_finished)
        fprintf(stderr,
            _("%s: finished segment at %X/%X (timeline %u)\n"),
            progname,
            (uint32)(segendpos >> 32),
            (uint32)segendpos,
            timeline);

    if (time_to_abort) {
        fprintf(stderr, _("%s: received interrupt signal, exiting.\n"), progname);
        return true;
    }
    return false;
}

/*
 * Determine starting location for streaming, based on:
 * 1. If there are existing xlog segments, start at the end of the last one
 *	  that is complete (size matches XLogSegSize)
 * 2. If no valid xlog exists, start from the beginning of the current
 *	  WAL segment.
 */
static XLogRecPtr FindStreamingStart(XLogRecPtr currentpos, uint32 currenttimeline)
{
    DIR* dir = NULL;
    struct dirent* dirent = NULL;
    int i = 0;
    bool b = FALSE;
    XLogSegNo high_segno = 0;
    errno_t errorno = EOK;

    dir = opendir(basedir);
    if (dir == NULL) {
        fprintf(stderr, _("%s: could not open directory \"%s\": %s\n"), progname, basedir, strerror(errno));
        disconnect_and_exit(1);
    }

    while ((dirent = readdir(dir)) != NULL) {
        char fullpath[MAXPGPATH];
        struct stat statbuf;
        uint32 tli;
        uint32 log, seg;
        XLogSegNo segno;

        if (strcmp(dirent->d_name, ".") == 0 || strcmp(dirent->d_name, "..") == 0)
            continue;

        /* xlog files are always 24 characters */
        if (strlen(dirent->d_name) != 24)
            continue;

        /* Filenames are always made out of 0-9 and A-F */
        b = false;
        for (i = 0; i < 24; i++) {
            if (!(dirent->d_name[i] >= '0' && dirent->d_name[i] <= '9') &&
                !(dirent->d_name[i] >= 'A' && dirent->d_name[i] <= 'F')) {
                b = true;
                break;
            }
        }
        if (b)
            continue;

        /*
         * Looks like an xlog file. Parse its position.
         */
        if (sscanf_s(dirent->d_name, "%08X%08X%08X", &tli, &log, &seg) != 3) {
            fprintf(stderr, _("%s: could not parse transaction log file name \"%s\"\n"), progname, dirent->d_name);
            closedir(dir);
            disconnect_and_exit(1);
        }
        segno = ((uint64)log) << 32 | seg;

        /* Ignore any files that are for another timeline */
        if (tli != currenttimeline)
            continue;

        /* Check if this is a completed segment or not */
        errorno = snprintf_s(fullpath, sizeof(fullpath), sizeof(fullpath) - 1, "%s/%s", basedir, dirent->d_name);
        securec_check_ss_c(errorno, "", "");
        if (stat(fullpath, &statbuf) != 0) {
            fprintf(stderr, _("%s: could not stat file \"%s\": %s\n"), progname, fullpath, strerror(errno));
            closedir(dir);
            disconnect_and_exit(1);
        }

        if (statbuf.st_size == (off_t)XLogSegSize) {
            /* Completed segment */
            if (segno > high_segno) {
                high_segno = segno;
                continue;
            }
        } else {
            fprintf(stderr,
                _("%s: segment file \"%s\" has incorrect size %d, skipping\n"),
                progname,
                dirent->d_name,
                (int)statbuf.st_size);
            continue;
        }
    }

    closedir(dir);

    if (high_segno > 0) {
        XLogRecPtr high_ptr;

        /*
         * Move the starting pointer to the start of the next segment, since
         * the highest one we've seen was completed.
         */
        high_segno++;

        XLogSegNoOffsetToRecPtr(high_segno, 0, high_ptr);

        return high_ptr;
    } else
        return currentpos;
}

/*
 * Start the log streaming
 */
static void StreamLog(void)
{
    PGresult* res = NULL;
    uint32 timeline = 0;
    XLogRecPtr startpos;
    int minServerMajor = 0;
    int maxServerMajor = 0;
    int serverMajor = 0;
    uint32 hi, lo;

    /*
     * Connect in replication mode to the server
     */
    conn = GetConnection();
    if (conn == NULL)
        /* Error message already written in GetConnection() */
        return;

    ClearAndFreePasswd();
    /*
     * Check server version. IDENTIFY_SYSTEM didn't return the current xlog
     * position before 9.1, so we can't work with servers older than 9.1. And
     * we don't support servers newer than the client.
     */
    minServerMajor = 901;
    maxServerMajor = PG_VERSION_NUM / 100;
    serverMajor = PQserverVersion(conn) / 100;
    if (serverMajor < minServerMajor || serverMajor > maxServerMajor) {
        fprintf(stderr, _("%s: unsupported server version %s\n"), progname, PQparameterStatus(conn, "server_version"));
        disconnect_and_exit(1);
    }

    /*
     * Run IDENTIFY_SYSTEM so we can get the timeline and current xlog
     * position.
     */
    res = PQexec(conn, "IDENTIFY_SYSTEM");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr,
            _("%s: could not send replication command \"%s\": %s"),
            progname,
            "IDENTIFY_SYSTEM",
            PQerrorMessage(conn));

        disconnect_and_exit(1);
    }
    if (PQntuples(res) != 1 || PQnfields(res) < 3) {
        fprintf(stderr,
            _("%s: could not identify system: got %d rows and %d fields, expected %d rows and %d or more fields\n"),
            progname,
            PQntuples(res),
            PQnfields(res),
            1,
            3);

        disconnect_and_exit(1);
    }
    timeline = (uint32)atoi(PQgetvalue(res, 0, 1));
    if (sscanf_s(PQgetvalue(res, 0, 2), "%X/%X", &hi, &lo) != 2) {
        fprintf(stderr, _("%s: could not parse transaction log location \"%s\"\n"), progname, PQgetvalue(res, 0, 2));
        disconnect_and_exit(1);
    }
    startpos = ((uint64)hi) << 32 | lo;
    PQclear(res);

    /*
     * Figure out where to start streaming.
     */
    startpos = FindStreamingStart(startpos, timeline);

    /*
     * Always start streaming at the beginning of a segment
     */
    startpos -= startpos % XLogSegSize;

    /*
     * Start the replication
     */
    if (verbose)
        fprintf(stderr,
            _("%s: starting log streaming at %X/%X (timeline %u)\n"),
            progname,
            (uint32)(startpos >> 32),
            (uint32)startpos,
            timeline);

    if (!ReceiveXlogStream(conn,
        startpos,
        timeline,
        (const char*)NULL,
        (const char*)basedir,
        stop_streaming,
        standby_message_timeout,
        false)) {
        fprintf(stderr, _("%s: ReceiveXlogStream failed.\n"), progname);
        disconnect_and_exit(1);
    }

    PQfinish(conn);
    conn = NULL;
}

/*
 * When sigint is called, just tell the system to exit at the next possible
 * moment.
 */
#ifndef WIN32

static void sigint_handler(int signum)
{
    time_to_abort = true;
}
#endif

int main(int argc, char** argv)
{
    static struct option long_options[] = {{"help", no_argument, NULL, '?'},
        {"version", no_argument, NULL, 'V'},
        {"directory", required_argument, NULL, 'D'},
        {"host", required_argument, NULL, 'h'},
        {"port", required_argument, NULL, 'p'},
        {"username", required_argument, NULL, 'U'},
        {"no-loop", no_argument, NULL, 'n'},
        {"no-password", no_argument, NULL, 'w'},
        {"password", no_argument, NULL, 'W'},
        {"status-interval", required_argument, NULL, 's'},
        {"slot", required_argument, NULL, 'S'},
        {"verbose", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0}};
    int c;
    int option_index;

    progname = get_progname(argv[0]);
    set_pglocale_pgservice(argv[0], PG_TEXTDOMAIN("pg_receivexlog"));

    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-?") == 0) {
            usage();
            exit(0);
        } else if (strcmp(argv[1], "-V") == 0 || strcmp(argv[1], "--version") == 0) {
            puts("pg_receivexlog " DEF_GS_VERSION);
            exit(0);
        }
    }

    while ((c = getopt_long(argc, argv, "D:h:p:U:s:nwWv", long_options, &option_index)) != -1) {
        switch (c) {
            case 'D': {
                GS_FREE(basedir);
                check_env_value_c(optarg);
                char realDir[PATH_MAX] = {0};
                if (realpath(optarg, realDir) == nullptr) {
                    fprintf(stderr, _("%s: realpath dir \"%s\" failed: %m\n"), progname, optarg);
                    exit(1);
                }
                basedir = xstrdup(realDir);
                break;
            }
            case 'h':
                GS_FREE(dbhost);
                check_env_value_c(optarg);
                dbhost = xstrdup(optarg);
                break;
            case 'p':
                if (atoi(optarg) <= 0) {
                    fprintf(stderr, _("%s: invalid port number \"%s\"\n"), progname, optarg);
                    exit(1);
                }
                GS_FREE(dbport);
                check_env_value_c(optarg);
                dbport = xstrdup(optarg);
                break;
            case 'U':
                GS_FREE(dbuser);
                check_env_name_c(optarg);
                dbuser = xstrdup(optarg);
                break;
            case 'w':
                dbgetpassword = -1;
                break;
            case 'W':
                dbgetpassword = 1;
                break;
            case 's':
                check_env_value_c(optarg);
                if ((atoi(optarg)) < 0 || (atoi(optarg) * 1000) > PG_INT32_MAX) {
                    fprintf(stderr, _("%s: invalid status interval \"%s\"\n"), progname, optarg);
                    exit(1);
                }
                standby_message_timeout = atoi(optarg);
                break;
            case 'S':
                GS_FREE(replication_slot);
                check_env_value_c(optarg);
                replication_slot = xstrdup(optarg);
                break;
            case 'n':
                noloop = 1;
                break;
            case 'v':
                verbose++;
                break;
            default:

                /*
                 * getopt_long already emitted a complaint
                 */
                fprintf(stderr, _("Try \"%s --help\" for more information.\n"), progname);
                exit(1);
                break;
        }
    }

    /*
     * Any non-option arguments?
     */
    if (optind < argc) {
        fprintf(stderr, _("%s: too many command-line arguments (first is \"%s\")\n"), progname, argv[optind]);
        fprintf(stderr, _("Try \"%s --help\" for more information.\n"), progname);
        exit(1);
    }

    /*
     * Required arguments
     */
    if (basedir == NULL) {
        fprintf(stderr, _("%s: no target directory specified\n"), progname);
        fprintf(stderr, _("Try \"%s --help\" for more information.\n"), progname);
        exit(1);
    }

#ifndef WIN32
    pqsignal(SIGINT, sigint_handler);
#endif

    while (true) {
        StreamLog();
        if (time_to_abort) {
            /*
             * We've been Ctrl-C'ed. That's not an error, so exit without an
             * errorcode.
             */
            exit(0);
        } else if (noloop) {
            fprintf(stderr, _("%s: disconnected.\n"), progname);
            exit(1);
        } else {
            fprintf(stderr,
                /* translator: check source for value for %d */
                _("%s: disconnected. Waiting %d seconds to try again.\n"),
                progname,
                RECONNECT_SLEEP_TIME);
            pg_usleep(RECONNECT_SLEEP_TIME * 1000000);
        }
    }

    free_receivexlog();
    /* Never get here */
    exit(2);
}

static void free_receivexlog()
{
    GS_FREE(basedir);
    GS_FREE(dbhost);
    GS_FREE(dbport);
    GS_FREE(dbuser);
    GS_FREE(replication_slot);
}
