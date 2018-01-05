/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#ifndef SSDB_IMPL_H_
#define SSDB_IMPL_H_

#include <queue>
#include <atomic>
#include "ClientContext.hpp"

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/table.h>
#include <rocksdb/utilities/sim_cache.h>
#include <codec/rdb/RdbEncoder.h>


#include "util/log.h"
#include "util/config.h"
#include "util/PTimer.h"
#include "util/time.h"
#include "util/thread.h"
#include "codec/error.h"

#include "ssdb.h"
#include "iterator.h"

#include "codec/kv/decode.h"
#include "codec/kv/encode.h"

#include "ttl.h"
#include "t_cursor.h"
#include "t_scan.h"


const double ZSET_SCORE_MAX = std::numeric_limits<double>::max();
const double ZSET_SCORE_MIN = std::numeric_limits<double>::min();
const double eps = 1e-15;

class ExpirationHandler;

inline
static rocksdb::Slice slice(const std::string &b) {
    return rocksdb::Slice(b.data(), (size_t) b.size());
}

inline
static rocksdb::Slice slice(const Bytes &b) {
    return rocksdb::Slice(b.data(), (size_t) b.size());
}

inline
static rocksdb::Slice slice() {
    return rocksdb::Slice();
}

const static std::string REPOPID_CF = "repopid";

enum LIST_POSITION {
    HEAD,
    TAIL,
};

enum DIRECTION {
    FORWARD,
    BACKWARD,
};

typedef RecordLock<Mutex> RecordKeyLock;
typedef RecordMutex<Mutex> RecordKeyMutex;


class SSDBImpl : public SSDB {
private:
    friend class SSDB;

    friend class ExpirationHandler;

    std::string path;

    rocksdb::DB *ldb;
    rocksdb::Options options;
    rocksdb::ReadOptions commonRdOpt = rocksdb::ReadOptions();

    RedisCursorService redisCursorService;;

    SSDBImpl();

public:

    rocksdb::SimCache *simCache = nullptr;

    std::vector<rocksdb::ColumnFamilyHandle *> handles;

    rocksdb::DB *getLdb() const {
        return ldb;
    }

    const string &getPath() const {
        return path;
    }

    string getDataPath() const {
        return path + "/data/";
    }

    int save(ClientContext &ctx);

    ExpirationHandler *expiration;

    virtual ~SSDBImpl();

    virtual int flushdb(ClientContext &ctx);

    virtual int flush(ClientContext &ctx, bool wait = false);

    // return (start, end], not include start
    virtual Iterator *iterator(const std::string &start, const std::string &end, uint64_t limit,
                               const rocksdb::Snapshot *snapshot = nullptr);

    virtual Iterator *iterator(const std::string &start, const std::string &end, uint64_t limit,
                               const rocksdb::ReadOptions &options);


    virtual Iterator *rev_iterator(const std::string &start, const std::string &end, uint64_t limit,
                                   const rocksdb::Snapshot *snapshot = nullptr);

    virtual const rocksdb::Snapshot *GetSnapshotWithLock();

    virtual const rocksdb::Snapshot *GetSnapshot();

    virtual void ReleaseSnapshot(const rocksdb::Snapshot *snapshot);

    //void flushdb();
    virtual uint64_t size();

    virtual std::vector<std::string> info();

    virtual void compact();

    virtual int digest(std::string *val);

    virtual rocksdb::Status CommitBatch(ClientContext &ctx, rocksdb::WriteBatch *updates);

    virtual rocksdb::Status
    CommitBatch(ClientContext &ctx, const rocksdb::WriteOptions &options, rocksdb::WriteBatch *updates);

    /* raw operates */

    // repl: whether to sync this operation to slaves
    virtual int raw_set(ClientContext &ctx, const Bytes &key, const Bytes &val);

    virtual int raw_del(ClientContext &ctx, const Bytes &key);

    virtual int raw_get(ClientContext &ctx, const Bytes &key, std::string *val);

    virtual int raw_get(ClientContext &ctx, const Bytes &key, rocksdb::ColumnFamilyHandle *column_family, std::string *val);

    /* 	General	*/
    virtual int type(ClientContext &ctx, const Bytes &key, std::string *type);

    virtual int dump(ClientContext &ctx, const Bytes &key, std::string *res, int64_t *pttl, bool compress);

