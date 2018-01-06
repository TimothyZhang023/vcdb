//
// Created by zts on 1/2/18.
//
#ifndef VCDB_COMMANDS_H
#define VCDB_COMMANDS_H

#include "ProcDefine.h"


/* Units */
#define UNIT_SECONDS 0
#define UNIT_MILLISECONDS 1


/* Command flags. Please check the command table defined in the redis.c file
 * for more information about the meaning of every flag. */
#define CMD_WRITE (1<<0)            /* "w" flag */
#define CMD_READONLY (1<<1)         /* "r" flag */
#define CMD_DENYOOM (1<<2)          /* "m" flag */
#define CMD_MODULE (1<<3)           /* Command exported by module. */
#define CMD_ADMIN (1<<4)            /* "a" flag */
#define CMD_PUBSUB (1<<5)           /* "p" flag */
#define CMD_NOSCRIPT (1<<6)         /* "s" flag */
#define CMD_RANDOM (1<<7)           /* "R" flag */
#define CMD_SORT_FOR_SCRIPT (1<<8)  /* "S" flag */
#define CMD_LOADING (1<<9)          /* "l" flag */
#define CMD_STALE (1<<10)           /* "t" flag */
#define CMD_SKIP_MONITOR (1<<11)    /* "M" flag */
#define CMD_ASKING (1<<12)          /* "k" flag */
#define CMD_FAST (1<<13)            /* "F" flag */
#define CMD_MODULE_GETKEYS (1<<14)  /* Use the modules getkeys interface. */
#define CMD_MODULE_NO_CLUSTER (1<<15) /* Deny on Redis Cluster. */



typedef int redisCommandProc(ClientContext &ctx, const Request &req, Response *resp);

typedef int *redisGetKeysProc(struct redisCommand *cmd, const Request &req, int *numkeys);

struct redisCommand {
    std::string name;
    redisCommandProc *proc;
    int arity;
    std::string sflags; /* Flags as string representation, one char per flag. */
    int flags;    /* The actual flags, obtained from the 'sflags' field. */
    /* Use a function to determine keys arguments in a command line.
     * Used for Redis Cluster redirect. */
    redisGetKeysProc *getkeys_proc;
    /* What keys should be loaded in background when calling this command? */
    int firstkey; /* The first argument that's a key (0 = no keys) */
    int lastkey;  /* The last argument that's a key */
    int keystep;  /* The step between first and last key */
    int64_t microseconds, calls;
};

struct expiretimeInfo {
    redisCommandProc *proc;
    int unit;
    int base;
    int time_arg_index;
};


/* Our command table.
 *
 * Every entry is composed of the following fields:
 *
 * name: a string representing the command name.
 * function: pointer to the C function implementing the command.
 * arity: number of arguments, it is possible to use -N to say >= N
 * sflags: command flags as string. See below for a table of flags.
 * flags: flags as bitmask. Computed by Redis using the 'sflags' field.
 * get_keys_proc: an optional function to get key arguments from a command.
 *                This is only used when the following three fields are not
 *                enough to specify what arguments are keys.
 * first_key_index: first argument that is a key
 * last_key_index: last argument that is a key
 * key_step: step to get all the keys from first to last argument. For instance
 *           in MSET the step is two since arguments are key,val,key,val,...
 * microseconds: microseconds of total execution time for this command.
 * calls: total number of calls of this command.
 *
 * The flags, microseconds and calls fields are computed by Redis and should
 * always be set to zero.
 *
 * Command flags are expressed using strings where every character represents
 * a flag. Later the populateCommandTable() function will take care of
 * populating the real 'flags' field using this characters.
 *
 * This is the meaning of the flags:
 *
 * w: write command (may modify the key space).
 * r: read command  (will never modify the key space).
 * m: may increase memory usage once called. Don't allow if out of memory.
 * a: admin command, like SAVE or SHUTDOWN.
 * p: Pub/Sub related command.
 * f: force replication of this command, regardless of server.dirty.
 * s: command not allowed in scripts.
 * R: random command. Command is not deterministic, that is, the same command
 *    with the same arguments, with the same key space, may have different
 *    results. For instance SPOP and RANDOMKEY are two random commands.
 * S: Sort command output array if called from script, so that the output
 *    is deterministic.
 * l: Allow command while loading the database.
 * t: Allow command while a slave has stale data but is not allowed to
 *    server this data. Normally no command is accepted in this condition
 *    but just a few.
 * M: Do not automatically propagate the command on MONITOR.
 * k: Perform an implicit ASKING for this command, so the command will be
 *    accepted in cluster mode if the slot is marked as 'importing'.
 * F: Fast command: O(1) or O(log(N)) command that should never delay
 *    its execution as long as the kernel scheduler is giving us time.
 *    Note that commands that may trigger a DEL as a side effect (like SET)
 *    are not fast commands.
 */
