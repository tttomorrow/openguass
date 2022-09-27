/* -------------------------------------------------------------------------
 *
 * view.cpp
 *	  use rewrite rules to construct views
 *
 * Portions Copyright (c) 2020 Huawei Technologies Co.,Ltd.
 * Portions Copyright (c) 1996-2012, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/gausskernel/optimizer/commands/view.cpp
 *
 * -------------------------------------------------------------------------
 */
#include "postgres.h"
#include "knl/knl_variable.h"

#include "access/heapam.h"
#include "access/xact.h"
#include "catalog/gs_matview.h"
#include "catalog/gs_matview_dependency.h"
#include "catalog/namespace.h"
#include "catalog/gs_column_keys.h"
#include "catalog/gs_encrypted_columns.h"
#include "client_logic/cache.h"
#include "commands/defrem.h"
#include "commands/tablecmds.h"
#include "commands/view.h"
#include "miscadmin.h"
#include "gs_policy/policy_common.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "parser/analyze.h"
#include "parser/parse_relation.h"
#include "rewrite/rewriteDefine.h"
#include "rewrite/rewriteManip.h"
#include "rewrite/rewriteSupport.h"
#include "optimizer/nodegroups.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/rel_gs.h"
#include "utils/syscache.h"
#ifdef PGXC
#include "pgxc/execRemote.h"
#include "tcop/utility.h"
#endif
static void checkViewTupleDesc(TupleDesc newdesc, TupleDesc olddesc);

InSideView query_from_view_hook = NULL;

static void setEncryptedColumnRef(ColumnDef *def, TargetEntry *tle)
{
    def->clientLogicColumnRef = (ClientLogicColumnRef*)palloc(sizeof(ClientLogicColumnRef));
    HeapTuple tup = SearchSysCache2(CERELIDCOUMNNAME, ObjectIdGetDatum(tle->resorigtbl), CStringGetDatum(def->colname));
    if (!HeapTupleIsValid(tup)) {
        ereport(ERROR,
                (errcode(ERRCODE_UNDEFINED_COLUMN), errmsg("client encrypted column \"%s\" does not exist", def->colname)));
    }
    Form_gs_encrypted_columns ce_form = (Form_gs_encrypted_columns) GETSTRUCT(tup);
    HeapTuple setting_tup  =  SearchSysCache1(COLUMNSETTINGOID, ObjectIdGetDatum(ce_form->column_key_id));
    if (!HeapTupleIsValid(setting_tup)) {
        ereport(ERROR, (errcode(ERRCODE_UNDEFINED_COLUMN),
                errmsg("client encrypted column key %u does not exist", ce_form->column_key_id)));
    }
    Form_gs_column_keys setting_form = (Form_gs_column_keys) GETSTRUCT(setting_tup);
    def->clientLogicColumnRef->column_key_name =  list_make1(makeString(NameStr(setting_form->column_key_name)));
    ReleaseSysCache(setting_tup);
    ReleaseSysCache(tup);
    def->clientLogicColumnRef->orig_typname =
            makeTypeNameFromOid(exprTypmod((Node*)tle->expr), -1);
    def->clientLogicColumnRef->dest_typname =
            makeTypeNameFromOid(exprType((Node*)tle->expr), exprTypmod((Node*)tle->expr));
}

/* ---------------------------------------------------------------------
 * DefineVirtualRelation
 *
 * Create the "view" relation. `DefineRelation' does all the work,
 * we just provide the correct arguments ... at least when we're
 * creating a view.  If we're updating an existing view, we have to
 * work harder.
 * ---------------------------------------------------------------------
 */
