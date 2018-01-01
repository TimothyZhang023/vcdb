//
// Created by zts on 12/18/17.
//

#ifndef VCDB_REDIS_CVT_H
#define VCDB_REDIS_CVT_H

#include <string>
#include <unordered_map>
#include <vector>
#include <swapdb/util/bytes.h>
#include <swapdb/serv.h>

namespace vcdb {

    enum REPLY {
        REPLY_BULK = 0,
        REPLY_MULTI_BULK,
        REPLY_INT,
        REPLY_OK_STATUS,
        REPLY_CUSTOM_STATUS,
        REPLY_SET_STATUS,
        REPLY_SPOP_SRANDMEMBER,
        REPLY_SCAN,
        REPLY_INFO,
    };

    enum STRATEGY {
        STRATEGY_AUTO,
        STRATEGY_PING,
        STRATEGY_MGET,
        STRATEGY_HMGET,
        STRATEGY_HGETALL,
        STRATEGY_HKEYS,
        STRATEGY_HVALS,
        STRATEGY_SETEX,
        STRATEGY_ZRANGE,
        STRATEGY_ZREVRANGE,
        STRATEGY_ZRANGEBYSCORE,
        STRATEGY_ZREVRANGEBYSCORE,
        STRATEGY_ZINCRBY,
        STRATEGY_REMRANGEBYRANK,
        STRATEGY_REMRANGEBYSCORE,
        STRATEGY_NULL
    };

    static bool inited = false;


    struct RedisRequestDesc {
        int strategy;
        std::string redis_cmd;
        std::string ssdb_cmd;
        int reply_type;
    };


    typedef std::unordered_map<std::string, RedisRequestDesc> RedisRequestConvertTable;
    static RedisRequestConvertTable cmd_table;

    struct RedisCommand_raw {
        int strategy;
        const char *redis_cmd;
        const char *ssdb_cmd;
        int reply_type;
    };

