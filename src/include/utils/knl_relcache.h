/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * openGauss is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * ---------------------------------------------------------------------------------------
 *
 * knl_relcache.h
 *
 *
 *
 * IDENTIFICATION
 *        src/include/utils/knl_relcache.h
 *
 * ---------------------------------------------------------------------------------------
 */

#ifndef KNL_RELCACHE_H
#define KNL_RELCACHE_H
#include "postgres.h"
#include "catalog/pg_attribute.h"
#include "catalog/pg_class.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "catalog/pg_database.h"
#include "catalog/pg_authid.h"
#include "catalog/pg_auth_members.h"
#include "catalog/pg_user_status.h"
/*
 *		hardcoded tuple descriptors, contents generated by genbki.pl
 */
extern const FormData_pg_attribute Desc_pg_class[Natts_pg_class];
extern const FormData_pg_attribute Desc_pg_attribute[Natts_pg_attribute];
extern const FormData_pg_attribute Desc_pg_proc[Natts_pg_proc];
extern const FormData_pg_attribute Desc_pg_type[Natts_pg_type];
extern const FormData_pg_attribute Desc_pg_database[Natts_pg_database];
extern const FormData_pg_attribute Desc_pg_authid[Natts_pg_authid];
extern const FormData_pg_attribute Desc_pg_auth_members[Natts_pg_auth_members];
extern const FormData_pg_attribute Desc_pg_index[Natts_pg_index];
extern const FormData_pg_attribute Desc_pg_user_status[Natts_pg_user_status];

/* non-export function prototypes */
extern void RelationDestroyRelation(Relation relation, bool remember_tupdesc);
extern void RelationClearRelation(Relation relation, bool rebuild);
extern void RelationReloadIndexInfo(Relation relation);
extern Relation RelationBuildDesc(Oid targetRelId, bool insertIt, bool buildkey = true);

extern void formrdesc(const char *relationName, Oid relationReltype, bool isshared, bool hasoids, int natts,
                      const FormData_pg_attribute *attrs);

extern void RelationInitPhysicalAddr(Relation relation);
extern Relation load_critical_index(Oid indexoid, Oid heapoid);

extern TupleDesc GetPgClassDescriptor(void);
extern TupleDesc GetPgIndexDescriptor(void);

extern void SetBackendId(Relation relation);
extern void RelationBuildRuleLock(Relation relation);
extern void RelationCacheInvalidOid(Relation relation);

#define RelationIdCacheInsertIntoLocal(RELATION)                                                       \
    do {                                                                                               \
        if (EnableLocalSysCache()) {                                                                   \
            t_thrd.lsc_cxt.lsc->tabdefcache.InsertRelationIntoLocal((RELATION));                       \
        } else {                                                                                       \
            RelIdCacheEnt *idhentry = NULL;                                                            \
            bool found = false;                                                                        \
            idhentry = (RelIdCacheEnt *)hash_search(u_sess->relcache_cxt.RelationIdCache,              \
                                                    (void *)&((RELATION)->rd_id), HASH_ENTER, &found); \
            /* used to give notice if found -- now just keep quiet */                                  \
            idhentry->reldesc = RELATION;                                                              \
        }                                                                                              \
    } while (0)

#define RelationIdCacheLookup(ID, RELATION)                                                                         \
    do {                                                                                                            \
        if (EnableLocalSysCache()) {                                                                                \
            (RELATION) = t_thrd.lsc_cxt.lsc->tabdefcache.SearchRelation((ID));                                      \
        } else {                                                                                                    \
            RelIdCacheEnt *hentry = NULL;                                                                           \
            hentry =                                                                                                \
                (RelIdCacheEnt *)hash_search(u_sess->relcache_cxt.RelationIdCache, (void *)&(ID), HASH_FIND, NULL); \
            if (hentry != NULL)                                                                                     \
                (RELATION) = hentry->reldesc;                                                                       \
            else                                                                                                    \
                (RELATION) = NULL;                                                                                  \
        }                                                                                                           \
    } while (0)