static Oid DefineVirtualRelation(RangeVar* relation, List* tlist, bool replace, List* options, ObjectType relkind)
{
    Oid viewOid;
    LOCKMODE lockmode;
    CreateStmt* createStmt = makeNode(CreateStmt);
    List* attrList = NIL;
    ListCell* t = NULL;

    /*
     * create a list of ColumnDef nodes based on the names and types of the
     * (non-junk) targetlist items from the view's SELECT list.
     */
    foreach (t, tlist) {
        TargetEntry* tle = (TargetEntry*)lfirst(t);

        if (!tle->resjunk) {
            ColumnDef* def = makeNode(ColumnDef);

            def->colname = pstrdup(tle->resname);
            def->typname = makeTypeNameFromOid(exprType((Node*)tle->expr), exprTypmod((Node*)tle->expr));
            def->inhcount = 0;
            def->is_local = true;
            def->is_not_null = false;
            def->is_from_type = false;
            def->storage = 0;
            def->cmprs_mode = ATT_CMPR_NOCOMPRESS; /* dont compress */
            def->raw_default = NULL;
            def->cooked_default = NULL;
            def->collClause = NULL;
            def->collOid = exprCollation((Node*)tle->expr);
            
            if IsClientLogicType(exprType((Node*)tle->expr)) {
                setEncryptedColumnRef(def, tle);
            }
            /*
             * It's possible that the column is of a collatable type but the
             * collation could not be resolved, so double-check.
             */
            if (type_is_collatable(exprType((Node*)tle->expr))) {
                if (!OidIsValid(def->collOid))
                    ereport(ERROR,
                        (errcode(ERRCODE_INDETERMINATE_COLLATION),
                            errmsg("could not determine which collation to use for view column \"%s\"", def->colname),
                            errhint("Use the COLLATE clause to set the collation explicitly.")));
            } else
                Assert(!OidIsValid(def->collOid));
            def->constraints = NIL;

            attrList = lappend(attrList, def);
        }
    }

    if (attrList == NIL)
        ereport(ERROR, (errcode(ERRCODE_INVALID_TABLE_DEFINITION), errmsg("view must have at least one column")));

    /*
     * Look up, check permissions on, and lock the creation namespace; also
     * check for a preexisting view with the same name.  This will also set
     * relation->relpersistence to RELPERSISTENCE_TEMP if the selected
     * namespace is temporary.
     */
    lockmode = replace ? AccessExclusiveLock : NoLock;
    (void)RangeVarGetAndCheckCreationNamespace(relation, lockmode, &viewOid, RELKIND_VIEW);
    
    bool flag = OidIsValid(viewOid) && replace;
    if (flag) {
        Relation rel;
        TupleDesc descriptor;
        List* atcmds = NIL;
        AlterTableCmd* atcmd = NULL;

        /*
         * During inplace upgrade, if we are doing rolling back, the old-versioned
         * relcache would cause problems. So update them anyway.
         */
        if (u_sess->attr.attr_common.IsInplaceUpgrade)
            RelationCacheInvalidateEntry(viewOid);

        /* Relation is already locked, but we must build a relcache entry. */
        rel = relation_open(viewOid, NoLock);
        /* Make sure it *is* a view or contquery. */
        flag = rel->rd_rel->relkind != RELKIND_VIEW && rel->rd_rel->relkind != RELKIND_CONTQUERY;
        if (flag)
            ereport(ERROR,
                (errcode(ERRCODE_WRONG_OBJECT_TYPE), errmsg("\"%s\" is not a view", RelationGetRelationName(rel))));

        /* Also check it's not in use already */
        CheckTableNotInUse(rel, "CREATE OR REPLACE VIEW");

        /*
         * Due to the namespace visibility rules for temporary objects, we
         * should only end up replacing a temporary view with another
         * temporary view, and similarly for permanent views.
         */
        Assert(relation->relpersistence == rel->rd_rel->relpersistence);

        /*
         * Create a tuple descriptor to compare against the existing view, and
         * verify that the old column list is an initial prefix of the new
         * column list.
         */
        descriptor = BuildDescForRelation(attrList, (Node*)makeString(ORIENTATION_ROW));

        /*
         * During inplace or online upgrade, we may need to drop newly-added
         * view columns to perform rollback. Since these columns should have
         * not been visible to users, we just skip the safety check.
         */
        if (!u_sess->attr.attr_common.IsInplaceUpgrade)
            checkViewTupleDesc(descriptor, rel->rd_att);

        /*
         * The new options list replaces the existing options list, even if
         * it's empty.
         */
        atcmd = makeNode(AlterTableCmd);
        atcmd->subtype = AT_ReplaceRelOptions;
        atcmd->def = (Node*)options;
        atcmds = lappend(atcmds, atcmd);

        /*
         * If new attributes have been added, we must add pg_attribute entries
         * for them.  It is convenient (although overkill) to use the ALTER
         * TABLE ADD COLUMN infrastructure for this.
         *
         * When we roll back upgrade by dropping view columns, we use the
         * ALTER TABLE DROP COLUMN infrastructure.
         */
        flag = (list_length(attrList) < rel->rd_att->natts) && u_sess->attr.attr_common.IsInplaceUpgrade;
        if (list_length(attrList) > rel->rd_att->natts) {
            ListCell* c = NULL;
            int skip = rel->rd_att->natts;

            foreach (c, attrList) {
                if (skip > 0) {
                    skip--;
                    continue;
                }
                atcmd = makeNode(AlterTableCmd);
                atcmd->subtype = AT_AddColumnToView;
                atcmd->def = (Node*)lfirst(c);
                atcmds = lappend(atcmds, atcmd);
            }
        } else if (flag) {
            for (int dropcolno = rel->rd_att->natts - 1; dropcolno >= list_length(attrList); dropcolno--) {
                atcmd = makeNode(AlterTableCmd);
                atcmd->subtype = AT_DropColumn;
                atcmd->name = rel->rd_att->attrs[dropcolno].attname.data;
                atcmd->behavior = DROP_RESTRICT;
                atcmd->missing_ok = true;
                atcmds = lappend(atcmds, atcmd);
            }
        }

        /* OK, let's do it. */
        AlterTableInternal(viewOid, atcmds, true);

        /*
         * Seems okay, so return the OID of the pre-existing view.
         */
        relation_close(rel, NoLock); /* keep the lock! */

        return viewOid;
    } else {
        Oid relid;

        /*
         * now set the parameters for keys/inheritance etc. All of these are
         * uninteresting for views...
         */
        createStmt->relation = relation;
        createStmt->tableElts = attrList;
        createStmt->inhRelations = NIL;
        createStmt->constraints = NIL;
        createStmt->options = options;
        createStmt->options = lappend(options, defWithOids(false));
        createStmt->oncommit = ONCOMMIT_NOOP;
        createStmt->tablespacename = NULL;
        createStmt->if_not_exists = false;

        /*
         * finally create the relation (this will error out if there's an
         * existing view, so we don't need more code to complain if "replace"
         * is false).
         */
        if (relkind == OBJECT_CONTQUERY) {
            relid = DefineRelation(createStmt, RELKIND_CONTQUERY, InvalidOid);
        } else {
            relid = DefineRelation(createStmt, RELKIND_VIEW, InvalidOid);
        }
        Assert(relid != InvalidOid);
        return relid;
    }
}

