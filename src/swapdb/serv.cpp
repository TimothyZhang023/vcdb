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


/*********************/

int proc_flushdb(ClientContext &ctx, const Request &req, Response *resp) {

    log_warn("[!!!] do flushdb");
    ctx.db->flushdb(ctx);
    resp->addReplyStatusOK();

    return 0;
}

int proc_flushall(ClientContext &ctx, const Request &req, Response *resp) {
    return proc_flushdb(ctx, req, resp);
}

int proc_flush(ClientContext &ctx, const Request &req, Response *resp) {

    ctx.db->flush(ctx);
    resp->addReplyStatusOK();

    return 0;
}

int proc_select(ClientContext &ctx, const Request &req, Response *resp) {
    resp->addReplyStatusOK();
    return 0;
}


int proc_client(ClientContext &ctx, const Request &req, Response *resp) {
    resp->addReplyStatusOK();
    return 0;
}


int proc_debug(ClientContext &ctx, const Request &req, Response *resp) {
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
        int ret = ctx.db->digest(&res);
        if (ret < 0) {
            addReplyErrorCodeReturn(ret);
        }

        resp->addReplyStatus(res);
        return 0;
    } else if (action == "populate") {
        CHECK_MIN_PARAMS(3);

        uint64_t count = req[2].Uint64();

        PTimer timer("DEBUG_POPULATE");
        timer.begin();
        rocksdb::WriteBatch batch;
        rocksdb::WriteOptions writeOptions;
        writeOptions.disableWAL = true;

        for (uint64_t i = 0; i < count; ++i) {
            char kbuf[128] = {0};
            snprintf(kbuf, sizeof(kbuf), "%s:%lu", "key", i);

            char vbuf[128] = {0};
            snprintf(vbuf, sizeof(vbuf), "%s:%lu", "value", i);

            batch.Put(encode_meta_key(Bytes(kbuf)), encode_kv_val(Bytes(vbuf), 0));

            if ((count % 10000) == 0) {
                rocksdb::Status s = ctx.db->CommitBatch(ctx, writeOptions, &(batch));
                if (!s.ok()) {
                    log_error("error: %s", s.ToString().c_str());
                    return STORAGE_ERR;
                }

                batch.Clear();
            }

        }

        rocksdb::Status s = ctx.db->CommitBatch(ctx, writeOptions, &(batch));
        if (!s.ok()) {
            log_error("error: %s", s.ToString().c_str());
            return STORAGE_ERR;
        }

        timer.end(str(count) + " keys");
    }

    resp->addReplyStatusOK();
    return 0;
}


int proc_quit(ClientContext &ctx, const Request &req, Response *resp) {
    resp->addReplyStatusOK();
    return 0;
}

int proc_ping(ClientContext &ctx, const Request &req, Response *resp) {
    resp->addReplyStatus("PONG");
    return 0;
}


int proc_restore(ClientContext &ctx, const Request &req, Response *resp) {
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
    int ret = ctx.db->restore(ctx, req[1], ttl, req[3], replace, &val);PTE(restore, hexstr(req[1]))

    if (ret < 0) {
        log_warn("%s, %s : %s", GetErrorInfo(ret).c_str(), hexmem(req[1].data(), req[1].size()).c_str(),
                 hexmem(req[3].data(), req[3].size()).c_str());
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyStatus(val);
    }

    return 0;
}

int proc_dump(ClientContext &ctx, const Request &req, Response *resp) {
        CHECK_MIN_PARAMS(2);

    std::string val;

    int ret = ctx.db->dump(ctx, req[1], &val, nullptr, true);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else if (ret == 0) {
        resp->addReplyNil();
    } else {
        resp->addReplyString(val);
    }

    return 0;
}


int proc_cursor_cleanup(ClientContext &ctx, const Request &req, Response *resp) {
        CHECK_MIN_PARAMS(2);

    ctx.db->redisCursorCleanup();

    return 0;
}

int proc_compact(ClientContext &ctx, const Request &req, Response *resp) {
        ctx.db->compact();
    resp->addReplyStatusOK();
    return 0;
}

int proc_dbsize(ClientContext &ctx, const Request &req, Response *resp) {
        uint64_t size = ctx.db->size();

    resp->addReplyInt(size);

    return 0;
}

int proc_save(ClientContext &ctx, const Request &req, Response *resp) {

    int ret = ctx.db->save(ctx);
    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyStatusOK();
    }

    return 0;
}