#define RelationIdCacheLookupOnlyLocal(ID, RELATION)                                                                \
    do {                                                                                                            \
        if (EnableLocalSysCache()) {                                                                                \
            (RELATION) = t_thrd.lsc_cxt.lsc->tabdefcache.SearchRelationFromLocal((ID));                             \
        } else {                                                                                                    \
            RelIdCacheEnt *hentry = NULL;                                                                           \
            hentry =                                                                                                \
                (RelIdCacheEnt *)hash_search(u_sess->relcache_cxt.RelationIdCache, (void *)&(ID), HASH_FIND, NULL); \
            if (hentry != NULL)                                                                                     \
                (RELATION) = hentry->reldesc;                                                                       \
            else                                                                                                    \
                (RELATION) = NULL;                                                                                  \
        }                                                                                                           \
    } while (0)

#define RelationCacheDeleteLocal(RELATION)                                                            \
    do {                                                                                              \
        if (EnableLocalSysCache()) {                                                                  \
            (void)t_thrd.lsc_cxt.lsc->tabdefcache.RemoveRelation(RELATION);                           \
        } else {                                                                                      \
            RelIdCacheEnt *idhentry;                                                                  \
            idhentry = (RelIdCacheEnt *)hash_search(u_sess->relcache_cxt.RelationIdCache,             \
                                                    (void *)&((RELATION)->rd_id), HASH_REMOVE, NULL); \
            if (idhentry == NULL)                                                                     \
                ereport(WARNING, (errmsg("trying to delete a rd_id reldesc that does not exist")));   \
        }                                                                                             \
    } while (0)


inline TupleDesc GetLSCPgClassDescriptor()
{
    if (EnableLocalSysCache()) {
        return t_thrd.lsc_cxt.lsc->tabdefcache.GetPgClassDescriptor();
    } else {
        return GetPgClassDescriptor();
    }
}

inline TupleDesc GetLSCPgIndexDescriptor()
{
    if (EnableLocalSysCache()) {
        return t_thrd.lsc_cxt.lsc->tabdefcache.GetPgIndexDescriptor();
    } else {
        return GetPgIndexDescriptor();
    }
}

inline bool LocalRelCacheCriticalRelcachesBuilt()
{
    if (EnableLocalSysCache()) {
        return t_thrd.lsc_cxt.lsc->tabdefcache.criticalRelcachesBuilt;
    } else {
        return u_sess->relcache_cxt.criticalRelcachesBuilt;
    }
}
inline void SetLocalRelCacheCriticalRelcachesBuilt(bool value)
{
    if (EnableLocalSysCache()) {
        t_thrd.lsc_cxt.lsc->tabdefcache.criticalRelcachesBuilt = value;
    } else {
        u_sess->relcache_cxt.criticalRelcachesBuilt = value;
    }
}

inline bool LocalRelCacheCriticalSharedRelcachesBuilt()
{
    if (EnableLocalSysCache()) {
        return t_thrd.lsc_cxt.lsc->tabdefcache.criticalSharedRelcachesBuilt;
    } else {
        return u_sess->relcache_cxt.criticalSharedRelcachesBuilt;
    }
}
inline void SetLocalRelCacheCriticalSharedRelcachesBuilt(bool value)
{
    if (EnableLocalSysCache()) {
        t_thrd.lsc_cxt.lsc->tabdefcache.criticalSharedRelcachesBuilt = value;
    } else {
        u_sess->relcache_cxt.criticalSharedRelcachesBuilt = value;
    }
}

inline long LocalRelCacheInvalsReceived()
{
    if (EnableLocalSysCache()) {
        return t_thrd.lsc_cxt.lsc->tabdefcache.relcacheInvalsReceived;
    } else {
        return u_sess->relcache_cxt.relcacheInvalsReceived;
    }
}

inline void AddLocalRelCacheInvalsReceived(int value)
{
    if (EnableLocalSysCache()) {
        t_thrd.lsc_cxt.lsc->tabdefcache.relcacheInvalsReceived += value;
    } else {
        u_sess->relcache_cxt.relcacheInvalsReceived += value;
    }
}

