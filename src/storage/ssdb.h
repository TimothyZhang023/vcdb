/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#ifndef SSDB_H_
#define SSDB_H_

#include <vector>
#include <set>
#include <string>
#include <map>
#include <memory>

class Bytes;

class Config;

class Iterator;

class ClientContext;

class Options;

namespace rocksdb {
    class Snapshot;
}


class SSDB {
public:
    SSDB() {}

    virtual ~SSDB() {};

    static SSDB *open(const Options &opt, const std::string &base_dir);

    virtual int flushdb(ClientContext &ctx) = 0;

    // return (start, end], not include start
    virtual Iterator *iterator(const std::string &start, const std::string &end, uint64_t limit,
                               const rocksdb::Snapshot *snapshot = nullptr) = 0;

    virtual Iterator *rev_iterator(const std::string &start, const std::string &end, uint64_t limit,
                                   const rocksdb::Snapshot *snapshot = nullptr) = 0;

    //void flushdb();
    virtual uint64_t size() = 0;

    virtual std::vector<std::string> info() = 0;

    virtual void compact() = 0;

    virtual int digest(std::string *val) = 0;

    /* raw operates */

    // repl: whether to sync this operation to slaves

    virtual int raw_set(ClientContext &ctx, const Bytes &key, const Bytes &val) = 0;

    virtual int raw_del(ClientContext &ctx, const Bytes &key) = 0;

    virtual int raw_get(ClientContext &ctx, const Bytes &key, std::string *val) = 0;

    /* 	General	*/
    virtual int type(ClientContext &ctx, const Bytes &key, std::string *type) = 0;

    virtual int dump(ClientContext &ctx, const Bytes &key, std::string *res, int64_t *pttl, bool compress) = 0;

    virtual int
    restore(ClientContext &ctx, const Bytes &key, int64_t expire, const Bytes &data, bool replace, std::string *res) = 0;

    virtual int exists(ClientContext &ctx, const Bytes &key) = 0;

    virtual int parse_replic(ClientContext &ctx, const std::vector<Bytes> &kvs) = 0;

    virtual int parse_replic(ClientContext &ctx, const std::vector<std::string> &kvs) = 0;

    /* key value */

    virtual int set(ClientContext &ctx, const Bytes &key, const Bytes &val, int flags, int64_t expire_ms, int *added) = 0;

    virtual int del(ClientContext &ctx, const Bytes &key) = 0;

    virtual int append(ClientContext &ctx, const Bytes &key, const Bytes &value, uint64_t *new_len) = 0;

    // -1: error, 1: ok, 0: value is not an integer or out of range
    virtual int incr(ClientContext &ctx, const Bytes &key, int64_t by, int64_t *new_val) = 0;

    virtual int incrbyfloat(ClientContext &ctx, const Bytes &key, long double by, long double *new_val) = 0;

    virtual int multi_set(ClientContext &ctx, const std::vector<Bytes> &kvs, int offset = 0) = 0;

    virtual int multi_del(ClientContext &ctx, const std::set<std::string> &keys, int64_t *count) = 0;

    virtual int setbit(ClientContext &ctx, const Bytes &key, int64_t bitoffset, int on, int *res) = 0;

    virtual int getbit(ClientContext &ctx, const Bytes &key, int64_t bitoffset, int *res) = 0;

    virtual int get(ClientContext &ctx, const Bytes &key, std::string *val) = 0;

    virtual int getset(ClientContext &ctx, const Bytes &key, std::pair<std::string, bool> &val, const Bytes &newval) = 0;

    virtual int
    getrange(ClientContext &ctx, const Bytes &key, int64_t start, int64_t end, std::pair<std::string, bool> &res) = 0;

    virtual int setrange(ClientContext &ctx, const Bytes &key, int64_t start, const Bytes &value, uint64_t *new_len) = 0;

    virtual int
    scan(const Bytes &cursor, const std::string &pattern, uint64_t limit, std::vector<std::string> &resp) = 0;

    // return (start, end]

    /* hash */

    virtual int hmset(ClientContext &ctx, const Bytes &name, const std::map<Bytes, Bytes> &kvs) = 0;

    virtual int hset(ClientContext &ctx, const Bytes &name, const Bytes &key, const Bytes &val, int *added) = 0;

    virtual int hsetnx(ClientContext &ctx, const Bytes &name, const Bytes &key, const Bytes &val, int *added) = 0;

    virtual int hdel(ClientContext &ctx, const Bytes &name, const std::set<Bytes> &fields, int *deleted) = 0;

    // -1: error, 1: ok, 0: value is not an integer or out of range
    virtual int hincr(ClientContext &ctx, const Bytes &name, const Bytes &key, int64_t by, int64_t *new_val) = 0;

    virtual int
    hincrbyfloat(ClientContext &ctx, const Bytes &name, const Bytes &key, long double by, long double *new_val) = 0;

    virtual int hsize(ClientContext &ctx, const Bytes &name, uint64_t *size) = 0;

    virtual int hget(ClientContext &ctx, const Bytes &name, const Bytes &key, std::pair<std::string, bool> &val) = 0;

    virtual int hgetall(ClientContext &ctx, const Bytes &name, std::vector<std::string> &list,
                        bool save_key = true, bool save_value = true) = 0;

    virtual int hmget(ClientContext &ctx, const Bytes &name, const std::vector<std::string> &reqKeys,
                      std::map<std::string, std::string> &val) = 0;

    virtual int hscan(ClientContext &ctx, const Bytes &name, const Bytes &cursor, const std::string &pattern, uint64_t limit,
                      std::vector<std::string> &resp) = 0;

