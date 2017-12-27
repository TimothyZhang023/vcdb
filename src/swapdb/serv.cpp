/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "util/log.h"
#include "util/strings.h"
#include "serv.h"
#include "util/bytes.h"
#include <sys/utsname.h>

extern "C" {
#include "redis/zmalloc.h"
}

DEF_PROC(type);
DEF_PROC(get);
DEF_PROC(set);
DEF_PROC(append);
DEF_PROC(setx);
DEF_PROC(psetx);
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
DEF_PROC(del);
DEF_PROC(incr);
DEF_PROC(incrbyfloat);
DEF_PROC(decr);
DEF_PROC(scan);
DEF_PROC(keys);
DEF_PROC(exists);
DEF_PROC(multi_get);
DEF_PROC(multi_set);
DEF_PROC(multi_del);
DEF_PROC(ttl);
DEF_PROC(pttl);
DEF_PROC(expire);
DEF_PROC(pexpire);
DEF_PROC(expireat);
DEF_PROC(pexpireat);
DEF_PROC(persist);
DEF_PROC(hsize);
DEF_PROC(hget);
DEF_PROC(hset);
DEF_PROC(hsetnx);
DEF_PROC(hdel);
DEF_PROC(hincr);
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
DEF_PROC(zrrank);
DEF_PROC(zrange);
DEF_PROC(zrrange);
DEF_PROC(zrangebyscore);
DEF_PROC(zrevrangebyscore);
DEF_PROC(zsize);
DEF_PROC(zget);
DEF_PROC(zincr);
DEF_PROC(zscan);
DEF_PROC(zcount);
DEF_PROC(zremrangebyrank);
DEF_PROC(zremrangebyscore);
DEF_PROC(multi_zset);
DEF_PROC(multi_zdel);
DEF_PROC(zlexcount);
DEF_PROC(zrangebylex);
DEF_PROC(zremrangebylex);
DEF_PROC(zrevrangebylex);
DEF_PROC(qsize);
DEF_PROC(qpush_front);
DEF_PROC(qpush_frontx);
DEF_PROC(qpush_back);
DEF_PROC(qpush_backx);
DEF_PROC(qpop_front);
DEF_PROC(qpop_back);
DEF_PROC(qslice);
DEF_PROC(qtrim);
DEF_PROC(qget);
DEF_PROC(qset);
DEF_PROC(info);
DEF_PROC(save);
DEF_PROC(version);
DEF_PROC(dbsize);
DEF_PROC(filesize);
DEF_PROC(compact);
DEF_PROC(flush);
DEF_PROC(flushdb);
DEF_PROC(dreply);
DEF_PROC(cursor_cleanup);
DEF_PROC(debug);
DEF_PROC(dump);
DEF_PROC(restore);
DEF_PROC(select);
DEF_PROC(client);
DEF_PROC(quit);

DEF_PROC(ssdb_scan);
DEF_PROC(ssdb_dbsize);



