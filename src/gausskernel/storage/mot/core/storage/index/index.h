/*
 * Copyright (c) 2020 Huawei Technologies Co.,Ltd.
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
 * -------------------------------------------------------------------------
 *
 * index.h
 *    Base class for primary and secondary index.
 *
 * IDENTIFICATION
 *    src/gausskernel/storage/mot/core/storage/index/index.h
 *
 * -------------------------------------------------------------------------
 */

#ifndef MOT_INDEX_H
#define MOT_INDEX_H

#include "global.h"
#include "index_defs.h"
#include "key.h"
#include "index_iterator.h"
#include "txn.h"
#include "sentinel.h"
#include "primary_sentinel.h"
#include "secondary_sentinel.h"
#include "secondary_sentinel_unique.h"
#include "object_pool.h"
#include "utilities.h"

#include <string>
using namespace std;

namespace MOT {
#define NON_UNIQUE_INDEX_SUFFIX_LEN 8

/**
 * @class Index
 * @brief This base class for primary and secondary index.
 */
class Index {
    friend class Table;

protected:
    /**
     * @brief Constructor.
     * @param indexing_method The method used for indexing (hash or tree).
     */
    Index(IndexOrder indexOrder, IndexingMethod indexingMethod)
        : m_keyLength(0),
          m_indexOrder(indexOrder),
          m_indexingMethod(indexingMethod),
          m_indexExtId(0),
          m_startCSN(-1),
          m_indexId(0),
          m_keyPool(nullptr),
          m_sentinelPool(nullptr),
          m_sSentinelVersionPool(nullptr),
          m_table(nullptr)
    {}

public:
    /**
     * @brief Destructor.
     */
    virtual ~Index()
    {
        if (m_keyPool != nullptr) {
            ObjAllocInterface::FreeObjPool(&m_keyPool);
            m_keyPool = nullptr;
        }
        if (m_sentinelPool != nullptr) {
            ObjAllocInterface::FreeObjPool(&m_sentinelPool);
            m_sentinelPool = nullptr;
        }
        if (m_sSentinelVersionPool != nullptr) {
            ObjAllocInterface::FreeObjPool(&m_sSentinelVersionPool);
            m_sSentinelVersionPool = nullptr;
        }
        if (m_colBitmap != nullptr) {
            delete[] m_colBitmap;
            m_colBitmap = nullptr;
        }

        m_table = nullptr;
    }

    Index* CloneEmpty();

    /**
     * @brief Initializes the index.
     * @param keyLength The maximum length in bytes of index keys.
     * @param name The name of the index.
     * @param args Null-terminated list of any additional arguments required by a specific index
     * implementation.
     * @return Return code denoting success or error.
     */
    inline RC IndexInit(uint32_t keyLength, bool isUnique, const string& name, void** args = nullptr)
    {
        // Any change in the init needs to be done also in the cloneEmpty method!!!
        m_keyLength = keyLength + (isUnique ? 0 : NON_UNIQUE_INDEX_SUFFIX_LEN);
        m_name = name;
        m_unique = isUnique;

        m_indexId = m_indexCounter.fetch_add(1, std::memory_order_relaxed);
        if (m_indexExtId == 0) {
            m_indexExtId = m_indexId;
        }

        if (m_keyLength > MAX_KEY_SIZE) {
            return RC_INDEX_EXCEEDS_MAX_SIZE;
        }

        m_keyPool = ObjAllocInterface::GetObjPool(sizeof(Key) + ALIGN8(m_keyLength), false);
        if (m_keyPool == nullptr) {
            MOT_REPORT_ERROR(
                MOT_ERROR_OOM, "Index Initialization", "Failed to allocate key pool for index %s", name.c_str());
            return RC_MEMORY_ALLOCATION_ERROR;  // safe cleanup during destructor
        }
        if (m_indexOrder == IndexOrder::INDEX_ORDER_PRIMARY) {
            m_sentinelPool = ObjAllocInterface::GetObjPool(sizeof(PrimarySentinel), false);
        } else {
            if (m_unique) {
                m_sSentinelVersionPool = ObjAllocInterface::GetObjPool(sizeof(PrimarySentinelNode), false);
                if (m_sSentinelVersionPool == nullptr) {
                    MOT_REPORT_ERROR(MOT_ERROR_OOM,
                        "Index Initialization",
                        "Failed to allocate sentinel pool for index %s",
                        name.c_str());
                    return RC_MEMORY_ALLOCATION_ERROR;  // safe cleanup during destructor
                }
                m_sentinelPool = ObjAllocInterface::GetObjPool(sizeof(SecondarySentinelUnique), false);
            } else {
                m_sentinelPool = ObjAllocInterface::GetObjPool(sizeof(SecondarySentinel), false);
            }
        }
        if (m_sentinelPool == nullptr) {
            MOT_REPORT_ERROR(
                MOT_ERROR_OOM, "Index Initialization", "Failed to allocate sentinel pool for index %s", name.c_str());
            return RC_MEMORY_ALLOCATION_ERROR;  // safe cleanup during destructor
        }
        return IndexInitImpl(args);
    }