    /*  list  */
    virtual int LIndex(ClientContext &ctx, const Bytes &key, int64_t index, std::pair<std::string, bool> &val) = 0;

    virtual int LLen(ClientContext &ctx, const Bytes &key, uint64_t *llen) = 0;

    virtual int LPop(ClientContext &ctx, const Bytes &key, std::pair<std::string, bool> &val) = 0;

    virtual int LPush(ClientContext &ctx, const Bytes &key, const std::vector<Bytes> &val, int offset, uint64_t *llen) = 0;

    virtual int LPushX(ClientContext &ctx, const Bytes &key, const std::vector<Bytes> &val, int offset, uint64_t *llen) = 0;

    virtual int RPop(ClientContext &ctx, const Bytes &key, std::pair<std::string, bool> &val) = 0;

    virtual int RPush(ClientContext &ctx, const Bytes &key, const std::vector<Bytes> &val, int offset, uint64_t *llen) = 0;

    virtual int RPushX(ClientContext &ctx, const Bytes &key, const std::vector<Bytes> &val, int offset, uint64_t *llen) = 0;

    virtual int LSet(ClientContext &ctx, const Bytes &key, int64_t index, const Bytes &val) = 0;

    virtual int lrange(ClientContext &ctx, const Bytes &key, int64_t start, int64_t end, std::vector<std::string> &list) = 0;

    virtual int ltrim(ClientContext &ctx, const Bytes &key, int64_t start, int64_t end) = 0;

    /* set */
    virtual int sadd(ClientContext &ctx, const Bytes &key, const std::set<Bytes> &members, int64_t *num) = 0;

    virtual int srem(ClientContext &ctx, const Bytes &key, const std::vector<Bytes> &members, int64_t *num) = 0;

    virtual int scard(ClientContext &ctx, const Bytes &key, uint64_t *llen) = 0;

    virtual int sismember(ClientContext &ctx, const Bytes &key, const Bytes &member, bool *ismember) = 0;

    virtual int smembers(ClientContext &ctx, const Bytes &key, std::vector<std::string> &members) = 0;

    virtual int spop(ClientContext &ctx, const Bytes &key, std::vector<std::string> &members, int64_t popcnt) = 0;

    virtual int srandmember(ClientContext &ctx, const Bytes &key, std::vector<std::string> &members, int64_t cnt) = 0;

    virtual int sscan(ClientContext &ctx, const Bytes &name, const Bytes &cursor, const std::string &pattern, uint64_t limit,
                      std::vector<std::string> &resp) = 0;

    /* zset */
    virtual int
    multi_zset(ClientContext &ctx, const Bytes &name, const std::map<Bytes, Bytes> &sortedSet, int flags, int64_t *num) = 0;

    virtual int multi_zdel(ClientContext &ctx, const Bytes &name, const std::set<Bytes> &keys, int64_t *count) = 0;

    // -1: error, 1: ok, 0: value is not an integer or out of range
    virtual int zincr(ClientContext &ctx, const Bytes &name, const Bytes &key, double by, int &flags, double *new_val) = 0;

    virtual int zsize(ClientContext &ctx, const Bytes &name, uint64_t *size) = 0;

    /**
	 * @return -1: error; 0: not found; 1: found
	 */
    virtual int zget(ClientContext &ctx, const Bytes &name, const Bytes &key, double *score) = 0;

    virtual int zrank(ClientContext &ctx, const Bytes &name, const Bytes &key, int64_t *rank) = 0;

    virtual int zrrank(ClientContext &ctx, const Bytes &name, const Bytes &key, int64_t *rank) = 0;

    virtual int zrange(ClientContext &ctx, const Bytes &name, const Bytes &begin, const Bytes &limit, bool withscore,
                       std::vector<std::string> &key_score) = 0;

    virtual int zrrange(ClientContext &ctx, const Bytes &name, const Bytes &begin, const Bytes &limit, bool withscore,
                        std::vector<std::string> &key_score) = 0;

    virtual int zrangebyscore(ClientContext &ctx, const Bytes &name, const Bytes &start_score, const Bytes &end_score,
                              std::vector<std::string> &key_score,
                              int withscores, long offset, long limit) = 0;

    virtual int zrevrangebyscore(ClientContext &ctx, const Bytes &name, const Bytes &start_score, const Bytes &end_score,
                                 std::vector<std::string> &key_score,
                                 int withscores, long offset, long limit) = 0;

    /**
     * scan by score, but won't return @key if key.score=score_start.
     * return (score_start, score_end]
     */
    virtual int zscan(ClientContext &ctx, const Bytes &name, const Bytes &cursor, const std::string &pattern, uint64_t limit,
                      std::vector<std::string> &resp) = 0;

    virtual int
    zlexcount(ClientContext &ctx, const Bytes &name, const Bytes &key_start, const Bytes &key_end, int64_t *count) = 0;

    virtual int zrangebylex(ClientContext &ctx, const Bytes &name, const Bytes &key_start, const Bytes &key_end,
                            std::vector<std::string> &keys,
                            long offset, long limit) = 0;

    virtual int zrevrangebylex(ClientContext &ctx, const Bytes &name, const Bytes &key_start, const Bytes &key_end,
                               std::vector<std::string> &keys,
                               long offset, long limit) = 0;

    virtual int
    zremrangebylex(ClientContext &ctx, const Bytes &name, const Bytes &key_start, const Bytes &key_end, int64_t *count) = 0;

    /* eset */
    virtual int check_meta_key(ClientContext &ctx, const Bytes &key) = 0;

    virtual int redisCursorCleanup() = 0;

};

#endif