    static RedisCommand_raw cmds_raw[] = {
            {STRATEGY_PING,            "ping",             "ping",             REPLY_OK_STATUS},
            {STRATEGY_AUTO,            "info",             "info",             REPLY_INFO},

            {STRATEGY_AUTO,            "dump",             "dump",             REPLY_BULK},
            {STRATEGY_AUTO,            "restore",          "restore",          REPLY_OK_STATUS},
            {STRATEGY_AUTO,            "restore-asking",   "restore",          REPLY_OK_STATUS},

            {STRATEGY_AUTO,            "select",           "select",           REPLY_OK_STATUS},
            {STRATEGY_AUTO,            "migrate",          "migrate",          REPLY_OK_STATUS},
            {STRATEGY_AUTO,            "client",           "client",           REPLY_OK_STATUS},
            {STRATEGY_AUTO,            "quit",             "quit",             REPLY_OK_STATUS},
            {STRATEGY_AUTO,            "debug",            "debug",            REPLY_CUSTOM_STATUS},
            {STRATEGY_AUTO,            "flushall",         "flushdb",          REPLY_OK_STATUS},
            {STRATEGY_AUTO,            "flushdb",          "flushdb",          REPLY_OK_STATUS},
            {STRATEGY_AUTO,            "flush",            "flush",            REPLY_OK_STATUS},

            {STRATEGY_AUTO,            "filesize",         "filesize",         REPLY_INT},
            {STRATEGY_AUTO,            "dbsize",           "dbsize",           REPLY_INT},

            {STRATEGY_AUTO,            "scan",             "scan",             REPLY_SCAN},
            {STRATEGY_AUTO,            "type",             "type",             REPLY_CUSTOM_STATUS},
            {STRATEGY_AUTO,            "get",              "get",              REPLY_BULK},
            {STRATEGY_AUTO,            "getset",           "getset",           REPLY_BULK},
            {STRATEGY_AUTO,            "set",              "set",              REPLY_SET_STATUS},
            {STRATEGY_AUTO,            "setnx",            "setnx",            REPLY_INT},
            {STRATEGY_MGET,            "mget",             "multi_get",        REPLY_MULTI_BULK},
            {STRATEGY_SETEX,           "setex",            "setx",             REPLY_OK_STATUS},
            {STRATEGY_SETEX,           "psetex",           "psetx",            REPLY_OK_STATUS},
            {STRATEGY_AUTO,            "exists",           "exists",           REPLY_INT},
            {STRATEGY_AUTO,            "incr",             "incr",             REPLY_INT},
            {STRATEGY_AUTO,            "decr",             "decr",             REPLY_INT},
            {STRATEGY_AUTO,            "ttl",              "ttl",              REPLY_INT},
            {STRATEGY_AUTO,            "pttl",             "pttl",             REPLY_INT},
            {STRATEGY_AUTO,            "expire",           "expire",           REPLY_INT},
            {STRATEGY_AUTO,            "pexpire",          "pexpire",          REPLY_INT},
            {STRATEGY_AUTO,            "expireat",         "expireat",         REPLY_INT},
            {STRATEGY_AUTO,            "pexpireat",        "pexpireat",        REPLY_INT},
            {STRATEGY_AUTO,            "persist",          "persist",          REPLY_INT},
            {STRATEGY_AUTO,            "append",           "append",           REPLY_INT},
            {STRATEGY_AUTO,            "getbit",           "getbit",           REPLY_INT},
            {STRATEGY_AUTO,            "setbit",           "setbit",           REPLY_INT},
            {STRATEGY_AUTO,            "strlen",           "strlen",           REPLY_INT},
            {STRATEGY_AUTO,            "bitcount",         "bitcount",         REPLY_INT},
            {STRATEGY_AUTO,            "substr",           "getrange",         REPLY_BULK},
            {STRATEGY_AUTO,            "getrange",         "getrange",         REPLY_BULK},
            {STRATEGY_AUTO,            "setrange",         "setrange",         REPLY_INT},
            {STRATEGY_AUTO,            "keys",             "keys",             REPLY_MULTI_BULK},
            {STRATEGY_AUTO,            "del",              "multi_del",        REPLY_INT},
            {STRATEGY_AUTO,            "mset",             "multi_set",        REPLY_OK_STATUS},
            {STRATEGY_AUTO,            "incrby",           "incr",             REPLY_INT},
            {STRATEGY_AUTO,            "incrbyfloat",      "incrbyfloat",      REPLY_BULK},
            {STRATEGY_AUTO,            "decrby",           "decr",             REPLY_INT},


            {STRATEGY_AUTO,            "hdel",             "hdel",             REPLY_INT},
            {STRATEGY_AUTO,            "hexists",          "hexists",          REPLY_INT},
            {STRATEGY_AUTO,            "hget",             "hget",             REPLY_BULK},
            {STRATEGY_HGETALL,         "hgetall",          "hgetall",          REPLY_MULTI_BULK},
            {STRATEGY_AUTO,            "hincrby",          "hincr",            REPLY_INT},
            {STRATEGY_AUTO,            "hincrbyfloat",     "hincrbyfloat",     REPLY_BULK},
            {STRATEGY_HKEYS,           "hkeys",            "hkeys",            REPLY_MULTI_BULK},
            {STRATEGY_AUTO,            "hlen",             "hsize",            REPLY_INT},
            {STRATEGY_HMGET,           "hmget",            "hmget",            REPLY_MULTI_BULK},
            {STRATEGY_AUTO,            "hmset",            "hmset",            REPLY_OK_STATUS},
            {STRATEGY_AUTO,            "hset",             "hset",             REPLY_INT},
            {STRATEGY_AUTO,            "hsetnx",           "hsetnx",           REPLY_INT},
            //TODO HSTRLEN since 3.2
            {STRATEGY_HVALS,           "hvals",            "hvals",            REPLY_MULTI_BULK},
            {STRATEGY_AUTO,            "hscan",            "hscan",            REPLY_SCAN},

            {STRATEGY_AUTO,            "sadd",             "sadd",             REPLY_INT},
            {STRATEGY_AUTO,            "srem",             "srem",             REPLY_INT},
            {STRATEGY_AUTO,            "scard",            "scard",            REPLY_INT},

            {STRATEGY_AUTO,            "sismember",        "sismember",        REPLY_INT},
            {STRATEGY_AUTO,            "smembers",         "smembers",         REPLY_MULTI_BULK},
            {STRATEGY_AUTO,            "spop",             "spop",             REPLY_SPOP_SRANDMEMBER},
            {STRATEGY_AUTO,            "srandmember",      "srandmember",      REPLY_SPOP_SRANDMEMBER},
            {STRATEGY_AUTO,            "sscan",            "sscan",            REPLY_SCAN},

            {STRATEGY_AUTO,            "zcard",            "zsize",            REPLY_INT},
            {STRATEGY_AUTO,            "zscore",           "zget",             REPLY_BULK},
            {STRATEGY_AUTO,            "zrem",             "multi_zdel",       REPLY_INT},
            {STRATEGY_AUTO,            "zrank",            "zrank",            REPLY_INT},
            {STRATEGY_AUTO,            "zrevrank",         "zrrank",           REPLY_INT},
            {STRATEGY_AUTO,            "zcount",           "zcount",           REPLY_INT},
            {STRATEGY_REMRANGEBYRANK,  "zremrangebyrank",  "zremrangebyrank",  REPLY_INT},
            {STRATEGY_REMRANGEBYSCORE, "zremrangebyscore", "zremrangebyscore", REPLY_INT},
            {STRATEGY_ZRANGE,          "zrange",           "zrange",           REPLY_MULTI_BULK},
            {STRATEGY_ZREVRANGE,       "zrevrange",        "zrrange",          REPLY_MULTI_BULK},
            {STRATEGY_AUTO,            "zadd",             "multi_zset",       REPLY_INT},
            {STRATEGY_ZINCRBY,         "zincrby",          "zincr",            REPLY_BULK},
//	{STRATEGY_ZRANGEBYSCORE,	"zrangebyscore",	"zscan",	REPLY_MULTI_BULK},
//	{STRATEGY_ZREVRANGEBYSCORE,	"zrevrangebyscore",	"zrscan",	REPLY_MULTI_BULK},
            {STRATEGY_AUTO,            "zrangebyscore",    "zrangebyscore",    REPLY_MULTI_BULK},
            {STRATEGY_AUTO,            "zrevrangebyscore", "zrevrangebyscore", REPLY_MULTI_BULK},
            {STRATEGY_AUTO,            "zscan",            "zscan",            REPLY_SCAN},
            {STRATEGY_AUTO,            "zlexcount",        "zlexcount",        REPLY_INT},
            {STRATEGY_AUTO,            "zrangebylex",      "zrangebylex",      REPLY_MULTI_BULK},
            {STRATEGY_AUTO,            "zremrangebylex",   "zremrangebylex",   REPLY_INT},
            {STRATEGY_AUTO,            "zrevrangebylex",   "zrevrangebylex",   REPLY_MULTI_BULK},

            {STRATEGY_AUTO,            "lpush",            "qpush_front",      REPLY_INT},
            {STRATEGY_AUTO,            "rpush",            "qpush_back",       REPLY_INT},
            {STRATEGY_AUTO,            "lpushx",           "qpush_frontx",     REPLY_INT},
            {STRATEGY_AUTO,            "rpushx",           "qpush_backx",      REPLY_INT},


            {STRATEGY_AUTO,            "lpop",             "qpop_front",       REPLY_BULK},
            {STRATEGY_AUTO,            "rpop",             "qpop_back",        REPLY_BULK},
            {STRATEGY_AUTO,            "llen",             "qsize",            REPLY_INT},
            {STRATEGY_AUTO,            "lindex",           "qget",             REPLY_BULK},
            {STRATEGY_AUTO,            "lset",             "qset",             REPLY_OK_STATUS},
            {STRATEGY_AUTO,            "lrange",           "qslice",           REPLY_MULTI_BULK},
            {STRATEGY_AUTO,            "ltrim",            "qtrim",            REPLY_OK_STATUS},

            {STRATEGY_AUTO, NULL, NULL,                                        0}


    };

    class RedisJob {
        RedisRequestDesc *req_desc;

        std::vector<std::string> recv_string;


    public:
        RedisJob(const std::vector<std::string> &recv_string, std::string *response);

        Request req;
        Response response;
        std::string cmd;

        int convert_req();

        virtual ~RedisJob();
    };


    template<class T>
    static std::string serialize_req(T &req) {
        std::string ret;
        char buf[50];
        for (int i = 0; i < req.size(); i++) {
            if (i >= 5 && i < req.size() - 1) {
                sprintf(buf, "[%d more...]", (int) req.size() - i);
                ret.append(buf);
                break;
            }
            if (((req[0] == "get" || req[0] == "set") && i == 1) || req[i].size() < 50) {
                if (req[i].size() == 0) {
                    ret.append("\"\"");
                } else {
                    std::string h = hexmem(req[i].data(), req[i].size());
                    ret.append(h);
                }
            } else {
                sprintf(buf, "[%d]", (int) req[i].size());
                ret.append(buf);
            }
            if (i < req.size() - 1) {
                ret.append(" ");
            }
        }
        return ret;
    }

}

#endif //VCDB_REDIS_CVT_H