/*
 * Verify that tupledesc associated with proposed new view definition
 * matches tupledesc of old view.  This is basically a cut-down version
 * of equalTupleDescs(), with code added to generate specific complaints.
 * Also, we allow the new tupledesc to have more columns than the old.
 */
static void checkViewTupleDesc(TupleDesc newdesc, TupleDesc olddesc)
{
    int i;

    if (newdesc->natts < olddesc->natts)
        ereport(ERROR, (errcode(ERRCODE_INVALID_TABLE_DEFINITION), errmsg("cannot drop columns from view")));
    /* we can ignore tdhasoid */
    for (i = 0; i < olddesc->natts; i++) {
        Form_pg_attribute newattr = &newdesc->attrs[i];
        Form_pg_attribute oldattr = &olddesc->attrs[i];

        /* XXX msg not right, but we don't support DROP COL on view anyway */
        if (newattr->attisdropped != oldattr->attisdropped)
            ereport(ERROR, (errcode(ERRCODE_INVALID_TABLE_DEFINITION), errmsg("cannot drop columns from view")));

        if (strcmp(NameStr(newattr->attname), NameStr(oldattr->attname)) != 0)
            ereport(ERROR,
                (errcode(ERRCODE_INVALID_TABLE_DEFINITION),
                    errmsg("cannot change name of view column \"%s\" to \"%s\"",
                        NameStr(oldattr->attname),
                        NameStr(newattr->attname))));
        /* XXX would it be safe to allow atttypmod to change?  Not sure */
        if (newattr->atttypid != oldattr->atttypid || newattr->atttypmod != oldattr->atttypmod)
            ereport(ERROR,
                (errcode(ERRCODE_INVALID_TABLE_DEFINITION),
                    errmsg("cannot change data type of view column \"%s\" from %s to %s",
                        NameStr(oldattr->attname),
                        format_type_with_typemod(oldattr->atttypid, oldattr->atttypmod),
                        format_type_with_typemod(newattr->atttypid, newattr->atttypmod))));
        /* We can ignore the remaining attributes of an attribute... */
    }

    /*
     * We ignore the constraint fields.  The new view desc can't have any
     * constraints, and the only ones that could be on the old view are
     * defaults, which we are happy to leave in place.
     */
}