void VcServer::regProcs() {
    REG_PROC(type, "rt");
    REG_PROC(get, "rt");
    REG_PROC(set, "wt");
    REG_PROC(append, "wt");
    REG_PROC(del, "wt");
    REG_PROC(setx, "wt");
    REG_PROC(psetx, "wt");
    REG_PROC(setnx, "wt");
    REG_PROC(getset, "wt");
    REG_PROC(getbit, "rt");
    REG_PROC(setbit, "wt");
    REG_PROC(countbit, "rt");
    REG_PROC(substr, "rt");
    REG_PROC(getrange, "rt");
    REG_PROC(setrange, "wt");
    REG_PROC(strlen, "rt");
    REG_PROC(bitcount, "rt");
    REG_PROC(incr, "wt");
    REG_PROC(incrbyfloat, "wt");
    REG_PROC(decr, "wt");
    REG_PROC(scan, "rt");
    REG_PROC(keys, "rt");
    REG_PROC(exists, "rt");
    REG_PROC(multi_get, "rt");
    REG_PROC(multi_set, "wt");
    REG_PROC(multi_del, "wt");
    REG_PROC(ttl, "rt");
    REG_PROC(pttl, "rt");
    REG_PROC(expire, "wt");
    REG_PROC(pexpire, "wt");
    REG_PROC(expireat, "wt");
    REG_PROC(pexpireat, "wt");
    REG_PROC(persist, "wt");

    REG_PROC(hsize, "rt");
    REG_PROC(hget, "rt");
    REG_PROC(hset, "wt");
    REG_PROC(hsetnx, "wt");
    REG_PROC(hincr, "wt");
    REG_PROC(hincrbyfloat, "wt");
    REG_PROC(hgetall, "rt");
    REG_PROC(hscan, "rt");
    REG_PROC(hkeys, "rt");
    REG_PROC(hvals, "rt");
    REG_PROC(hexists, "rt");
    REG_PROC(hmget, "rt");
    REG_PROC(hmset, "wt");
    REG_PROC(hdel, "wt");

    REG_PROC(sadd, "wt");
    REG_PROC(srem, "wt");
    REG_PROC(scard, "rt");
    REG_PROC(sismember, "rt");
    REG_PROC(smembers, "rt");
    REG_PROC(spop, "wt");
    REG_PROC(srandmember, "rt");
    REG_PROC(sscan, "rt");

    REG_PROC(zrank, "rt");
    REG_PROC(zrrank, "rt");
    REG_PROC(zrange, "rt");
    REG_PROC(zrrange, "rt");
    REG_PROC(zrangebyscore, "rt");
    REG_PROC(zrevrangebyscore, "rt");
    REG_PROC(zsize, "rt");
    REG_PROC(zget, "rt");
    REG_PROC(zincr, "wt");
    REG_PROC(zscan, "rt");
    REG_PROC(zcount, "rt");
    REG_PROC(zremrangebyrank, "wt");
    REG_PROC(zremrangebyscore, "wt");
    REG_PROC(multi_zset, "wt");
    REG_PROC(multi_zdel, "wt");
    REG_PROC(zlexcount, "rt");
    REG_PROC(zrangebylex, "rt");
    REG_PROC(zremrangebylex, "wt");
    REG_PROC(zrevrangebylex, "rt");

    REG_PROC(qsize, "rt");
    REG_PROC(qpush_frontx, "wt");
    REG_PROC(qpush_front, "wt");
    REG_PROC(qpush_back, "wt");
    REG_PROC(qpush_backx, "wt");
    REG_PROC(qpop_front, "wt");
    REG_PROC(qpop_back, "wt");
    REG_PROC(qslice, "rt");
    REG_PROC(qget, "rt");
    REG_PROC(qset, "wt");
    REG_PROC(qtrim, "wt");

    REG_PROC(cursor_cleanup, "rt");
    REG_PROC(dump, "wt");
    REG_PROC(restore, "wt");

    REG_PROC(select, "rt");
//    REG_PROC(migrate, "rt");
    REG_PROC(client, "r");
    REG_PROC(quit, "r");
    REG_PROC(flushdb, "wt");
    REG_PROC(flush, "wt");

    REG_PROC(info, "r");
    REG_PROC(dbsize, "rt");
    REG_PROC(save, "rt");

    REG_PROC(compact, "rt");
    REG_PROC(debug, "wt");

}

/*********************/

int proc_flushdb(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;

    log_warn("[!!!] do flushdb");
    serv->db->flushdb(ctx);
    resp->reply_ok();

    return 0;
}

int proc_flush(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;

    serv->db->flush(ctx);
    resp->reply_ok();

    return 0;
}

int proc_select(Context &ctx, const Request &req, Response *resp) {
    resp->reply_ok();
    return 0;
}


int proc_client(Context &ctx, const Request &req, Response *resp) {
    resp->reply_ok();
    return 0;
}



int proc_debug(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(2);

    std::string action = req[1].String();
    strtolower(&action);

    if (action == "segfault") {
        *((char *) -1) = 'x';
    } else if (action == "sleep") {
        CHECK_MIN_PARAMS(3);
        double dtime = req[2].Double();

        long long utime = dtime * 1000000;
        struct timespec tv;

        tv.tv_sec = utime / 1000000;
        tv.tv_nsec = (utime % 1000000) * 1000;
        nanosleep(&tv, NULL);

    } else if (action == "digest") {

        std::string res;
        int ret = serv->db->digest(&res);
        if (ret < 0) {
            addReplyErrorCodeReturn(ret);
        }

        resp->reply_ok();
        resp->add(res);

        return 0;
    } else if (action == "populate") {
        CHECK_MIN_PARAMS(3);

        uint64_t count = req[2].Uint64();

        PTimer timer("DEBUG_POPULATE");
        timer.begin();
        leveldb::WriteBatch batch;
        leveldb::WriteOptions writeOptions;
        writeOptions.disableWAL = true;

        for (uint64_t i = 0; i < count; ++i) {
            char kbuf[128] = {0};
            snprintf(kbuf, sizeof(kbuf), "%s:%lu", "key", i);

            char vbuf[128] = {0};
            snprintf(vbuf, sizeof(vbuf), "%s:%lu", "value", i);

            batch.Put(encode_meta_key(Bytes(kbuf)), encode_kv_val(Bytes(vbuf), 0));

            if ((count % 10000) == 0) {
                leveldb::Status s = serv->db->CommitBatch(ctx, writeOptions, &(batch));
                if (!s.ok()) {
                    log_error("error: %s", s.ToString().c_str());
                    return STORAGE_ERR;
                }

                batch.Clear();
            }

        }

        leveldb::Status s = serv->db->CommitBatch(ctx, writeOptions, &(batch));
        if (!s.ok()) {
            log_error("error: %s", s.ToString().c_str());
            return STORAGE_ERR;
        }

        timer.end(str(count) + " keys");
    }

    resp->reply_ok();
    return 0;
}


