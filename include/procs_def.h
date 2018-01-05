//
// Created by zts on 1/2/18.
//

#ifndef VCDB_PROCS_H
#define VCDB_PROCS_H

#include "ClientContext.hpp"
#include "Response.h"
#include "Request.h"


#define DEF_PROC(f) int proc_##f(ClientContext &ctx, const Request &req, Response *resp)

DEF_PROC(type);

DEF_PROC(get);

DEF_PROC(set);

DEF_PROC(append);

DEF_PROC(setex);

DEF_PROC(psetex);

DEF_PROC(setnx);

DEF_PROC(getset);

DEF_PROC(getbit);

DEF_PROC(setbit);

DEF_PROC(countbit);

DEF_PROC(substr);

DEF_PROC(getrange);

DEF_PROC(setrange);

DEF_PROC(strlen);

DEF_PROC(bitcount);

DEF_PROC(incr);

DEF_PROC(incrbyfloat);

DEF_PROC(decr);

DEF_PROC(scan);

DEF_PROC(keys);

DEF_PROC(exists);

DEF_PROC(mget);

DEF_PROC(mset);

DEF_PROC(del);

DEF_PROC(ttl);

DEF_PROC(pttl);

DEF_PROC(expire);

DEF_PROC(pexpire);

DEF_PROC(expireat);

DEF_PROC(pexpireat);

DEF_PROC(persist);

DEF_PROC(hlen);

DEF_PROC(hget);

DEF_PROC(hset);

DEF_PROC(hsetnx);

DEF_PROC(hdel);

DEF_PROC(hincrby);

DEF_PROC(hincrbyfloat);

DEF_PROC(hgetall);

DEF_PROC(hscan);

DEF_PROC(hkeys);

DEF_PROC(hvals);

DEF_PROC(hexists);

DEF_PROC(hmget);

DEF_PROC(hmset);

DEF_PROC(sadd);

DEF_PROC(srem);

DEF_PROC(scard);

DEF_PROC(sismember);

DEF_PROC(smembers);

DEF_PROC(spop);

DEF_PROC(srandmember);

DEF_PROC(sscan);

DEF_PROC(zrank);

DEF_PROC(zrevrank);

DEF_PROC(zrange);

DEF_PROC(zrevrange);

DEF_PROC(zrangebyscore);

DEF_PROC(zrevrangebyscore);

DEF_PROC(zcard);

DEF_PROC(zscore);

DEF_PROC(zincrby);

DEF_PROC(zscan);

DEF_PROC(zcount);

DEF_PROC(zremrangebyrank);

DEF_PROC(zremrangebyscore);

DEF_PROC(zadd);

DEF_PROC(zrem);

DEF_PROC(zlexcount);

DEF_PROC(zrangebylex);

DEF_PROC(zremrangebylex);

DEF_PROC(zrevrangebylex);

DEF_PROC(llen);

DEF_PROC(lpush);

DEF_PROC(lpushx);

DEF_PROC(rpush);

DEF_PROC(rpushx);

DEF_PROC(lpop);

DEF_PROC(rpop);

DEF_PROC(lrange);

DEF_PROC(ltrim);

DEF_PROC(lindex);

DEF_PROC(lset);

DEF_PROC(info);

DEF_PROC(save);

DEF_PROC(version);

DEF_PROC(dbsize);

DEF_PROC(filesize);

DEF_PROC(compact);

DEF_PROC(flush);

DEF_PROC(flushdb);

DEF_PROC(flushall);

DEF_PROC(dreply);

DEF_PROC(cursor_cleanup);

DEF_PROC(debug);

DEF_PROC(dump);

DEF_PROC(restore);

DEF_PROC(select);

DEF_PROC(client);

DEF_PROC(quit);

DEF_PROC(ping);

DEF_PROC(ssdb_scan);

DEF_PROC(ssdb_dbsize);


#endif //VCDB_PROCS_H