    virtual int rdbSaveObject(ClientContext &ctx, const Bytes &key, char dtype, const std::string &meta_val,
                              RdbEncoder &encoder, const rocksdb::Snapshot *snapshot);

    virtual int
    restore(ClientContext &ctx, const Bytes &key, int64_t expire, const Bytes &data, bool replace, std::string *res);

    virtual int exists(ClientContext &ctx, const Bytes &key);

    virtual int parse_replic(ClientContext &ctx, const std::vector<Bytes> &kvs);

    virtual int parse_replic(ClientContext &ctx, const std::vector<std::string> &kvs);

    /* key value */

    virtual int set(ClientContext &ctx, const Bytes &key, const Bytes &val, int flags, int64_t expire_ms, int *added);

    virtual int del(ClientContext &ctx, const Bytes &key);

    virtual int append(ClientContext &ctx, const Bytes &key, const Bytes &value, uint64_t *llen);

    // -1: error, 1: ok, 0: value is not an integer or out of range
    virtual int incr(ClientContext &ctx, const Bytes &key, int64_t by, int64_t *new_val);

    virtual int incrbyfloat(ClientContext &ctx, const Bytes &key, long double by, long double *new_val);

    virtual int multi_set(ClientContext &ctx, const std::vector<Bytes> &kvs, int offset);

    virtual int multi_del(ClientContext &ctx, const std::set<std::string> &keys, int64_t *count);

    virtual int setbit(ClientContext &ctx, const Bytes &key, int64_t bitoffset, int on, int *res);

    virtual int getbit(ClientContext &ctx, const Bytes &key, int64_t bitoffset, int *res);

    virtual int get(ClientContext &ctx, const Bytes &key, std::string *val);

    virtual int getset(ClientContext &ctx, const Bytes &key, std::pair<std::string, bool> &val, const Bytes &newval);

    virtual int getrange(ClientContext &ctx, const Bytes &key, int64_t start, int64_t end, std::pair<std::string, bool> &res);

    // return (start, end]
    virtual int setrange(ClientContext &ctx, const Bytes &key, int64_t start, const Bytes &value, uint64_t *new_len);

    virtual int scan(const Bytes &cursor, const std::string &pattern, uint64_t limit, std::vector<std::string> &resp);

    /* hash */

    virtual int hmset(ClientContext &ctx, const Bytes &name, const std::map<Bytes, Bytes> &kvs);

    virtual int hset(ClientContext &ctx, const Bytes &name, const Bytes &key, const Bytes &val, int *added);

    virtual int hsetnx(ClientContext &ctx, const Bytes &name, const Bytes &key, const Bytes &val, int *added);

    virtual int hdel(ClientContext &ctx, const Bytes &name, const std::set<Bytes> &fields, int *deleted);

    // -1: error, 1: ok, 0: value is not an integer or out of range
    virtual int hincr(ClientContext &ctx, const Bytes &name, const Bytes &key, int64_t by, int64_t *new_val);

    virtual int hincrbyfloat(ClientContext &ctx, const Bytes &name, const Bytes &key, long double by, long double *new_val);
    //int multi_hset(Context &ctx, const Bytes &name,const std::vector<Bytes> &kvs, int offset=0);
    //int multi_hdel(Context &ctx, const Bytes &name,const std::vector<Bytes> &keys, int offset=0);

    virtual int hsize(ClientContext &ctx, const Bytes &name, uint64_t *size);

    virtual int hmget(ClientContext &ctx, const Bytes &name, const std::vector<std::string> &reqKeys,
                      std::map<std::string, std::string> &val);

    virtual int hgetall(ClientContext &ctx, const Bytes &name, std::vector<std::string> &list,
                        bool save_key, bool save_value);

    virtual int hget(ClientContext &ctx, const Bytes &name, const Bytes &key, std::pair<std::string, bool> &val);

//	virtual HIterator* hscan(Context &ctx, const Bytes &name,const Bytes &start, const Bytes &end, uint64_t limit);
    virtual int hscan(ClientContext &ctx, const Bytes &name, const Bytes &cursor, const std::string &pattern, uint64_t limit,
                      std::vector<std::string> &resp);


    /*  list  */
    virtual int LIndex(ClientContext &ctx, const Bytes &key, int64_t index, std::pair<std::string, bool> &val);

    virtual int LLen(ClientContext &ctx, const Bytes &key, uint64_t *llen);