    inline bool IsKeySizeValid(uint32_t len, bool isUnique) const
    {
        len += (isUnique ? 0 : 8);
        if (len > MAX_KEY_SIZE) {
            return false;
        }

        return true;
    }

    inline uint32_t GetKeySizeNoSuffix() const
    {
        return (m_unique ? m_keyLength : m_keyLength - NON_UNIQUE_INDEX_SUFFIX_LEN);
    }

    /**
     * @brief Clears object pool thread level cache
     */
    virtual void ClearThreadMemoryCache()
    {
        if (m_keyPool != nullptr) {
            m_keyPool->ClearThreadCache();
        }
        if (m_sentinelPool != nullptr) {
            m_sentinelPool->ClearThreadCache();
        }
        if (m_sSentinelVersionPool != nullptr) {
            m_sSentinelVersionPool->ClearThreadCache();
        }
    }

    /**
     * @brief Clears object pool level cache
     */
    virtual void ClearFreeCache()
    {
        if (m_keyPool != nullptr) {
            m_keyPool->ClearFreeCache();
        }
        if (m_sentinelPool != nullptr) {
            m_sentinelPool->ClearFreeCache();
        }
        if (m_sSentinelVersionPool != nullptr) {
            m_sSentinelVersionPool->ClearFreeCache();
        }
    }

    /**
     * @brief Allocate sentinel from memory pool
     */
    Sentinel* SentinelAlloc()
    {
        switch (m_indexOrder) {
            case IndexOrder::INDEX_ORDER_PRIMARY:
                return m_sentinelPool->Alloc<PrimarySentinel>();
            case IndexOrder::INDEX_ORDER_SECONDARY:
                return m_sentinelPool->Alloc<SecondarySentinel>();
            case IndexOrder::INDEX_ORDER_SECONDARY_UNIQUE:
                return m_sentinelPool->Alloc<SecondarySentinelUnique>();
            default:
                break;
        }
        return nullptr;
    }

    /**
     * @brief Release sentinel to memory pool
     */
    void SentinelRelease(Sentinel* sentinel)
    {
        switch (m_indexOrder) {
            case IndexOrder::INDEX_ORDER_PRIMARY:
                m_sentinelPool->Release<PrimarySentinel>(static_cast<PrimarySentinel*>(sentinel));
                break;
            case IndexOrder::INDEX_ORDER_SECONDARY:
                m_sentinelPool->Release<SecondarySentinel>(static_cast<SecondarySentinel*>(sentinel));
                break;
            case IndexOrder::INDEX_ORDER_SECONDARY_UNIQUE:
                m_sentinelPool->Release<SecondarySentinelUnique>(static_cast<SecondarySentinelUnique*>(sentinel));
                break;
            default:
                break;
        }
    }

    void ReclaimSentinel(Sentinel* sentinel);

    /**
     * @brief Allocate the sentinel node from the memory pool
     * @return pointer to the newly allocated node
     */
    PrimarySentinelNode* SentinelNodeAlloc()
    {
        return m_sSentinelVersionPool->Alloc<PrimarySentinelNode>();
    }