static struct redisCommand redisCommandTable[] = {
        {"get",              proc_get,              2,  "rF",  0, NULL, 1, 1,  1, 0, 0},
        {"set",              proc_set,              -3, "wm",  0, NULL, 1, 1,  1, 0, 0},
        {"setnx",            proc_setnx,            3,  "wmF", 0, NULL, 1, 1,  1, 0, 0},
        {"setex",            proc_setex,            4,  "wm",  0, NULL, 1, 1,  1, 0, 0},
        {"psetex",           proc_psetex,           4,  "wm",  0, NULL, 1, 1,  1, 0, 0},
        {"append",           proc_append,           3,  "wm",  0, NULL, 1, 1,  1, 0, 0},
        {"strlen",           proc_strlen,           2,  "rF",  0, NULL, 1, 1,  1, 0, 0},
        {"del",              proc_del,              -2, "w",   0, NULL, 1, -1, 1, 0, 0},
        {"exists",           proc_exists,           -2, "rF",  0, NULL, 1, -1, 1, 0, 0},
        {"setbit",           proc_setbit,           4,  "wm",  0, NULL, 1, 1,  1, 0, 0},
        {"getbit",           proc_getbit,           3,  "rF",  0, NULL, 1, 1,  1, 0, 0},
        {"setrange",         proc_setrange,         4,  "wm",  0, NULL, 1, 1,  1, 0, 0},
        {"getrange",         proc_getrange,         4,  "r",   0, NULL, 1, 1,  1, 0, 0},
        {"substr",           proc_getrange,         4,  "r",   0, NULL, 1, 1,  1, 0, 0},
        {"incr",             proc_incr,             2,  "wmF", 0, NULL, 1, 1,  1, 0, 0},
        {"decr",             proc_decr,             2,  "wmF", 0, NULL, 1, 1,  1, 0, 0},
        {"mget",             proc_mget,             -2, "r",   0, NULL, 1, -1, 1, 0, 0},
        {"rpush",            proc_rpush,            -3, "wmF", 0, NULL, 1, 1,  1, 0, 0},
        {"lpush",            proc_lpush,            -3, "wmF", 0, NULL, 1, 1,  1, 0, 0},
        {"rpushx",           proc_rpushx,           3,  "wmF", 0, NULL, 1, 1,  1, 0, 0},
        {"lpushx",           proc_lpushx,           3,  "wmF", 0, NULL, 1, 1,  1, 0, 0},
        {"rpop",             proc_rpop,             2,  "wF",  0, NULL, 1, 1,  1, 0, 0},
        {"lpop",             proc_lpop,             2,  "wF",  0, NULL, 1, 1,  1, 0, 0},
        {"llen",             proc_llen,             2,  "rF",  0, NULL, 1, 1,  1, 0, 0},
        {"lindex",           proc_lindex,           3,  "r",   0, NULL, 1, 1,  1, 0, 0},
        {"lset",             proc_lset,             4,  "wm",  0, NULL, 1, 1,  1, 0, 0},
        {"lrange",           proc_lrange,           4,  "r",   0, NULL, 1, 1,  1, 0, 0},
        {"ltrim",            proc_ltrim,            4,  "w",   0, NULL, 1, 1,  1, 0, 0},