    virtual int LPop(ClientContext &ctx, const Bytes &key, std::pair<std::string, bool> &val);

    virtual int LPush(ClientContext &ctx, const Bytes &key, const std::vector<Bytes> &val, int offset, uint64_t *llen);

    virtual int LPushX(ClientContext &ctx, const Bytes &key, const std::vector<Bytes> &val, int offset, uint64_t *llen);

    virtual int RPop(ClientContext &ctx, const Bytes &key, std::pair<std::string, bool> &val);

    virtual int RPush(ClientContext &ctx, const Bytes &key, const std::vector<Bytes> &val, int offset, uint64_t *llen);

    virtual int RPushX(ClientContext &ctx, const Bytes &key, const std::vector<Bytes> &val, int offset, uint64_t *llen);

    virtual int LSet(ClientContext &ctx, const Bytes &key, int64_t index, const Bytes &val);

    virtual int lrange(ClientContext &ctx, const Bytes &key, int64_t start, int64_t end, std::vector<std::string> &list);

    virtual int ltrim(ClientContext &ctx, const Bytes &key, int64_t start, int64_t end);


    /* set */
    virtual int sadd(ClientContext &ctx, const Bytes &key, const std::set<Bytes> &members, int64_t *num);

    virtual int srem(ClientContext &ctx, const Bytes &key, const std::vector<Bytes> &members, int64_t *num);

    virtual int scard(ClientContext &ctx, const Bytes &key, uint64_t *llen);

    virtual int sismember(ClientContext &ctx, const Bytes &key, const Bytes &member, bool *ismember);

    virtual int smembers(ClientContext &ctx, const Bytes &key, std::vector<std::string> &members);

    virtual int spop(ClientContext &ctx, const Bytes &key, std::vector<std::string> &members, int64_t popcnt);

    virtual int srandmember(ClientContext &ctx, const Bytes &key, std::vector<std::string> &members, int64_t cnt);

    virtual int sscan(ClientContext &ctx, const Bytes &name, const Bytes &cursor, const std::string &pattern, uint64_t limit,
                      std::vector<std::string> &resp);

    /* zset */
    virtual int
    multi_zset(ClientContext &ctx, const Bytes &name, const std::map<Bytes, Bytes> &sortedSet, int flags, int64_t *count);

    virtual int multi_zdel(ClientContext &ctx, const Bytes &name, const std::set<Bytes> &keys, int64_t *count);

    // -1: error, 1: ok, 0: value is not an integer or out of range
    virtual int zincr(ClientContext &ctx, const Bytes &name, const Bytes &key, double by, int &flags, double *new_val);
    //int multi_zset(Context &ctx, const Bytes &name,const std::vector<Bytes> &kvs, int offset=0);
    //int multi_zdel(Context &ctx, const Bytes &name,const std::vector<Bytes> &keys, int offset=0);

    int zremrangebyscore(ClientContext &ctx, const Bytes &name, const Bytes &score_start, const Bytes &score_end, bool remove,
                         int64_t *count);

    virtual int zsize(ClientContext &ctx, const Bytes &name, uint64_t *size);

    /**
     * @return -1: error; 0: not found; 1: found
     */
    virtual int zget(ClientContext &ctx, const Bytes &name, const Bytes &key, double *score);

    virtual int zrank(ClientContext &ctx, const Bytes &name, const Bytes &key, int64_t *rank);

    virtual int zrrank(ClientContext &ctx, const Bytes &name, const Bytes &key, int64_t *rank);

    virtual int zrange(ClientContext &ctx, const Bytes &name, const Bytes &begin, const Bytes &limit, bool withscore,
                       std::vector<std::string> &key_score);

    virtual int zrrange(ClientContext &ctx, const Bytes &name, const Bytes &begin, const Bytes &limit, bool withscore,
                        std::vector<std::string> &key_score);

    virtual int zrangebyscore(ClientContext &ctx, const Bytes &name, const Bytes &start_score, const Bytes &end_score,
                              std::vector<std::string> &key_score,
                              int withscores, long offset, long limit);

    virtual int zrevrangebyscore(ClientContext &ctx, const Bytes &name, const Bytes &start_score, const Bytes &end_score,
                                 std::vector<std::string> &key_score,
                                 int withscores, long offset, long limit);

