/* -------------------------------------------------------------------------
 *
 * tsgistidx.c
 *	  GiST support functions for tsvector_ops
 *
 * Portions Copyright (c) 1996-2012, PostgreSQL Global Development Group
 *
 *
 * IDENTIFICATION
 *	  src/backend/utils/adt/tsgistidx.c
 *
 * -------------------------------------------------------------------------
 */

#include "postgres.h"
#include "knl/knl_variable.h"
#include "port/pg_bitutils.h"

#include "access/gist.h"
#include "access/tuptoaster.h"
#include "tsearch/ts_utils.h"

#define SIGLENINT                                     \
    31 /* >121 => key will toast, so it will not work \
        * !!! */

#define SIGLEN (sizeof(int4) * SIGLENINT)
#define SIGLENBIT (SIGLEN * BITS_PER_BYTE)

typedef char BITVEC[SIGLEN];
typedef char* BITVECP;

#define LOOPBYTE for (i = 0; i < SIGLEN; i++)

#define GETBYTE(x, i) (*((BITVECP)(x) + (int)((i) / BITS_PER_BYTE)))
#define GETBITBYTE(x, i) (((char)(x)) >> (i)&0x01)
#define CLRBIT(x, i) GETBYTE(x, i) &= ~(0x01 << ((i) % BITS_PER_BYTE))
#define SETBIT(x, i) GETBYTE(x, i) |= (0x01 << ((i) % BITS_PER_BYTE))
#define GETBIT(x, i) ((GETBYTE(x, i) >> ((i) % BITS_PER_BYTE)) & 0x01)

#define HASHVAL(val) (((unsigned int)(val)) % SIGLENBIT)
#define HASH(sign, val) SETBIT((sign), HASHVAL(val))

#define GETENTRY(vec, pos) ((SignTSVector*)DatumGetPointer((vec)->vector[(pos)].key))

/*
 * type of GiST index key
 */

typedef struct {
    int32 vl_len_; /* varlena header (do not touch directly!) */
    int4 flag;
    char data[1];
} SignTSVector;

#define ARRKEY 0x01
#define SIGNKEY 0x02
#define ALLISTRUE 0x04

#define ISARRKEY(x) (((SignTSVector*)(x))->flag & ARRKEY)
#define ISSIGNKEY(x) (((SignTSVector*)(x))->flag & SIGNKEY)
#define ISALLTRUE(x) (((SignTSVector*)(x))->flag & ALLISTRUE)

#define GTHDRSIZE (VARHDRSZ + sizeof(int4))
#define CALCGTSIZE(flag, len) \
    (GTHDRSIZE + (((flag)&ARRKEY) ? ((len) * sizeof(int4)) : (((flag)&ALLISTRUE) ? 0 : SIGLEN)))

#define GETSIGN(x) ((BITVECP)((char*)(x) + GTHDRSIZE))
#define GETARR(x) ((int4*)((char*)(x) + GTHDRSIZE))
#define ARRNELEM(x) ((VARSIZE(x) - GTHDRSIZE) / sizeof(int4))


static int4 sizebitvec(BITVECP sign);

Datum gtsvectorin(PG_FUNCTION_ARGS)
{
    ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED), errmsg("gtsvector_in not implemented")));
    PG_RETURN_DATUM(0);
}

#define SINGOUTSTR "%d true bits, %d false bits"
#define ARROUTSTR "%d unique words"
#define EXTRALEN (2 * 13)

Datum gtsvectorout(PG_FUNCTION_ARGS)
{
    SignTSVector* key = (SignTSVector*)DatumGetPointer(PG_DETOAST_DATUM(PG_GETARG_POINTER(0)));
    uint4 outbuf_maxlen = 2 * EXTRALEN + Max(strlen(SINGOUTSTR), strlen(ARROUTSTR)) + 1;
    char* outbuf = NULL;
    errno_t rc;

    outbuf = (char*)palloc(outbuf_maxlen);

    if (ISARRKEY(key))
        rc = sprintf_s(outbuf, outbuf_maxlen, ARROUTSTR, (int)ARRNELEM(key));
    else {
        int cnttrue = (ISALLTRUE(key)) ? SIGLENBIT : sizebitvec(GETSIGN(key));

        rc = sprintf_s(outbuf, outbuf_maxlen, SINGOUTSTR, cnttrue, (int)SIGLENBIT - cnttrue);
    }
    securec_check(rc, "\0", "\0");

    PG_FREE_IF_COPY(key, 0);
    PG_RETURN_POINTER(outbuf);
}