        {"sadd",             proc_sadd,             -3, "wmF", 0, NULL, 1, 1,  1, 0, 0},
        {"srem",             proc_srem,             -3, "wF",  0, NULL, 1, 1,  1, 0, 0},
        {"sismember",        proc_sismember,        3,  "rF",  0, NULL, 1, 1,  1, 0, 0},
        {"scard",            proc_scard,            2,  "rF",  0, NULL, 1, 1,  1, 0, 0},
        {"spop",             proc_spop,             -2, "wRF", 0, NULL, 1, 1,  1, 0, 0},
        {"srandmember",      proc_srandmember,      -2, "rR",  0, NULL, 1, 1,  1, 0, 0},
        {"smembers",         proc_smembers,         2,  "rS",  0, NULL, 1, 1,  1, 0, 0},
        {"sscan",            proc_sscan,            -3, "rR",  0, NULL, 1, 1,  1, 0, 0},
        {"zadd",             proc_zadd,             -4, "wmF", 0, NULL, 1, 1,  1, 0, 0},
        {"zincrby",          proc_zincrby,          4,  "wmF", 0, NULL, 1, 1,  1, 0, 0},
        {"zrem",             proc_zrem,             -3, "wF",  0, NULL, 1, 1,  1, 0, 0},
        {"zremrangebyscore", proc_zremrangebyscore, 4,  "w",   0, NULL, 1, 1,  1, 0, 0},
        {"zremrangebyrank",  proc_zremrangebyrank,  4,  "w",   0, NULL, 1, 1,  1, 0, 0},
        {"zremrangebylex",   proc_zremrangebylex,   4,  "w",   0, NULL, 1, 1,  1, 0, 0},
        {"zrange",           proc_zrange,           -4, "r",   0, NULL, 1, 1,  1, 0, 0},
        {"zrangebyscore",    proc_zrangebyscore,    -4, "r",   0, NULL, 1, 1,  1, 0, 0},
        {"zrevrangebyscore", proc_zrevrangebyscore, -4, "r",   0, NULL, 1, 1,  1, 0, 0},
        {"zrangebylex",      proc_zrangebylex,      -4, "r",   0, NULL, 1, 1,  1, 0, 0},
        {"zrevrangebylex",   proc_zrevrangebylex,   -4, "r",   0, NULL, 1, 1,  1, 0, 0},
        {"zcount",           proc_zcount,           4,  "rF",  0, NULL, 1, 1,  1, 0, 0},
        {"zlexcount",        proc_zlexcount,        4,  "rF",  0, NULL, 1, 1,  1, 0, 0},
        {"zrevrange",        proc_zrevrange,        -4, "r",   0, NULL, 1, 1,  1, 0, 0},
        {"zcard",            proc_zcard,            2,  "rF",  0, NULL, 1, 1,  1, 0, 0},
        {"zscore",           proc_zscore,           3,  "rF",  0, NULL, 1, 1,  1, 0, 0},
        {"zrank",            proc_zrank,            3,  "rF",  0, NULL, 1, 1,  1, 0, 0},
        {"zrevrank",         proc_zrevrank,         3,  "rF",  0, NULL, 1, 1,  1, 0, 0},
        {"zscan",            proc_zscan,            -3, "rR",  0, NULL, 1, 1,  1, 0, 0},
        {"hset",             proc_hset,             4,  "wmF", 0, NULL, 1, 1,  1, 0, 0},
        {"hsetnx",           proc_hsetnx,           4,  "wmF", 0, NULL, 1, 1,  1, 0, 0},
        {"hget",             proc_hget,             3,  "rF",  0, NULL, 1, 1,  1, 0, 0},
        {"hmset",            proc_hmset,            -4, "wm",  0, NULL, 1, 1,  1, 0, 0},
        {"hmget",            proc_hmget,            -3, "r",   0, NULL, 1, 1,  1, 0, 0},
        {"hincrby",          proc_hincrby,          4,  "wmF", 0, NULL, 1, 1,  1, 0, 0},
        {"hincrbyfloat",     proc_hincrbyfloat,     4,  "wmF", 0, NULL, 1, 1,  1, 0, 0},
        {"hdel",             proc_hdel,             -3, "wF",  0, NULL, 1, 1,  1, 0, 0},
        {"hlen",             proc_hlen,             2,  "rF",  0, NULL, 1, 1,  1, 0, 0},
//        {"hstrlen",          proc_hstrlen,          3,  "rF",   0, NULL, 1, 1,  1, 0, 0},
        {"hkeys",            proc_hkeys,            2,  "rS",  0, NULL, 1, 1,  1, 0, 0},
        {"hvals",            proc_hvals,            2,  "rS",  0, NULL, 1, 1,  1, 0, 0},
        {"hgetall",          proc_hgetall,          2,  "r",   0, NULL, 1, 1,  1, 0, 0},
        {"hexists",          proc_hexists,          3,  "rF",  0, NULL, 1, 1,  1, 0, 0},
        {"hscan",            proc_hscan,            -3, "rR",  0, NULL, 1, 1,  1, 0, 0},
        {"incrby",           proc_incr,             3,  "wmF", 0, NULL, 1, 1,  1, 0, 0},
        {"decrby",           proc_decr,             3,  "wmF", 0, NULL, 1, 1,  1, 0, 0},
        {"incrbyfloat",      proc_incrbyfloat,      3,  "wmF", 0, NULL, 1, 1,  1, 0, 0},
        {"getset",           proc_getset,           3,  "wm",  0, NULL, 1, 1,  1, 0, 0},
        {"mset",             proc_mset,             -3, "wm",  0, NULL, 1, -1, 2, 0, 0},