    /**
     * @brief Release the sentinel node to the memory pool
     *  @param node pointer to the node we want to reclaim
     * @return
     */
    void SentinelNodeRelease(PrimarySentinelNode* node)
    {
        m_sSentinelVersionPool->Release<PrimarySentinelNode>(node);
    }

    /**
     * @brief GC callback function for sentinel reclamation
     *  @param gcElement element with all the necessary meta-data
     *  @param oper Current GC operation
     *  @param unused part of the function pointer signature
     * @return The name of the index.
     */
    static uint32_t SentinelDtor(void* gcElement, void* oper, void* aux)
    {
        GC_OPERATION_TYPE gcOperType = (*(GC_OPERATION_TYPE*)oper);
        LimboElement* elem = reinterpret_cast<LimboElement*>(gcElement);
        // If drop_index == true, all index's pools are going to be cleaned, so we skip the release here
        Sentinel* sent = reinterpret_cast<Sentinel*>(elem->m_objectPtr);
        Index* ind = sent->GetIndex();
        uint32_t gcSlot = 0;
        uint32_t size = ind->m_sentinelPool->m_size;
        if (sent->IsPrimaryIndex()) {
            PrimarySentinel* ps = reinterpret_cast<PrimarySentinel*>(sent);
            gcSlot = ps->GetGcInfo().RefCountUpdate(DEC);
            if ((gcSlot == 1) and ps->IsSentinelRemovable()) {
                if (gcOperType != GC_OPERATION_TYPE::GC_OPER_DROP_INDEX) {
                    MOT_LOG_DEBUG(
                        "SentinelDtor Deleting  %p startCSN %lu CSN %lu ", (void*)sent, ps->GetStartCSN(), elem->m_csn);
                    ind->ReclaimSentinel(sent);
                } else {
                    MOT_LOG_DEBUG(
                        "SentinelDtor Skipping %p startCSN %lu CSN %lu", (void*)sent, ps->GetStartCSN(), elem->m_csn);
                }
            }
        } else {
            if (gcOperType != GC_OPERATION_TYPE::GC_OPER_DROP_INDEX) {
                ind->ReclaimSentinel(sent);
            }
        }
        return size;
    }

    /**
     * @brief a callback function to remove and destroy a deleted version chain.
     * @param gcElement A place holder for the gc element metadata.
     * @param oper GC operation for the current element
     * @param aux a pointer to the GC delete vector
     * @return size of cleaned element
     */
    static uint32_t DeleteKeyDtor(void* gcElement, void* oper, void* aux)
    {
        GC_OPERATION_TYPE gcOperType = (*(GC_OPERATION_TYPE*)oper);
        LimboElement* elem = reinterpret_cast<LimboElement*>(gcElement);
        GcQueue::DeleteVector* deleteVector = static_cast<GcQueue::DeleteVector*>(aux);
        // If drop_index == true, all index's pools are going to be cleaned, so we skip the release here
        Sentinel* sentinel = reinterpret_cast<Sentinel*>(elem->m_objectPtr);
        Key* key = reinterpret_cast<Key*>(elem->m_objectPool);
        Index* index_ = sentinel->GetIndex();
        uint32_t size = index_->m_sentinelPool->m_size;
        if (unlikely(gcOperType == GC_OPERATION_TYPE::GC_OPER_DROP_INDEX)) {
            return size;
        }
        RC rc = sentinel->RefCountUpdate(DEC);
        if (rc == RC::RC_INDEX_DELETE) {
            Sentinel* outputSen = index_->IndexRemove(key, 0);
            MOT_ASSERT(outputSen == sentinel);
            deleteVector->push_back(outputSen);
        }
        index_->DestroyKey(key);
        return size;
    }

    // Meta-data API
    /**
     * @brief Retrieves the name of the index.
     * @return The name of the index.
     */
    inline const string& GetName() const
    {
        return m_name;
    }

    /**
     * @brief Retrieves the maximum length in bytes of keys used by the index.
     * @return The key length in bytes.
     */
    inline uint32_t GetKeyLength() const
    {
        return m_keyLength;
    }