static void DefineViewRules(Oid viewOid, Query* viewParse, bool replace)
{
    /*
     * Set up the ON SELECT rule.  Since the query has already been through
     * parse analysis, we use DefineQueryRewrite() directly.
     */
    DefineQueryRewrite(pstrdup(ViewSelectRuleName), viewOid, NULL, CMD_SELECT, true, replace, list_make1(viewParse));

    /*
     * Someday: automatic ON INSERT, etc
     */
}

/* ---------------------------------------------------------------
 * UpdateRangeTableOfViewParse
 *
 * Update the range table of the given parsetree.
 * This update consists of adding two new entries IN THE BEGINNING
 * of the range table (otherwise the rule system will die a slow,
 * horrible and painful death, and we do not want that now, do we?)
 * one for the OLD relation and one for the NEW one (both of
 * them refer in fact to the "view" relation).
 *
 * Of course we must also increase the 'varnos' of all the Var nodes
 * by 2...
 *
 * These extra RT entries are not actually used in the query,
 * except for run-time permission checking.
 * ---------------------------------------------------------------
 */
static Query* UpdateRangeTableOfViewParse(Oid viewOid, Query* viewParse)
{
    Relation viewRel;
    List* new_rt = NIL;
    RangeTblEntry *rt_entry1, *rt_entry2;

    /*
     * Make a copy of the given parsetree.	It's not so much that we don't
     * want to scribble on our input, it's that the parser has a bad habit of
     * outputting multiple links to the same subtree for constructs like
     * BETWEEN, and we mustn't have OffsetVarNodes increment the varno of a
     * Var node twice.	copyObject will expand any multiply-referenced subtree
     * into multiple copies.
     */
    viewParse = (Query*)copyObject(viewParse);

    /* need to open the rel for addRangeTableEntryForRelation */
    viewRel = relation_open(viewOid, AccessShareLock);

    /*
     * Create the 2 new range table entries and form the new range table...
     * OLD first, then NEW....
     */
    rt_entry1 = addRangeTableEntryForRelation(NULL, viewRel, makeAlias("old", NIL), false, false);
    rt_entry2 = addRangeTableEntryForRelation(NULL, viewRel, makeAlias("new", NIL), false, false);
    /* Must override addRangeTableEntry's default access-check flags */
    rt_entry1->requiredPerms = 0;
    rt_entry2->requiredPerms = 0;

    new_rt = lcons(rt_entry1, lcons(rt_entry2, viewParse->rtable));

    viewParse->rtable = new_rt;

    /*
     * Now offset all var nodes by 2, and jointree RT indexes too.
     */
    OffsetVarNodes((Node*)viewParse, 2, 0);

    relation_close(viewRel, AccessShareLock);

    return viewParse;
}