int proc_quit(Context &ctx, const Request &req, Response *resp) {
    resp->reply_ok();
    return 0;
}


int proc_restore(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(4);

    int64_t ttl = req[2].Int64();
    if (errno == EINVAL || ttl < 0) {
        addReplyErrorCodeReturn(INVALID_EX_TIME);
    }

    bool replace = false;
    if (req.size() > 4) {
        std::string q4 = req[4].String();
        strtoupper(&q4);
        if (q4 == "REPLACE") {
            replace = true;
        } else {
            addReplyErrorCodeReturn(SYNTAX_ERR);
        }
    }

    std::string val;

    PTST(restore, 0.01)
    int ret = serv->db->restore(ctx, req[1], ttl, req[3], replace, &val);
    PTE(restore, hexstr(req[1]))

    if (ret < 0) {
        log_warn("%s, %s : %s", GetErrorInfo(ret).c_str(), hexmem(req[1].data(), req[1].size()).c_str(),
                 hexmem(req[3].data(), req[3].size()).c_str());
        addReplyErrorCodeReturn(ret);
    } else {
        resp->reply_get(ret, &val);
    }

    return 0;
}

int proc_dump(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(2);

    std::string val;

    PTST(dump, 0.01)
    int ret = serv->db->dump(ctx, req[1], &val, nullptr, true);
    PTE(dump, hexstr(req[1]))

    resp->reply_get(ret, &val);
    return 0;
}


int proc_cursor_cleanup(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(2);

    serv->db->redisCursorCleanup();

    return 0;
}

int proc_compact(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    serv->db->compact();
    resp->reply_ok();
    return 0;
}

int proc_dbsize(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    uint64_t size = serv->db->size();
    resp->reply_int(1, size);

    return 0;
}

int proc_save(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;

    int ret = serv->db->save(ctx);
    if (ret < 0) {
        resp->push_back("error");
    } else {
        resp->push_back("ok");
    }

    return 0;
}