    inline uint32_t GetAlignedKeyLength() const
    {
        return ALIGN8(m_keyLength);
    }

    /**
     * @brief Retrieves the index order (primary or secondary).
     * @return The index order.
     */
    inline IndexOrder GetIndexOrder() const
    {
        return m_indexOrder;
    }

    /**
     * @brief Retrieves the method of indexing used by the underlying index implementation.
     * @return The indexing method.
     */
    inline IndexingMethod GetIndexingMethod() const
    {
        return m_indexingMethod;
    }

    /**
     * @brief Retrieves the number of rows stored in the index. This may be an estimation.
     * @return The number of rows stored in the index.
     */
    virtual uint64_t GetSize() const;

    /**
     * @brief Re-initialize index back to empty and compacted one.
     */
    virtual RC ReInitIndex(bool isDrop) = 0;

    // Iterator API
    /**
     * @brief Create a forward iterator to the first item in the index.
     *
     * @detail This API is provided to support full index scans. All index types support this API.
     * The returned iterator is guaranteed to be a forward iterator. Some index types may provide a
     * bidirectional iterator as well (see IIndexIterator). You may query the returned iterator to
     * find out whether it is bidirectional or not.
     *
     * @param pid The logical identifier of the requesting thread.
     * @return An iterator to the first item in the index.
     */
    virtual IndexIterator* Begin(uint32_t pid, bool passive = false) const = 0;

    /**
     * @brief Retrieves a reverse iterator pointing to the first index item in logical order.
     * Practically the iterator points to the last item of the index and moves backwards when
     * calling IndexIterator::operator++(). Not all index types may support this API.
     *
     * @detail This API is provided to support full index scans in reverse order. Pay attention that
     * unlike end() this method provides an iterator pointing to a valid index item. The iterator is
     * guaranteed to be a reverse iterator (see IIndexIterator).
     *
     * @param pid The logical identifier of the requesting thread.
     * @return A reverse iterator pointing to the first index item in logical order, or the end()
     * forward iterator if this API is not supported.
     */
    virtual IndexIterator* ReverseBegin(uint32_t pid) const;

    /**
     * @brief Searches for a key in the index, returning an iterator to the closest matching key
     * according to the search criteria.
     * @param key The key to search. May be a partial key.
     * @param matchKey Specifies whether to include in the result an exact match of the key. Specify
     * true for point queries and range queries that should include the range boundary.
     * @param forward Specifies the search direction and the direction of the resulting iterator.
     * @param pid The logical identifier of the requesting thread.
     * @param[out] found Returns information whether an exact match was found. Check this output
     * parameter when issuing point queries.
     * @return The resulting iterator. The caller is responsible whether to use the iterator or not
     * in case the requested key was not found exactly.
     */
    virtual IndexIterator* Search(
        const Key* key, bool matchKey, bool forward, uint32_t pid, bool& found, bool passive = false) const = 0;

    /**
     * @brief Retrieves a forward iterator to the first item in the index that has the required key,
     * or the end iterator if the key was not found.
     *
     * @detail This API is provided to support point queries, that may be extended to range queries.
     * The iterator is guaranteed to be a forward iterator. Some index types may provide a
     * bidirectional iterator as well (see IIndexIterator). You may query the returned iterator to
     * find out whether it is bidirectional or not.
     *
     * @param key The key to search.
     * @param pid The logical identifier of the requesting thread.
     * @return An iterator pointing to the first item matching the searched key or the end iterator.
     */
    virtual IndexIterator* Find(const Key* key, uint32_t pid) const;

    /**
     * @brief Retrieves a forward iterator to the last item in the index that has the searched key,
     * or the end iterator if the key was not found. Not all index types may support this API.
     *
     * @detail This API is provided to support index types that allow for duplicate keys. The
     * iterator is guaranteed to be a forward iterator. Some index types may provide a bidirectional
     * iterator as well (see IIndexIterator). You may query the returned iterator to find out
     * whether it is bidirectional or not.
     *
     * Not all index implementations may support this API, in which case the end() forward iterator
     * is always returned.
     *
     * @param key The key to search.
     * @param pid The logical identifier of the requesting thread.
     * @return An iterator pointing to the last first item matching the searched key or the end
     * iterator.
     */
    virtual IndexIterator* FindLast(const Key* key, uint32_t pid) const;