#ifdef ENABLE_MULTIPLE_NODES
static void CreateMvCommand(ViewStmt* stmt, const char* queryString)
{
    Oid groupid = 0;
    char* queryStringinfo = (char*)queryString;
    char* group_name = NULL;

    if (stmt->subcluster == NULL) {
        group_name = ng_get_installation_group_name();
    } else {
        group_name = strVal(linitial(stmt->subcluster->members));
    }

    groupid = get_pgxc_groupoid(group_name);
    if (!OidIsValid(groupid)) {
        ereport(ERROR,
            (errcode(ERRCODE_UNDEFINED_OBJECT),
            errmsg("Target node group \"%s\" doesn't exist", group_name)));
    }

    /* Force add Remotequery. */
    RemoteQuery* step = makeNode(RemoteQuery);
    step->combine_type = COMBINE_TYPE_NONE;
    step->sql_statement = (char*)queryString;
    step->exec_type = EXEC_ON_DATANODES;
    step->is_temp = false;

    step->exec_nodes = makeNode(ExecNodes);
    step->exec_nodes->distribution.group_oid = groupid;

    Oid* members = NULL;
    int nmembers = 0;
    nmembers = get_pgxc_groupmembers(groupid, &members);
    step->exec_nodes->nodeList = GetNodeGroupNodeList(members, nmembers);
    pfree_ext(members);

    /* Recurse for anything else */
    ProcessUtility((Node *)step,
            queryStringinfo,
            NULL,
            false,
            None_Receiver,
            true,
            NULL);

    return;
}
#endif

/*
 * DefineView
 *		Execute a CREATE VIEW command.
 */
Oid DefineView(ViewStmt* stmt, const char* queryString, bool send_remote, bool isFirstNode)
{
    Query* viewParse = NULL;
    Oid viewOid = InvalidOid;
    RangeVar* view = NULL;

    /*
     * Run parse analysis to convert the raw parse tree to a Query.  Note this
     * also acquires sufficient locks on the source table(s).
     */
    if (!IsA(stmt->query, Query)) {
        viewParse = parse_analyze(stmt->query, queryString, NULL, 0);
    } else {
        viewParse = (Query *)stmt->query;
    }

    /*
     * The grammar should ensure that the result is a single SELECT Query.
     * However, it doesn't forbid SELECT INTO, so we have to check for that.
     */
    if (!IsA(viewParse, Query)) {
        ereport(ERROR, (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE), errmsg("unexpected parse analysis result")));
    }

    if (viewParse->utilityStmt != NULL && IsA(viewParse->utilityStmt, CreateTableAsStmt)) {
        ereport(
            ERROR, (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE), errmsg("views must not contain SELECT INTO")));
    }

    if (viewParse->commandType != CMD_SELECT || viewParse->utilityStmt != NULL) {
        ereport(ERROR, (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE), errmsg("unexpected parse analysis result")));
    }

    /*
     * Check for unsupported cases.  These tests are redundant with ones in
     * DefineQueryRewrite(), but that function will complain about a bogus ON
     * SELECT rule, and we'd rather the message complain about a view.
     */
    if (viewParse->hasModifyingCTE) {
        ereport(ERROR,
            (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                errmsg("views must not contain data-modifying statements in WITH")));
    }
#ifdef ENABLE_MULTIPLE_NODES
    validate_streaming_engine_status((Node*) stmt);