static int compareint(const void* va, const void* vb)
{
    int4 a = *((const int4*)va);
    int4 b = *((const int4*)vb);

    if (a == b)
        return 0;
    return (a > b) ? 1 : -1;
}

/*
 * Removes duplicates from an array of int4. 'l' is
 * size of the input array. Returns the new size of the array.
 */
static int uniqueint(int4* a, int4 l)
{
    int4* ptr = NULL;
    int4* res = NULL;

    if (l <= 1)
        return l;

    ptr = res = a;

    qsort((void*)a, l, sizeof(int4), compareint);

    while (ptr - a < l)
        if (*ptr != *res)
            *(++res) = *ptr++;
        else
            ptr++;
    return res + 1 - a;
}

static void makesign(BITVECP sign, SignTSVector* a)
{
    int4 k, len = ARRNELEM(a);
    int4* ptr = GETARR(a);
    errno_t rc = 0;

    rc = memset_s((void*)sign, sizeof(BITVEC), 0, sizeof(BITVEC));
    securec_check(rc, "\0", "\0");

    for (k = 0; k < len; k++)
        HASH(sign, ptr[k]);
}

Datum gtsvector_compress(PG_FUNCTION_ARGS)
{
    GISTENTRY* entry = (GISTENTRY*)PG_GETARG_POINTER(0);
    GISTENTRY* retval = entry;

    if (entry->leafkey) { /* tsvector */
        SignTSVector* res = NULL;
        TSVector val = DatumGetTSVector(entry->key);
        int4 len;
        int4* arr = NULL;
        WordEntry* ptr = ARRPTR(val);
        char* words = STRPTR(val);

        len = CALCGTSIZE(ARRKEY, val->size);
        res = (SignTSVector*)palloc(len);
        SET_VARSIZE(res, len);
        res->flag = ARRKEY;
        arr = GETARR(res);
        len = val->size;
        while (len--) {
            pg_crc32 c;

            INIT_CRC32(c);
            COMP_CRC32(c, words + ptr->pos, ptr->len);
            FIN_CRC32(c);

            *arr = *(int4*)&c;
            arr++;
            ptr++;
        }

        len = uniqueint(GETARR(res), val->size);
        if (len != val->size) {
            /*
             * there is a collision of hash-function; len is always less than
             * val->size
             */
            len = CALCGTSIZE(ARRKEY, len);
            res = (SignTSVector*)repalloc((void*)res, len);
            SET_VARSIZE(res, len);
        }

        /* make signature, if array is too long */
        if (VARSIZE(res) > TOAST_INDEX_TARGET) {
            SignTSVector* ressign = NULL;

            len = CALCGTSIZE(SIGNKEY, 0);
            ressign = (SignTSVector*)palloc(len);
            SET_VARSIZE(ressign, len);
            ressign->flag = SIGNKEY;
            makesign(GETSIGN(ressign), res);
            /* free the unused buffer and then assign a new */
            pfree_ext(res);
            res = ressign;
        }

        retval = (GISTENTRY*)palloc(sizeof(GISTENTRY));
        gistentryinit(*retval, PointerGetDatum(res), entry->rel, entry->page, entry->offset, FALSE);
    } else if (ISSIGNKEY(DatumGetPointer(entry->key)) && !ISALLTRUE(DatumGetPointer(entry->key))) {
        uint32 i, len;
        SignTSVector* res = NULL;
        BITVECP sign = GETSIGN(DatumGetPointer(entry->key));

        LOOPBYTE
        {
            if ((sign[i] & 0xff) != 0xff)
                PG_RETURN_POINTER(retval);
        }

        len = CALCGTSIZE(SIGNKEY | ALLISTRUE, 0);
        res = (SignTSVector*)palloc(len);
        SET_VARSIZE(res, len);
        res->flag = SIGNKEY | ALLISTRUE;

        retval = (GISTENTRY*)palloc(sizeof(GISTENTRY));
        gistentryinit(*retval, PointerGetDatum(res), entry->rel, entry->page, entry->offset, FALSE);
    }
    PG_RETURN_POINTER(retval);
}