    /**
     * scan by score, but won't return @key if key.score=score_start.
     * return (score_start, score_end]
     */
    virtual int zscan(ClientContext &ctx, const Bytes &name, const Bytes &cursor, const std::string &pattern, uint64_t limit,
                      std::vector<std::string> &resp);

    virtual int
    zlexcount(ClientContext &ctx, const Bytes &name, const Bytes &key_start, const Bytes &key_end, int64_t *count);

    virtual int zrangebylex(ClientContext &ctx, const Bytes &name, const Bytes &key_start, const Bytes &key_end,
                            std::vector<std::string> &keys,
                            long offset, long limit);

    virtual int zrevrangebylex(ClientContext &ctx, const Bytes &name, const Bytes &key_start, const Bytes &key_end,
                               std::vector<std::string> &keys,
                               long offset, long limit);

    virtual int
    zremrangebylex(ClientContext &ctx, const Bytes &name, const Bytes &key_start, const Bytes &key_end, int64_t *count);


    /* eset */
    virtual int eget(ClientContext &ctx, const Bytes &key, int64_t *ts);

    virtual int eset_one(ClientContext &ctx, const Bytes &key, rocksdb::WriteBatch &batch, int64_t ts_ms);

    virtual int edel_one(ClientContext &ctx, const Bytes &key, rocksdb::WriteBatch &batch);

    virtual int check_meta_key(ClientContext &ctx, const Bytes &key);

    virtual int redisCursorCleanup();

private:

    int SetGeneric(ClientContext &ctx, const Bytes &key, rocksdb::WriteBatch &batch, const Bytes &val, int flags,
                   int64_t expire_ms, int *added);

    int GetKvMetaVal(const std::string &meta_key, KvMetaVal &kv);

    int del_key_internal(ClientContext &ctx, const Bytes &key, rocksdb::WriteBatch &batch);

    int mark_key_deleted(ClientContext &ctx, const Bytes &key, rocksdb::WriteBatch &batch, const std::string &meta_key,
                         std::string &meta_val);


    int GetHashMetaVal(const std::string &meta_key, HashMetaVal &hv);

    int GetHashItemValInternal(const std::string &item_key, std::string *val);

    HIterator *
    hscan_internal(ClientContext &ctx, const Bytes &name, uint16_t version, const rocksdb::Snapshot *snapshot = nullptr);

    int incr_hsize(ClientContext &ctx, const Bytes &name, rocksdb::WriteBatch &batch, const std::string &size_key,
                   HashMetaVal &hv, int64_t incr);

    int
    hset_one(rocksdb::WriteBatch &batch, const HashMetaVal &hv, bool check_exists, const Bytes &name, const Bytes &key,
             const Bytes &val);

    int GetSetMetaVal(const std::string &meta_key, SetMetaVal &sv);

    int GetSetItemValInternal(const std::string &item_key);

    SIterator *
    sscan_internal(ClientContext &ctx, const Bytes &name, uint16_t version, const rocksdb::Snapshot *snapshot = nullptr);

    int incr_ssize(ClientContext &ctx, const Bytes &key, rocksdb::WriteBatch &batch, const SetMetaVal &sv,
                   const std::string &meta_key, int64_t incr);

    int GetListItemValInternal(const std::string &item_key, std::string *val,
                               const rocksdb::ReadOptions &options = rocksdb::ReadOptions());

    int GetListMetaVal(const std::string &meta_key, ListMetaVal &lv);

    int doListPop(ClientContext &ctx, const Bytes &key, rocksdb::WriteBatch &batch, ListMetaVal &lv, std::string &meta_key,
                  LIST_POSITION lp, std::pair<std::string, bool> &val);

    template<typename T>
    int doListPush(ClientContext &ctx, const Bytes &key, rocksdb::WriteBatch &batch, const std::vector<T> &val, int offset,
                   std::string &meta_key, ListMetaVal &meta_val, LIST_POSITION lp);

    int GetZSetMetaVal(const std::string &meta_key, ZSetMetaVal &zv);

    int GetZSetItemVal(const std::string &item_key, double *score);

    ZIterator *zscan_internal(ClientContext &ctx, const Bytes &name, const Bytes &score_start, const Bytes &score_end,
                              uint64_t limit, Iterator::Direction direction, uint16_t version,
                              const rocksdb::Snapshot *snapshot = nullptr);