#define ReplyWtihSize(name_size) \
    resp_arr->emplace_back(#name_size":" + str(name_size));

#define ReplyWtihHuman(name_size) \
    resp_arr->emplace_back(#name_size":" + str(name_size));\
    resp_arr->emplace_back(#name_size"_human:" + bytesToHuman((int64_t) name_size));


#define FastGetProperty(key, name) \
    if (ctx.db->getLdb()->GetProperty((key), &val)) {\
    uint64_t temp_size = Bytes(val).Uint64();\
    resp_arr->emplace_back(name":" + str(temp_size));\
}

#define FastGetPropertyHuman(key, name) \
    if (ctx.db->getLdb()->GetProperty(key, &val)) {\
    uint64_t temp_size = Bytes(val).Uint64();\
    resp_arr->emplace_back(name":" + str(temp_size));\
    resp_arr->emplace_back(name"_human:" + bytesToHuman((int64_t) temp_size));\
}

int proc_info(ClientContext &ctx, const Request &req, Response *resp) {


    auto resp_arr = &resp->resp_arr;

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


    resp_arr->emplace_back("ok");

    if (all || selected == "server") {
        //# Server
        resp_arr->emplace_back("# Server");
        resp_arr->emplace_back("os:" + str(name.sysname) + " " + str(name.release) + " " + str(name.machine));
        resp_arr->emplace_back("arch_bits:" + str((sizeof(long) == 8) ? 64 : 32));
#ifdef __GNUC__
        resp_arr->emplace_back("gcc_version:" + str(__GNUC__) + "." + str(__GNUC_MINOR__) + "." + str(__GNUC_PATCHLEVEL__));
#else
        resp->emplace_back("gcc_version: 0.0.0");
#endif
        resp_arr->emplace_back("pid:" + str(getpid()));

        resp_arr->emplace_back("");
    };

    if (all || selected == "clients") {
        //# Clients
        resp_arr->emplace_back("# Clients");
//        resp->emplace_back("connected_clients:" + str(ctx.net->link_count));
        resp_arr->emplace_back("blocked_clients: 0");

        resp_arr->emplace_back("");
    };


    if (all || selected == "memory") {//memory
        resp_arr->emplace_back("# Memory");

        uint64_t used_memory_rss = zmalloc_get_rss();
        uint64_t used_memory = used_memory_rss;
        ReplyWtihHuman(used_memory); //TODO Fake
        ReplyWtihHuman(used_memory_rss);

        uint64_t total_system_mem = zmalloc_get_memory_size();
        ReplyWtihHuman(total_system_mem);


        auto options = ctx.db->getLdb()->GetOptions().table_factory->GetOptions();
        if (options != nullptr) {
            {
                uint64_t block_cache_size = ((rocksdb::BlockBasedTableOptions *) options)->block_cache->GetCapacity();
                ReplyWtihHuman(block_cache_size);
            }

            {
                uint64_t block_cache_used = ((rocksdb::BlockBasedTableOptions *) options)->block_cache->GetUsage();
                ReplyWtihHuman(block_cache_used);
            }

        }

        std::string val;
        FastGetPropertyHuman(rocksdb::DB::Properties::kCurSizeAllMemTables, "current_memtables_size");
        FastGetPropertyHuman(rocksdb::DB::Properties::kSizeAllMemTables, "all_memtables_size");
        FastGetPropertyHuman(rocksdb::DB::Properties::kEstimateTableReadersMem, "indexes_filter_blocks");

        if (ctx.db->simCache != nullptr) {

            uint64_t block_cache_miss = ctx.db->simCache->get_miss_counter();
            uint64_t block_cache_hit = ctx.db->simCache->get_hit_counter();
            uint64_t total = block_cache_miss + block_cache_hit;
            ReplyWtihSize(block_cache_miss);
            ReplyWtihSize(block_cache_hit);

            double block_cache_hit_rate = (block_cache_hit * 1.0 / (total + (total > 0 ? 0 : 1)) * 1.0) * 100;
            ReplyWtihSize(block_cache_hit_rate);

//            serv->ssdb->simCache->reset_counter();

        }

        resp_arr->emplace_back("");
    }

    if (all || selected == "persistence") {//filesize
        resp_arr->emplace_back("#Persistence");

        std::string val;
        FastGetPropertyHuman(rocksdb::DB::Properties::kTotalSstFilesSize, "sst_file_size");
        FastGetPropertyHuman(rocksdb::DB::Properties::kEstimateLiveDataSize, "live_data_size");

        FastGetProperty(rocksdb::DB::Properties::kActualDelayedWriteRate, "write_delay_rate");
        FastGetProperty(rocksdb::DB::Properties::kIsWriteStopped, "is_write_stop");

        FastGetProperty(rocksdb::DB::Properties::kMemTableFlushPending, "mem_table_flush_pending");
        FastGetProperty(rocksdb::DB::Properties::kNumRunningFlushes, "num_running_flushes");

        FastGetProperty(rocksdb::DB::Properties::kCompactionPending, "num_compaction_pending");
        FastGetProperty(rocksdb::DB::Properties::kNumRunningCompactions, "num_running_compactions");


        resp_arr->emplace_back("bgsave_in_progress:0"); //Todo Fake
        resp_arr->emplace_back("aof_rewrite_in_progress:0"); //Todo Fake
        resp_arr->emplace_back("loading:0"); //Todo Fake

        resp_arr->emplace_back("");
    }


    if (all || selected == "snapshot") {//snapshot
        resp_arr->emplace_back("#Snapshot");

        std::string val;
        FastGetProperty(rocksdb::DB::Properties::kNumSnapshots, "live_snapshots");
        FastGetProperty(rocksdb::DB::Properties::kOldestSnapshotTime, "oldest_snapshot");

        resp_arr->emplace_back("");
    }


    if (all || selected == "keyspace") {//Keyspace
        resp_arr->emplace_back("#Keyspace");

        uint64_t size = ctx.db->size();
        resp_arr->emplace_back("db0:keys=" + str(size) + ",expires=0,avg_ttl=0");

        resp_arr->emplace_back("");
    }


    if (selected == "rocksdb") {
        for (auto const &block : ctx.db->info()) {
            resp_arr->push_back(block);
        }

        resp_arr->emplace_back("");
    }


    resp_arr->emplace_back("");
    return 0;
}