Datum gtsvector_decompress(PG_FUNCTION_ARGS)
{
    GISTENTRY* entry = (GISTENTRY*)PG_GETARG_POINTER(0);
    SignTSVector* key = (SignTSVector*)DatumGetPointer(PG_DETOAST_DATUM(entry->key));

    if (key != (SignTSVector*)DatumGetPointer(entry->key)) {
        GISTENTRY* retval = (GISTENTRY*)palloc(sizeof(GISTENTRY));

        gistentryinit(*retval, PointerGetDatum(key), entry->rel, entry->page, entry->offset, FALSE);

        PG_RETURN_POINTER(retval);
    }

    PG_RETURN_POINTER(entry);
}

typedef struct {
    int4* arrb;
    int4* arre;
} CHKVAL;

/*
 * is there value 'val' in array or not ?
 */
static bool checkcondition_arr(const void* checkval, QueryOperand* val)
{
    int4* StopLow = ((CHKVAL*)checkval)->arrb;
    int4* StopHigh = ((CHKVAL*)checkval)->arre;
    int4* StopMiddle = NULL;

    /* Loop invariant: StopLow <= val < StopHigh */

    /*
     * we are not able to find a a prefix by hash value
     */
    if (val->prefix)
        return true;

    while (StopLow < StopHigh) {
        StopMiddle = StopLow + (StopHigh - StopLow) / 2;
        if (*StopMiddle == val->valcrc)
            return (true);
        else if (*StopMiddle < val->valcrc)
            StopLow = StopMiddle + 1;
        else
            StopHigh = StopMiddle;
    }

    return (false);
}

static bool checkcondition_bit(const void* checkval, QueryOperand* val)
{
    /*
     * we are not able to find a a prefix in signature tree
     */
    if (val->prefix)
        return true;
    return GETBIT(checkval, (unsigned int)HASHVAL(val->valcrc));
}

Datum gtsvector_consistent(PG_FUNCTION_ARGS)
{
    GISTENTRY* entry = (GISTENTRY*)PG_GETARG_POINTER(0);
    TSQuery query = PG_GETARG_TSQUERY(1);

    bool* recheck = (bool*)PG_GETARG_POINTER(4);
    SignTSVector* key = (SignTSVector*)DatumGetPointer(entry->key);

    /* All cases served by this function are inexact */
    *recheck = true;

    if (!query->size)
        PG_RETURN_BOOL(false);

    if (ISSIGNKEY(key)) {
        if (ISALLTRUE(key))
            PG_RETURN_BOOL(true);

        PG_RETURN_BOOL(TS_execute(GETQUERY(query), (void*)GETSIGN(key), false, checkcondition_bit));
    } else { /* only leaf pages */
        CHKVAL chkval;

        chkval.arrb = GETARR(key);
        chkval.arre = chkval.arrb + ARRNELEM(key);
        PG_RETURN_BOOL(TS_execute(GETQUERY(query), (void*)&chkval, true, checkcondition_arr));
    }
}

static int4 unionkey(BITVECP sbase, SignTSVector* add)
{
    uint32 i;

    if (ISSIGNKEY(add)) {
        BITVECP sadd = GETSIGN(add);

        if (ISALLTRUE(add))
            return 1;

        LOOPBYTE
        sbase[i] |= sadd[i];
    } else {
        int4* ptr = GETARR(add);

        for (i = 0; i < ARRNELEM(add); i++)
            HASH(sbase, ptr[i]);
    }
    return 0;
}