    ZIteratorByLex *zscanbylex_internal(ClientContext &ctx, const Bytes &name, const Bytes &key_start, const Bytes &key_end,
                                        uint64_t limit, Iterator::Direction direction, uint16_t version,
                                        const rocksdb::Snapshot *snapshot = nullptr);

    int zset_one(rocksdb::WriteBatch &batch, bool needCheck, const Bytes &name, const Bytes &key, double score,
                 uint16_t cur_version, int *flags, double *newscore);

    int zdel_one(rocksdb::WriteBatch &batch, const Bytes &name, const Bytes &key, uint16_t version);

    int incr_zsize(ClientContext &ctx, const Bytes &name, rocksdb::WriteBatch &batch, const ZSetMetaVal &zv, int64_t incr);

    int setNoLock(ClientContext &ctx, const Bytes &key, const Bytes &val, int flags, int64_t expire_ms, int *added);

    template<typename T>
    int saddNoLock(ClientContext &ctx, const Bytes &key, const std::set<T> &mem_set, int64_t *num);

    template<typename T>
    int hmsetNoLock(ClientContext &ctx, const Bytes &name, const std::map<T, T> &kvs, bool check_exists);

    template<typename T>
    int zsetNoLock(ClientContext &ctx, const Bytes &name, const std::map<T, T> &sortedSet, int flags, int64_t *num);

    int zdelNoLock(ClientContext &ctx, const Bytes &name, const std::set<Bytes> &keys, int64_t *count);


    int zrangeGeneric(ClientContext &ctx, const Bytes &name, const Bytes &begin, const Bytes &limit, bool withscore,
                      std::vector<string> &key_score, int reverse);

    int genericZrangebyscore(ClientContext &ctx, const Bytes &name, const Bytes &start_score, const Bytes &end_score,
                             std::vector<std::string> &key_score,
                             int withscores, long offset, long limit, int reverse);

    int genericZrangebylex(ClientContext &ctx, const Bytes &name, const Bytes &key_start, const Bytes &key_end,
                           std::vector<string> &keys,
                           long offset, long limit, int save, int64_t *count);

    template<typename L>
    int updateKvCommon(ClientContext &ctx, const Bytes &name, L lambda);

    template<typename L>
    int hincrCommon(ClientContext &ctx, const Bytes &name, const Bytes &key, L lambda);

    int hsetCommon(ClientContext &ctx, const Bytes &name, const Bytes &key, const Bytes &val, int *added, bool nx);


    int quickKv(ClientContext &ctx, const Bytes &key, const Bytes &val, const std::string &meta_key,
                const std::string &old_meta_val, int64_t expire_ms);

    template<typename T>
    int quickSet(ClientContext &ctx, const Bytes &key, const std::string &meta_key, const std::string &meta_val, T lambda);

    template<typename T>
    int quickHash(ClientContext &ctx, const Bytes &key, const std::string &meta_key, const std::string &meta_val, T lambda);

    template<typename T>
    int quickZset(ClientContext &ctx, const Bytes &key, const std::string &meta_key, const std::string &meta_val, T lambda);

    template<typename T>
    int quickList(ClientContext &ctx, const Bytes &key, const std::string &meta_key, const std::string &meta_val, T lambda);

private:
    //    pthread_mutex_t mutex_bgtask_;
    Mutex mutex_bgtask_;
    Mutex mutex_backup_;
    std::atomic<bool> bgtask_quit;
    pthread_t bg_tid_;
    std::queue<std::string> tasks_;


    void load_delete_keys_from_db(int num);

    void delete_key_loop(const std::string &del_key);

    int delete_meta_key(const DeleteKey &dk, rocksdb::WriteBatch &batch);

    void runBGTask();

    static void *thread_func(void *arg);

    RecordKeyMutex mutex_record_;

public:

    void start();

    void stop();
};

uint64_t getSeqByIndex(int64_t index, const ListMetaVal &meta_val);


class SnapshotPtr {
private:

public:
    SnapshotPtr(rocksdb::DB *ldb, const rocksdb::Snapshot *snapshot) : ldb(ldb), snapshot(snapshot) {}

    virtual ~SnapshotPtr() {
        if (snapshot != nullptr) {
            ldb->ReleaseSnapshot(snapshot);
        }
    }

    rocksdb::DB *ldb;
    const rocksdb::Snapshot *snapshot;

};


#endif