        {"select",           proc_select,           2,  "lF",  0, NULL, 0, 0,  0, 0, 0},
        {"expire",           proc_expire,           3,  "wF",  0, NULL, 1, 1,  1, 0, 0},
        {"expireat",         proc_expireat,         3,  "wF",  0, NULL, 1, 1,  1, 0, 0},
        {"pexpire",          proc_pexpire,          3,  "wF",  0, NULL, 1, 1,  1, 0, 0},
        {"pexpireat",        proc_pexpireat,        3,  "wF",  0, NULL, 1, 1,  1, 0, 0},
        {"keys",             proc_keys,             2,  "rS",  0, NULL, 0, 0,  0, 0, 0},
        {"scan",             proc_scan,             -2, "rR",  0, NULL, 0, 0,  0, 0, 0},
        {"dbsize",           proc_dbsize,           1,  "rF",  0, NULL, 0, 0,  0, 0, 0},

        {"ping",             proc_ping,             -1, "tF",  0, NULL, 0, 0,  0, 0, 0},

        {"type",             proc_type,             2,  "rF",  0, NULL, 1, 1,  1, 0, 0},
        {"flushdb",          proc_flushdb,          1,  "w",   0, NULL, 0, 0,  0, 0, 0},
        {"flushall",         proc_flushdb,          1,  "w",   0, NULL, 0, 0,  0, 0, 0},
        {"info",             proc_info,             -1, "lt",  0, NULL, 0, 0,  0, 0, 0},

        {"ttl",              proc_ttl,              2,  "rF",  0, NULL, 1, 1,  1, 0, 0},
        {"pttl",             proc_pttl,             2,  "rF",  0, NULL, 1, 1,  1, 0, 0},
        {"persist",          proc_persist,          2,  "wF",  0, NULL, 1, 1,  1, 0, 0},
//        {"slaveof",          proc_slaveof,          3,  "ast",  0, NULL, 0, 0,  0, 0, 0},
//        {"role",             proc_role,             1,  "lst",  0, NULL, 0, 0,  0, 0, 0},
        {"debug",            proc_debug,            -1, "as",  0, NULL, 0, 0,  0, 0, 0},
//        {"config",           proc_config,           -2, "lat",  0, NULL, 0, 0,  0, 0, 0},
//        {"cluster",          proc_cluster,          -2, "a",    0, NULL, 0, 0,  0, 0, 0},
        {"restore",          proc_restore,          -4, "wm",  0, NULL, 1, 1,  1, 0, 0},
        {"restore-asking",   proc_restore,          -4, "wmk", 0, NULL, 1, 1,  1, 0, 0},
//        {"migrate",          proc_migrate,          -6, "w",    0, NULL, 0, 0,  0, 0, 0},
//        {"asking",           proc_asking,           1,  "F",    0, NULL, 0, 0,  0, 0, 0},
//        {"readonly",         proc_readonly,         1,  "F",    0, NULL, 0, 0,  0, 0, 0},
//        {"readwrite",        proc_readwrite,        1,  "F",    0, NULL, 0, 0,  0, 0, 0},
        {"dump",             proc_dump,             2,  "r",   0, NULL, 1, 1,  1, 0, 0},
//        {"object",           proc_object,           3,  "r",    0, NULL, 2, 2,  2, 0, 0},
        {"client",           proc_client,           -2, "as",  0, NULL, 0, 0,  0, 0, 0},
//        {"slowlog",          proc_slowlog,          -2, "a",    0, NULL, 0, 0,  0, 0, 0},
//        {"script",           proc_script,           -2, "s",    0, NULL, 0, 0,  0, 0, 0},
//        {"time",             proc_time,             1,  "RF",   0, NULL, 0, 0,  0, 0, 0},
//        {"bitop",            proc_bitop,            -4, "wm",   0, NULL, 2, -1, 1, 0, 0},
//        {"bitcount",         proc_bitcount,         -2, "r",    0, NULL, 1, 1,  1, 0, 0},
//        {"bitpos",           proc_bitpos,           -3, "r",    0, NULL, 1, 1,  1, 0, 0},

//        {"post",             proc_securityWarning,  -1, "lt",   0, NULL, 0, 0,  0, 0, 0},
//        {"host:",            proc_securityWarning,  -1, "lt",   0, NULL, 0, 0,  0, 0, 0},
//        {"latency",          proc_latency,          -2, "aslt", 0, NULL, 0, 0,  0, 0, 0},

//        {"filesize",         proc_filesize,         1,  "rF",  0, NULL, 0, 0,  0, 0, 0}

};


static struct expiretimeInfo expiretimeInfoTable[] = {
        {proc_setex,     UNIT_SECONDS,      1, 2},
        {proc_psetex,    UNIT_MILLISECONDS, 1, 2},
        {proc_expire,    UNIT_SECONDS,      1, 2},
        {proc_pexpire,   UNIT_MILLISECONDS, 1, 2},
        {proc_expireat,  UNIT_SECONDS,      0, 2},
        {proc_pexpireat, UNIT_MILLISECONDS, 0, 2},
};


typedef redisCommand Command;


#endif //VCDB_COMMANDS_H