Datum gtsvector_union(PG_FUNCTION_ARGS)
{
    GistEntryVector* entryvec = (GistEntryVector*)PG_GETARG_POINTER(0);
    int* size = (int*)PG_GETARG_POINTER(1);
    BITVEC base;
    int4 i, len;
    int4 flag = 0;
    SignTSVector* result = NULL;
    errno_t rc;

    rc = memset_s((void*)base, sizeof(BITVEC), 0, sizeof(BITVEC));
    securec_check(rc, "\0", "\0");
    for (i = 0; i < entryvec->n; i++) {
        if (unionkey(base, GETENTRY(entryvec, i))) {
            flag = ALLISTRUE;
            break;
        }
    }

    flag |= SIGNKEY;
    len = CALCGTSIZE(flag, 0);
    result = (SignTSVector*)palloc(len);
    *size = len;
    SET_VARSIZE(result, len);
    result->flag = flag;
    if (!ISALLTRUE(result)) {
        rc = memcpy_s((void*)GETSIGN(result), sizeof(BITVEC), (void*)base, sizeof(BITVEC));
        securec_check(rc, "\0", "\0");
    }
    PG_RETURN_POINTER(result);
}

Datum gtsvector_same(PG_FUNCTION_ARGS)
{
    SignTSVector* a = (SignTSVector*)PG_GETARG_POINTER(0);
    SignTSVector* b = (SignTSVector*)PG_GETARG_POINTER(1);
    bool* result = (bool*)PG_GETARG_POINTER(2);

    if (ISSIGNKEY(a)) { /* then b also ISSIGNKEY */
        if (ISALLTRUE(a) && ISALLTRUE(b))
            *result = true;
        else if (ISALLTRUE(a))
            *result = false;
        else if (ISALLTRUE(b))
            *result = false;
        else {
            uint32 i;
            BITVECP sa = GETSIGN(a), sb = GETSIGN(b);

            *result = true;
            LOOPBYTE
            {
                if (sa[i] != sb[i]) {
                    *result = false;
                    break;
                }
            }
        }
    } else { /* a and b ISARRKEY */
        int4 lena = ARRNELEM(a), lenb = ARRNELEM(b);

        if (lena != lenb)
            *result = false;
        else {
            int4 *ptra = GETARR(a), *ptrb = GETARR(b);
            int4 i;

            *result = true;
            for (i = 0; i < lena; i++)
                if (ptra[i] != ptrb[i]) {
                    *result = false;
                    break;
                }
        }
    }

    PG_RETURN_POINTER(result);
}

static int4 sizebitvec(BITVECP sign)
{
    return pg_popcount(sign, SIGLEN);
}

static int hemdistsign(BITVECP a, BITVECP b)
{
    uint32 i, diff;
    int dist = 0;

    LOOPBYTE
    {
        diff = (unsigned char)((unsigned char)a[i] ^ (unsigned char)b[i]);
        /* Using the popcount functions here isn't likely to win */
        dist += pg_number_of_ones[diff];
    }
    return dist;
}

static int hemdist(SignTSVector* a, SignTSVector* b)
{
    if (ISALLTRUE(a)) {
        if (ISALLTRUE(b))
            return 0;
        else
            return SIGLENBIT - sizebitvec(GETSIGN(b));
    } else if (ISALLTRUE(b))
        return SIGLENBIT - sizebitvec(GETSIGN(a));

    return hemdistsign(GETSIGN(a), GETSIGN(b));
}

Datum gtsvector_penalty(PG_FUNCTION_ARGS)
{
    GISTENTRY* origentry = (GISTENTRY*)PG_GETARG_POINTER(0); /* always ISSIGNKEY */
    GISTENTRY* newentry = (GISTENTRY*)PG_GETARG_POINTER(1);
    float* penalty = (float*)PG_GETARG_POINTER(2);
    SignTSVector* origval = (SignTSVector*)DatumGetPointer(origentry->key);
    SignTSVector* newval = (SignTSVector*)DatumGetPointer(newentry->key);
    BITVECP orig = GETSIGN(origval);

    *penalty = 0.0;

    if (ISARRKEY(newval)) {
        BITVEC sign;

        makesign(sign, newval);

        if (ISALLTRUE(origval))
            *penalty = ((float)(SIGLENBIT - sizebitvec(sign))) / (float)(SIGLENBIT + 1);
        else
            *penalty = hemdistsign(sign, orig);
    } else
        *penalty = hemdist(origval, newval);
    PG_RETURN_POINTER(penalty);
}

typedef struct {
    bool allistrue;
    BITVEC sign;
} CACHESIGN;