    /**
     * @brief Retrieves a reverse iterator to the first item in the index that has the required key,
     * or the end iterator if the key was not found. Not all index types may support this API.
     *
     * @detail This API is provided to support point queries, that may be extended to range queries.
     * The iterator is guaranteed to be a reverse iterator. Some index types may provide a
     * bidirectional iterator as well (see IIndexIterator). You may query the returned iterator to
     * find out whether it is bidirectional or not.
     *
     * @param key The key to search.
     * @param pid The logical identifier of the requesting thread.
     * @return A reverse iterator pointing to the first item matching the searched key or the end
     * iterator.
     */
    virtual IndexIterator* ReverseFind(const Key* key, uint32_t pid) const;

    /**
     * @brief Retrieves a reverse iterator to the last item in the index that has the searched key,
     * or the end iterator if the key was not found. Not all index types may support this API.
     *
     * @detail This API is provided to support index types that allow for duplicate keys. The
     * iterator is guaranteed to be a reverse iterator. Some index types may provide a bidirectional
     * iterator as well (see IIndexIterator). You may query the returned iterator to find out
     * whether it is bidirectional or not.
     *
     * Not all index implementations may support this API, in which case the end() forward iterator
     * is always returned.
     *
     * @param key The key to search.
     * @param pid The logical identifier of the requesting thread.
     * @return A reverse iterator pointing to the last first item matching the searched key or the
     * end iterator.
     */
    virtual IndexIterator* ReverseFindLast(const Key* key, uint32_t pid) const;

    /**
     * @brief Retrieves a forward iterator to the smallest item that has a key larger than or equal
     * to the specified key. Not all index types may support this API, in which case the end
     * iterator is always returned.
     *
     * @detail This API is provided to support range queries.
     *
     * Not all index implementations may support this API, in which case the end iterator is always
     * returned.
     *
     * In mathematical terms the returned iterator points to the following item:
     * <code>
     * min( {x | y belongs to index AND x >= y} )
     * </code>
     *
     * The returned iterator is guaranteed to be a forward iterator. Some index types may provide a
     * bidirectional iterator as well (see IIndexIterator). You may query the returned iterator to
     * find out whether it is bidirectional or not.
     *
     * @param key The key to search.
     * @param pid The logical identifier of the requesting thread.
     * @return An iterator to first item matching the searched key or the next key if not found. The
     * end iterator may be returned if no items are matching. The end iterator is also returned if
     * the index does not support this API.
     */
    virtual IndexIterator* LowerBound(const Key* key, uint32_t pid) const;

    /**
     * @brief Retrieves a forward iterator to the smallest item that has a key larger than the
     * specified key. Not all index types may support this API, in which case the end iterator is
     * always returned.
     *
     * @detail This API is provided to support range queries.
     *
     * In mathematical terms the returned iterator points to the following item:
     * <code>
     * min( {x | y belongs to index AND x > y} )
     * </code>
     *
     * The returned iterator is guaranteed to be a forward iterator. Some index types may provide a
     * bidirectional iterator as well (see IIndexIterator). You may query the returned iterator to
     * find out whether it is bidirectional or not.
     *
     * @param key The key to search.
     * @param pid The logical identifier of the requesting thread.
     * @return An iterator to first item matching the first key after the searched key. The end
     * iterator may be returned if no items are matching. The end iterator is also returned if the
     * index does not support this API.
     */
    virtual IndexIterator* UpperBound(const Key* key, uint32_t pid) const;