#define ReplyWtihSize(name_size) \
    resp->emplace_back(#name_size":" + str(name_size));

#define ReplyWtihHuman(name_size) \
    resp->emplace_back(#name_size":" + str(name_size));\
    resp->emplace_back(#name_size"_human:" + bytesToHuman((int64_t) name_size));


#define FastGetProperty(key, name) \
    if (serv->db->getLdb()->GetProperty((key), &val)) {\
    uint64_t temp_size = Bytes(val).Uint64();\
    resp->emplace_back(name":" + str(temp_size));\
}

#define FastGetPropertyHuman(key, name) \
    if (serv->db->getLdb()->GetProperty(key, &val)) {\
    uint64_t temp_size = Bytes(val).Uint64();\
    resp->emplace_back(name":" + str(temp_size));\
    resp->emplace_back(name"_human:" + bytesToHuman((int64_t) temp_size));\
}

int proc_info(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;


    static struct utsname name;
    static int call_uname = 1;
    if (call_uname) {
        /* Uname can be slow and is always the same output. Cache it. */
        uname(&name);
        call_uname = 0;
    }

    bool all = true;
    std::string selected;
    if (req.size() > 1) {
        selected = req[1].String();
        strtolower(&selected);
        all = false;
    }


    resp->emplace_back("ok");

    if (all || selected == "server") {
        //# Server
        resp->emplace_back("# Server");
        resp->emplace_back("os:" + str(name.sysname) + " " + str(name.release) + " " + str(name.machine));
        resp->emplace_back("arch_bits:" + str((sizeof(long) == 8) ? 64 : 32));
#ifdef __GNUC__
        resp->emplace_back("gcc_version:" + str(__GNUC__) + "." + str(__GNUC_MINOR__) + "." + str(__GNUC_PATCHLEVEL__));
#else
        resp->emplace_back("gcc_version: 0.0.0");
#endif
        resp->emplace_back("pid:" + str(getpid()));

        resp->emplace_back("");
    };

    if (all || selected == "clients") {
        //# Clients
        resp->emplace_back("# Clients");
//        resp->emplace_back("connected_clients:" + str(ctx.net->link_count));
        resp->emplace_back("blocked_clients: 0");

        resp->emplace_back("");
    };


    if (all || selected == "memory") {//memory
        resp->emplace_back("# Memory");

        uint64_t used_memory_rss = zmalloc_get_rss();
        uint64_t used_memory = used_memory_rss;
        ReplyWtihHuman(used_memory); //TODO Fake
        ReplyWtihHuman(used_memory_rss);

        uint64_t total_system_mem = zmalloc_get_memory_size();
        ReplyWtihHuman(total_system_mem);


        auto options = serv->db->getLdb()->GetOptions().table_factory->GetOptions();
        if (options != nullptr) {
            {
                uint64_t block_cache_size = ((leveldb::BlockBasedTableOptions *) options)->block_cache->GetCapacity();
                ReplyWtihHuman(block_cache_size);
            }

            {
                uint64_t block_cache_used = ((leveldb::BlockBasedTableOptions *) options)->block_cache->GetUsage();
                ReplyWtihHuman(block_cache_used);
            }

        }

        std::string val;
        FastGetPropertyHuman(leveldb::DB::Properties::kCurSizeAllMemTables, "current_memtables_size");
        FastGetPropertyHuman(leveldb::DB::Properties::kSizeAllMemTables, "all_memtables_size");
        FastGetPropertyHuman(leveldb::DB::Properties::kEstimateTableReadersMem, "indexes_filter_blocks");

        if (serv->db->simCache != nullptr) {

            uint64_t block_cache_miss = serv->db->simCache->get_miss_counter();
            uint64_t block_cache_hit = serv->db->simCache->get_hit_counter();
            uint64_t total = block_cache_miss + block_cache_hit;
            ReplyWtihSize(block_cache_miss);
            ReplyWtihSize(block_cache_hit);

            double block_cache_hit_rate = (block_cache_hit * 1.0 / (total + (total > 0 ? 0 : 1)) * 1.0) * 100;
            ReplyWtihSize(block_cache_hit_rate);

//            serv->ssdb->simCache->reset_counter();

        }

        resp->emplace_back("");
    }

    if (all || selected == "persistence") {//filesize
        resp->push_back("# Persistence");

        std::string val;
        FastGetPropertyHuman(leveldb::DB::Properties::kTotalSstFilesSize, "sst_file_size");
        FastGetPropertyHuman(leveldb::DB::Properties::kEstimateLiveDataSize, "live_data_size");

        FastGetProperty(leveldb::DB::Properties::kActualDelayedWriteRate, "write_delay_rate");
        FastGetProperty(leveldb::DB::Properties::kIsWriteStopped, "is_write_stop");

        FastGetProperty(leveldb::DB::Properties::kMemTableFlushPending, "mem_table_flush_pending");
        FastGetProperty(leveldb::DB::Properties::kNumRunningFlushes, "num_running_flushes");

        FastGetProperty(leveldb::DB::Properties::kCompactionPending, "num_compaction_pending");
        FastGetProperty(leveldb::DB::Properties::kNumRunningCompactions, "num_running_compactions");


        resp->emplace_back("bgsave_in_progress:0"); //Todo Fake
        resp->emplace_back("aof_rewrite_in_progress:0"); //Todo Fake
        resp->emplace_back("loading:0"); //Todo Fake

        resp->emplace_back("");
    }


    if (all || selected == "snapshot") {//snapshot
        resp->push_back("# Snapshot");

        std::string val;
        FastGetProperty(leveldb::DB::Properties::kNumSnapshots, "live_snapshots");
        FastGetProperty(leveldb::DB::Properties::kOldestSnapshotTime, "oldest_snapshot");

        resp->emplace_back("");
    }


    if (all || selected == "stats") {//Stats
        resp->push_back("# Stats");
        resp->emplace_back("total_connections_received:0"); //Todo Fake

        {//total_calls
            int64_t calls = 0;
            proc_map_t::iterator it;
            for (it = ctx.serv->procMap.begin(); it != ctx.serv->procMap.end(); it++) {
                Command *cmd = it->second;
                calls += cmd->calls;
            }
            resp->emplace_back("total_commands_processed:" + str(calls));
        }

        resp->emplace_back("");
    }

    if (all || selected == "keyspace") {//Keyspace
        resp->push_back("# Keyspace");

        uint64_t size = serv->db->size();
        resp->emplace_back("db0:keys=" + str(size) + ",expires=0,avg_ttl=0");

        resp->emplace_back("");
    }


    if (selected == "leveldb" || selected == "rocksdb") {
        for (auto const &block : serv->db->info()) {
            resp->push_back(block);
        }

        resp->emplace_back("");
    }

    if (selected == "cmd") {
        for_each(serv->procMap.begin(), ctx.serv->procMap.end(), [&](std::pair<const Bytes, Command *> it) {
            Command *cmd = it.second;
            resp->push_back("cmd." + cmd->name);
            char buf[128];
            snprintf(buf, sizeof(buf), "calls: %" PRIu64 "\ttime_wait: %.0f\ttime_proc: %.0f",
                     cmd->calls, cmd->time_wait, cmd->time_proc);
            resp->push_back(buf);
        });

        resp->emplace_back("");
    }

    resp->push_back("");
    return 0;
}