static void fillcache(CACHESIGN* item, SignTSVector* key)
{
    item->allistrue = false;
    if (ISARRKEY(key))
        makesign(item->sign, key);
    else if (ISALLTRUE(key))
        item->allistrue = true;
    else {
        errno_t rc = EOK;
        rc = memcpy_s((void*)item->sign, sizeof(BITVEC), (void*)GETSIGN(key), sizeof(BITVEC));
        securec_check(rc, "\0", "\0");
    }
}

#define WISH_F(a, b, c) (double)(-(double)(((a) - (b)) * ((a) - (b)) * ((a) - (b))) * (c))
typedef struct {
    OffsetNumber pos;
    int4 cost;
} SPLITCOST;

static int comparecost(const void* va, const void* vb)
{
    const SPLITCOST* a = (const SPLITCOST*)va;
    const SPLITCOST* b = (const SPLITCOST*)vb;

    if (a->cost == b->cost)
        return 0;
    else
        return (a->cost > b->cost) ? 1 : -1;
}

static int hemdistcache(CACHESIGN* a, CACHESIGN* b)
{
    if (a->allistrue) {
        if (b->allistrue)
            return 0;
        else
            return SIGLENBIT - sizebitvec(b->sign);
    } else if (b->allistrue)
        return SIGLENBIT - sizebitvec(a->sign);

    return hemdistsign(a->sign, b->sign);
}