    /**
     * @brief Retrieves a reverse iterator to the largest item that has a key smaller than or equal
     * to the specified key. Not all index types may support this API, in which case the end
     * iterator is always returned.
     *
     * @detail This API is provided to support range queries.
     *
     * In mathematical terms the returned iterator points to the following item:
     * <code>
     * max( {x | y belongs to index AND x <= y} )
     * </code>
     *
     * The returned iterator is guaranteed to be a reverse iterator. Some index types may provide a
     * bidirectional iterator as well (see IIndexIterator). You may query the returned iterator to
     * find out whether it is bidirectional or not (in addition to being a reverse iterator).
     *
     * @param key The key to search.
     * @param pid The logical identifier of the requesting thread.
     * @return A reverse iterator to item matching the search. The end iterator may be returned if
     * no items are matching. The end iterator is also returned if the index does not support this
     * API.
     */
    virtual IndexIterator* ReverseLowerBound(const Key* key, uint32_t pid) const;

    /**
     * @brief Retrieves a reverse iterator to the largest item that has a key smaller than the
     * specified key. Not all index types may support this API, in which case the end iterator is
     * always returned.
     *
     * @detail This API is provided to support range queries.
     *
     * In mathematical terms the returned iterator points to the following item:
     * <code>
     * max( {x | y belongs to index AND x < y} )
     * </code>
     *
     * The returned iterator is guaranteed to be a reverse iterator. Some index types may provide a
     * bidirectional iterator as well (see IIndexIterator). You may query the returned iterator to
     * find out whether it is bidirectional or not (in addition to being a reverse iterator).
     *
     * @param key The key to search.
     * @param pid The logical identifier of the requesting thread.
     * @return A reverse iterator to item matching the search. The end iterator may be returned if
     * no items are matching. The end iterator is also returned if the index does not support this
     * API.
     */
    virtual IndexIterator* ReverseUpperBound(const Key* key, uint32_t pid) const;

    // Key API
    /**
     * @brief Builds a key from a row according to index specification.
     * @param row The source row from which the key is to be built.
     * @param[out] destBuf The resulting key allocated on the appropriate pool.
     */
    virtual void BuildKey(Table* table, const Row* row, Key* key);

    /**
     * @brief Builds an error message for key from a row according to index specification.
     * @param row The source row from which the key is to be built.
     * @param[out] destBuf The resulting error message
     */
    virtual void BuildErrorMsg(Table* table, const Row* row, char* destBuf, size_t len);

protected:
    /**
     * @brief Derived classes should override this method to perform any further required
     * initialization.
     * @param args Null-terminated list of any additional arguments.
     * @return Return code denoting success or error.
     */
    virtual RC IndexInitImpl(void** args);

public:
    inline void SetLenghtKeyFields(const uint16_t& field, const int16_t& orgField, const uint16_t& length)
    {
        if (field >= MAX_KEY_COLUMNS) {
            return;
        }
        m_lengthKeyFields[field] = length;
        m_columnKeyFields[field] = orgField;
        if (m_colBitmap != nullptr && orgField > 0) {
            BITMAP_SET(m_colBitmap, orgField);
        }
    }

    inline uint16_t const* GetLengthKeyFields() const
    {
        return m_lengthKeyFields;
    }

    inline int16_t const* GetColumnKeyFields() const
    {
        return m_columnKeyFields;
    }

    inline int16_t GetNumFields() const
    {
        return m_numKeyFields;
    }

    inline void SetOrder(IndexOrder primary)
    {
        m_indexOrder = primary;
    }

    inline bool IsPrimaryKey() const
    {
        return (m_indexOrder == IndexOrder::INDEX_ORDER_PRIMARY);
    }

    inline void SetFakePrimary(bool fake)
    {
        m_fake = fake;
    }

    inline bool IsFakePrimary() const
    {
        return ((m_indexOrder == IndexOrder::INDEX_ORDER_PRIMARY && m_fake) ? true : false);
    }

    inline bool IsUnique() const
    {
        return m_unique;
    }

    inline void SetIsCommited(bool isCommited)
    {
        m_isCommited = isCommited;
    }

    inline bool GetIsCommited() const
    {
        return m_isCommited;
    }

    inline void SetSnapshot(uint64_t csn)
    {
        m_startCSN = csn;
    }

    inline uint64_t GetSnapshot() const
    {
        return m_startCSN;
    }