inline bool GetRelCacheNeedEOXActWork()
{
    if (EnableLocalSysCache()) {
        return t_thrd.lsc_cxt.lsc->tabdefcache.RelCacheNeedEOXActWork;
    } else {
        return u_sess->relcache_cxt.RelCacheNeedEOXActWork;
    }
}

inline void SetRelCacheNeedEOXActWork(bool value)
{
    if (EnableLocalSysCache()) {
        t_thrd.lsc_cxt.lsc->tabdefcache.RelCacheNeedEOXActWork = value;
    } else {
        u_sess->relcache_cxt.RelCacheNeedEOXActWork = value;
    }
}

inline List *LocalRelCacheInitFileRelationIds()
{
    if (EnableLocalSysCache()) {
        return t_thrd.lsc_cxt.lsc->tabdefcache.initFileRelationIds;
    } else {
        return u_sess->relcache_cxt.initFileRelationIds;
    }
}

inline void LconsLocalRelCacheInitFileRelationIds(Relation rel)
{
    if (EnableLocalSysCache()) {
        t_thrd.lsc_cxt.lsc->tabdefcache.initFileRelationIds =
            lcons_oid(RelationGetRelid(rel), t_thrd.lsc_cxt.lsc->tabdefcache.initFileRelationIds);
    } else {
        u_sess->relcache_cxt.initFileRelationIds =
            lcons_oid(RelationGetRelid(rel), u_sess->relcache_cxt.initFileRelationIds);
    }
}

inline void ClearLocalRelCacheInitFileRelationIds()
{
    if (EnableLocalSysCache()) {
        Assert(t_thrd.lsc_cxt.lsc->tabdefcache.initFileRelationIds == NIL);
        t_thrd.lsc_cxt.lsc->tabdefcache.initFileRelationIds = NIL;
    } else {
        u_sess->relcache_cxt.initFileRelationIds = NIL;
    }
}

inline List *LocalRelCacheGBucketMapCache()
{
    if (EnableLocalSysCache()) {
        return t_thrd.lsc_cxt.lsc->tabdefcache.g_bucketmap_cache;
    } else {
        return u_sess->relcache_cxt.g_bucketmap_cache;
    }
}

inline void AppendLocalRelCacheGBucketMapCache(ListCell *cell)
{
    if (EnableLocalSysCache()) {
        t_thrd.lsc_cxt.lsc->tabdefcache.g_bucketmap_cache =
            lappend(t_thrd.lsc_cxt.lsc->tabdefcache.g_bucketmap_cache, cell);
    } else {
        u_sess->relcache_cxt.g_bucketmap_cache = lappend(u_sess->relcache_cxt.g_bucketmap_cache, cell);
    }
}

inline void DeteleLocalRelCacheGBucketMapCache(ListCell *cell, ListCell *prev)
{
    if (EnableLocalSysCache()) {
        t_thrd.lsc_cxt.lsc->tabdefcache.g_bucketmap_cache =
            list_delete_cell(t_thrd.lsc_cxt.lsc->tabdefcache.g_bucketmap_cache, cell, prev);
    } else {
        u_sess->relcache_cxt.g_bucketmap_cache = list_delete_cell(u_sess->relcache_cxt.g_bucketmap_cache, cell, prev);
    }
}

inline uint32 LocalRelCacheMaxBucketMapSize()
{
    if (EnableLocalSysCache()) {
        return t_thrd.lsc_cxt.lsc->tabdefcache.max_bucket_map_size;
    } else {
        return u_sess->relcache_cxt.max_bucket_map_size;
    }
}

inline void EnlargeLocalRelCacheMaxBucketMapSize(double ratio)
{
    if (EnableLocalSysCache()) {
        t_thrd.lsc_cxt.lsc->tabdefcache.max_bucket_map_size *= ratio;
    } else {
        u_sess->relcache_cxt.max_bucket_map_size *= ratio;
    }
}

extern void RememberToFreeTupleDescAtEOX(TupleDesc td);

#endif