#endif
    /*
     * If a list of column names was given, run through and insert these into
     * the actual query tree. - thomas 2000-03-08
     */
    if (stmt->aliases != NIL) {
        ListCell* alist_item = list_head(stmt->aliases);
        ListCell* targetList = NULL;

        foreach (targetList, viewParse->targetList) {
            TargetEntry* te = (TargetEntry*)lfirst(targetList);

            Assert(IsA(te, TargetEntry));
            /* junk columns don't get aliases */
            if (te->resjunk)
                continue;
            te->resname = pstrdup(strVal(lfirst(alist_item)));
            alist_item = lnext(alist_item);
            if (alist_item == NULL)
                break; /* done assigning aliases */
        }

        if (alist_item != NULL)
            ereport(ERROR,
                (errcode(ERRCODE_SYNTAX_ERROR),
                    errmsg("CREATE VIEW specifies more column "
                           "names than columns")));
    }

    /* Unlogged views are not sensible. */
    if (stmt->view->relpersistence == RELPERSISTENCE_UNLOGGED)
        ereport(ERROR,
            (errcode(ERRCODE_SYNTAX_ERROR), errmsg("views cannot be unlogged because they do not have storage")));

    if (stmt->view->relpersistence == RELPERSISTENCE_GLOBAL_TEMP) {
        ereport(ERROR,
            (errcode(ERRCODE_SYNTAX_ERROR), errmsg("views cannot be global temp because they do not have storage")));
    }

    /*
     * If the user didn't explicitly ask for a temporary view, check whether
     * we need one implicitly.	We allow TEMP to be inserted automatically as
     * long as the CREATE command is consistent with that --- no explicit
     * schema name.
     */
    view = (RangeVar*)copyObject(stmt->view); /* don't corrupt original command */
    if (view->relpersistence == RELPERSISTENCE_PERMANENT && isQueryUsingTempRelation(viewParse)) {
        view->relpersistence = RELPERSISTENCE_TEMP;
        ereport(NOTICE, (errmsg("view \"%s\" will be a temporary view", view->relname)));
    }

#ifdef PGXC
    /* In case view is temporary, be sure not to use 2PC on such relations */
    if (view->relpersistence == RELPERSISTENCE_TEMP)
        ExecSetTempObjectIncluded();
#endif

     if (stmt->relkind == OBJECT_MATVIEW) {
        /* Relation Already Created */
        (void)RangeVarGetAndCheckCreationNamespace(view, NoLock, &viewOid, RELKIND_MATVIEW);

#ifdef ENABLE_MULTIPLE_NODES
        /* try to send CREATE MATERIALIZED VIEW to DNs, Only consider PGXC now. */
        if (IS_PGXC_COORDINATOR && !IsConnFromCoord() && !send_remote) {
            CreateMvCommand(stmt, queryString);
        }
#endif
     } else {
        /*
         * Create the view relation
         *
         * NOTE: if it already exists and replace is false, the xact will be
         * aborted.
         */
        viewOid = DefineVirtualRelation(view, viewParse->targetList, stmt->replace, stmt->options, stmt->relkind);

        /*
         * The relation we have just created is not visible to any other commands
         * running with the same transaction & command id. So, increment the
         * command id counter (but do NOT pfree any memory!!!!)
         */
        CommandCounterIncrement();
    }

    StoreViewQuery(viewOid, viewParse, stmt->replace);

    return viewOid;
}

bool IsViewTemp(ViewStmt* stmt, const char* queryString)
{
    Query* viewParse = NULL;
    RangeVar* view = NULL;
    view = (RangeVar*)copyObject(((ViewStmt*)stmt)->view); /* don't corrupt original command */
    viewParse = parse_analyze((Node*)copyObject(((ViewStmt*)stmt)->query), queryString, NULL, 0, false, true);
    /*
     * If the user didn't explicitly ask for a temporary view, check whether
     * we need one implicitly.	We allow TEMP to be inserted automatically as
     * long as the CREATE command is consistent with that --- no explicit
     * schema name.
     */
    if (view->relpersistence == RELPERSISTENCE_PERMANENT && isQueryUsingTempRelation(viewParse)) {
        view->relpersistence = RELPERSISTENCE_TEMP;
    }

    if (view->relpersistence == RELPERSISTENCE_TEMP)
        return true;
    return false;
}

/*
 * Use the rules system to store the query for the view.
 */
void
StoreViewQuery(Oid viewOid, Query *viewParse, bool replace)
{
    /*
     * The range table of 'viewParse' does not contain entries for the "OLD"
     * and "NEW" relations. So... add them!
     */
    viewParse = UpdateRangeTableOfViewParse(viewOid, viewParse);

    /*
     * Now create the rules associated with the view.
     */
    DefineViewRules(viewOid, viewParse, replace);
}