    inline bool SetNumTableFields(uint32_t num)
    {
        int bLen = BITMAP_GETLEN(num);
        int oldLen = BITMAP_GETLEN(m_numTableFields);
        uint8_t* tmp = nullptr;
        bool needAlloc = false;
        if (m_colBitmap != nullptr) {
            if (bLen != oldLen) {
                needAlloc = true;
            }
        } else {
            needAlloc = true;
        }
        if (needAlloc) {
            tmp = new (std::nothrow) uint8_t[bLen]{0};
            if (tmp == nullptr) {
                MOT_REPORT_ERROR(
                    MOT_ERROR_OOM, "Index Initialization", "Failed to allocate %u bytes for null bitmap", num);
                return false;
            }
            if (m_colBitmap != nullptr) {
                errno_t erc = memcpy_s(tmp, bLen, m_colBitmap, oldLen);
                securec_check(erc, "\0", "\0");
                delete[] m_colBitmap;
            }
            m_colBitmap = tmp;
        }
        m_numTableFields = num;
        return true;
    }

    inline void SetNumIndexFields(uint32_t num)
    {
        if (num > MAX_KEY_COLUMNS) {
            return;
        }
        m_numKeyFields = num;
    }

    inline bool IsFieldPresent(int16_t colid) const
    {
        return (m_colBitmap && BITMAP_GET(m_colBitmap, colid));
    }

    inline Key* CreateNewKey()
    {
        Key* key = m_keyPool->Alloc<Key>(m_keyLength, (IsPrimaryKey() ? KeyType::PRIMARY_KEY : KeyType::SECONDARY_KEY));
        if (key == nullptr) {
            MOT_REPORT_ERROR(
                MOT_ERROR_OOM, "Create Index Key", "Failed to allocate key in length %u", (unsigned)m_keyLength);
        }
        return key;
    }

    inline void DestroyKey(Key* key)
    {
        m_keyPool->Release<Key>(key);
    }

    inline void AdjustKey(Key* key, uint8_t pattern) const
    {
        if (!m_unique) {
            MOT_ASSERT(m_keyLength <= key->GetKeyLength());
            (void)key->FillPattern(pattern, NON_UNIQUE_INDEX_SUFFIX_LEN, m_keyLength - NON_UNIQUE_INDEX_SUFFIX_LEN);
        }
    }

    inline uint64_t GetExtId() const
    {
        return m_indexExtId;
    }

    inline void SetExtId(uint64_t extId)
    {
        m_indexExtId = extId;
    }

    inline Table* GetTable()
    {
        return m_table;
    }

    inline void SetTable(Table* table)
    {
        m_table = table;
    }

    inline Key* CreateNewSearchKey()
    {
        return m_keyPool->Alloc<Key>(m_keyLength, (IsPrimaryKey() ? KeyType::PRIMARY_KEY : KeyType::SECONDARY_KEY));
    }

    inline uint32_t GetKeyPoolSize() const
    {
        return m_keyPool->m_size;
    }

    inline uint32_t GetSentinelSizeFromPool() const
    {
        return m_sentinelPool->m_size;
    }

    inline uint32_t GetSecondarySentinelObjSizeFromPool() const
    {
        return m_sSentinelVersionPool->m_size;
    }

    inline void SetUnique(bool unique)
    {
        m_unique = unique;
    }

    inline bool GetUnique() const
    {
        return m_unique;
    }

    RC Truncate(bool isDrop);

    virtual void Compact(Table* table, uint32_t pid);

    virtual uint64_t GetIndexSize(uint64_t& netTotal);

    // Index API
    /**
     * @brief Inserts a row into the index.
     * @param key A pointer to the key.
     * @param row The row to insert. In case of a secondary index this should be null pointer.
     * before calling to indexInsert(). When inserting into a secondary index, this parameter should
     * be the result of a prior call to indexInsert() on a primary index.
     * @param pid The logical identifier of the requesting thread.
     * @param mem_ifc Interface for allocating NUMA-aware pooled objects.
     * @return A previously existing row already mapped to the specified key or null pointer if a new
     * row was inserted.
     */
    bool IndexInsert(Sentinel*& outputSentinel, const Key* key, uint32_t pid, RC& rc, bool isRecovery = false);
    Sentinel* IndexInsert(const Key* key, Row* row, uint32_t pid);