Datum gtsvector_picksplit(PG_FUNCTION_ARGS)
{
    GistEntryVector* entryvec = (GistEntryVector*)PG_GETARG_POINTER(0);
    GIST_SPLITVEC* v = (GIST_SPLITVEC*)PG_GETARG_POINTER(1);
    OffsetNumber k, j;
    SignTSVector* datum_l = NULL;
    SignTSVector* datum_r = NULL;
    BITVECP union_l = NULL;
    BITVECP union_r = NULL;
    int4 size_alpha, size_beta;
    int4 size_waste, waste = -1;
    int4 nbytes;
    OffsetNumber seed_1 = 0, seed_2 = 0;
    OffsetNumber* left = NULL;
    OffsetNumber* right = NULL;
    OffsetNumber maxoff;
    BITVECP ptr = NULL;
    uint32 i;
    CACHESIGN* cache = NULL;
    SPLITCOST* costvector = NULL;
    errno_t rc = EOK;

    maxoff = entryvec->n - 2;
    nbytes = (maxoff + 2) * sizeof(OffsetNumber);
    v->spl_left = (OffsetNumber*)palloc(nbytes);
    v->spl_right = (OffsetNumber*)palloc(nbytes);

    cache = (CACHESIGN*)palloc(sizeof(CACHESIGN) * (maxoff + 2));
    fillcache(&cache[FirstOffsetNumber], GETENTRY(entryvec, FirstOffsetNumber));

    for (k = FirstOffsetNumber; k < maxoff; k = OffsetNumberNext(k)) {
        for (j = OffsetNumberNext(k); j <= maxoff; j = OffsetNumberNext(j)) {
            if (k == FirstOffsetNumber)
                fillcache(&cache[j], GETENTRY(entryvec, j));

            size_waste = hemdistcache(&(cache[j]), &(cache[k]));
            if (size_waste > waste) {
                waste = size_waste;
                seed_1 = k;
                seed_2 = j;
            }
        }
    }

    left = v->spl_left;
    v->spl_nleft = 0;
    right = v->spl_right;
    v->spl_nright = 0;

    if (seed_1 == 0 || seed_2 == 0) {
        seed_1 = 1;
        seed_2 = 2;
    }

    /* form initial .. */
    if (cache[seed_1].allistrue) {
        datum_l = (SignTSVector*)palloc(CALCGTSIZE(SIGNKEY | ALLISTRUE, 0));
        SET_VARSIZE(datum_l, CALCGTSIZE(SIGNKEY | ALLISTRUE, 0));
        datum_l->flag = SIGNKEY | ALLISTRUE;
    } else {
        datum_l = (SignTSVector*)palloc(CALCGTSIZE(SIGNKEY, 0));
        SET_VARSIZE(datum_l, CALCGTSIZE(SIGNKEY, 0));
        datum_l->flag = SIGNKEY;
        rc = memcpy_s((void*)GETSIGN(datum_l), sizeof(BITVEC), (void*)cache[seed_1].sign, sizeof(BITVEC));
        securec_check(rc, "\0", "\0");
    }
    if (cache[seed_2].allistrue) {
        datum_r = (SignTSVector*)palloc(CALCGTSIZE(SIGNKEY | ALLISTRUE, 0));
        SET_VARSIZE(datum_r, CALCGTSIZE(SIGNKEY | ALLISTRUE, 0));
        datum_r->flag = SIGNKEY | ALLISTRUE;
    } else {
        datum_r = (SignTSVector*)palloc(CALCGTSIZE(SIGNKEY, 0));
        SET_VARSIZE(datum_r, CALCGTSIZE(SIGNKEY, 0));
        datum_r->flag = SIGNKEY;
        rc = memcpy_s((void*)GETSIGN(datum_r), sizeof(BITVEC), (void*)cache[seed_2].sign, sizeof(BITVEC));
        securec_check(rc, "\0", "\0");
    }

    union_l = GETSIGN(datum_l);
    union_r = GETSIGN(datum_r);
    maxoff = OffsetNumberNext(maxoff);
    fillcache(&cache[maxoff], GETENTRY(entryvec, maxoff));
    /* sort before ... */
    costvector = (SPLITCOST*)palloc(sizeof(SPLITCOST) * maxoff);
    for (j = FirstOffsetNumber; j <= maxoff; j = OffsetNumberNext(j)) {
        costvector[j - 1].pos = j;
        size_alpha = hemdistcache(&(cache[seed_1]), &(cache[j]));
        size_beta = hemdistcache(&(cache[seed_2]), &(cache[j]));
        costvector[j - 1].cost = Abs(size_alpha - size_beta);
    }
    qsort((void*)costvector, maxoff, sizeof(SPLITCOST), comparecost);

    for (k = 0; k < maxoff; k++) {
        j = costvector[k].pos;
        if (j == seed_1) {
            *left++ = j;
            v->spl_nleft++;
            continue;
        } else if (j == seed_2) {
            *right++ = j;
            v->spl_nright++;
            continue;
        }

        if (ISALLTRUE(datum_l) || cache[j].allistrue) {
            if (ISALLTRUE(datum_l) && cache[j].allistrue)
                size_alpha = 0;
            else
                size_alpha = SIGLENBIT - sizebitvec((cache[j].allistrue) ? GETSIGN(datum_l) : GETSIGN(cache[j].sign));
        } else
            size_alpha = hemdistsign(cache[j].sign, GETSIGN(datum_l));

        if (ISALLTRUE(datum_r) || cache[j].allistrue) {
            if (ISALLTRUE(datum_r) && cache[j].allistrue)
                size_beta = 0;
            else
                size_beta = SIGLENBIT - sizebitvec((cache[j].allistrue) ? GETSIGN(datum_r) : GETSIGN(cache[j].sign));
        } else
            size_beta = hemdistsign(cache[j].sign, GETSIGN(datum_r));

        if (size_alpha < size_beta + WISH_F(v->spl_nleft, v->spl_nright, 0.1)) {
            if (ISALLTRUE(datum_l) || cache[j].allistrue) {
                if (!ISALLTRUE(datum_l)) {
                    rc = memset_s((void*)GETSIGN(datum_l), sizeof(BITVEC), 0xff, sizeof(BITVEC));
                    securec_check(rc, "\0", "\0");
                }
            } else {
                ptr = cache[j].sign;
                LOOPBYTE
                union_l[i] |= ptr[i];
            }
            *left++ = j;
            v->spl_nleft++;
        } else {
            if (ISALLTRUE(datum_r) || cache[j].allistrue) {
                if (!ISALLTRUE(datum_r)) {
                    rc = memset_s((void*)GETSIGN(datum_r), sizeof(BITVEC), 0xff, sizeof(BITVEC));
                    securec_check(rc, "\0", "\0");
                }
            } else {
                ptr = cache[j].sign;
                LOOPBYTE
                union_r[i] |= ptr[i];
            }
            *right++ = j;
            v->spl_nright++;
        }
    }

    *right = *left = FirstOffsetNumber;
    v->spl_ldatum = PointerGetDatum(datum_l);
    v->spl_rdatum = PointerGetDatum(datum_r);

    PG_RETURN_POINTER(v);
}