    /**
     * @brief Reads a single row from the index.
     * @param key A pointer to the key to search.
     * @param pid The logical identifier of the requesting thread.
     * @return Return The row or null pointer if not found.
     */
    Row* IndexRead(const Key* key, uint32_t pid) const;
    Sentinel* IndexReadHeader(const Key* key, uint32_t pid) const;

    /**
     * @brief Reads a single row from the index.
     * @param key A pointer to the key to search.
     * @param pid The logical identifier of the requesting thread.
     * @return Return The sentinel or null pointer if not found.
     */
    inline Sentinel* IndexReadSentinel(const Key* key, uint32_t pid)
    {
        return IndexReadImpl(key, pid);
    }

    /**
     * @brief Removes a single row from the index
     * @param key A pointer to the key.
     * @param key_length The length in bytes of the key.
     * @param pid The logical identifier of the requesting thread.
     * @return Return The row that was removed or null pointer if not found.
     */
    Sentinel* IndexRemove(const Key* key, uint32_t pid);
    Sentinel* IndexRemove(const Sentinel* sentinel, uint32_t pid);

    inline uint32_t GetIndexId() const
    {
        return m_indexId;
    }

protected:
    /** @var The length of the key in bytes. */
    uint32_t m_keyLength;

    /** @var The name of the index. */
    string m_name;

    /** @var The order of the index (primary or secondary). */
    IndexOrder m_indexOrder;

    /** @var The indexing method used by the index (hash or tree). */
    IndexingMethod m_indexingMethod;

    /** @var Specifies external index identifier. */
    uint64_t m_indexExtId;
    /** @var The creation snapshot of the index */
    uint64_t m_startCSN;

    /** @var Global atomic index identifier. */
    static std::atomic<uint32_t> m_indexCounter;

    uint32_t m_indexId;

    uint16_t m_lengthKeyFields[MAX_KEY_COLUMNS] = {0};
    int16_t m_columnKeyFields[MAX_KEY_COLUMNS] = {0};
    int16_t m_numKeyFields = 0;
    uint32_t m_numTableFields = 0;
    bool m_fake = false;
    bool m_isCommited = false;
    uint8_t* m_colBitmap = nullptr;
    bool m_unique = true;

    ObjAllocInterface* m_keyPool;
    ObjAllocInterface* m_sentinelPool;
    ObjAllocInterface* m_sSentinelVersionPool;

    Table* m_table;

    /**
     * @brief Inserts a single row into the actual data structure that implements the index.
     * @param key Pointer to the key.
     * @param sentinel The primary index sentinel that holds the row and the key.
     * @param[out] inserted Boolean value denoting whether the row was inserted (true) or a row with
     * the same key already existed (false).
     * @param pid The logical identifier of the requesting thread.
     * @return The sentinel object that is mapped to the given key. If the sentinel was inserted, then
     * null pointer is returned, otherwise the sentinel that was already mapped to the key is
     * returned.
     */
    virtual Sentinel* IndexInsertImpl(const Key* key, Sentinel* sentinel, bool& inserted, uint32_t pid) = 0;

    /**
     * @brief Reads a row from the actual data structure that implements the index.
     * @param key Pointer to the key.
     * @param pid The logical identifier of the requesting thread.
     * @return The primary index sentinel that was read or null pointer if not found.
     */
    virtual Sentinel* IndexReadImpl(const Key* key, uint32_t pid) const = 0;

    /**
     * @brief Removes a row from the actual data structure that implements the index.
     * @param key A pointer to the key.
     * @param pid The logical identifier of the requesting thread.
     * @return The primary index sentinel that was removed or null pointer if not found.
     */
    virtual Sentinel* IndexRemoveImpl(const Key* key, uint32_t pid) = 0;

    DECLARE_CLASS_LOGGER()
};
}  // namespace MOT

#endif /* MOT_INDEX_H */
