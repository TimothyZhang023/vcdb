/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "ssdb_impl.h"


#include "rocksdb/options.h"
#include "rocksdb/env.h"
#include "rocksdb/cache.h"
#include "rocksdb/filter_policy.h"
#include "rocksdb/table.h"
#include "rocksdb/convenience.h"
#include "rocksdb/slice_transform.h"
#include <rocksdb/utilities/sim_cache.h>

extern "C" {
#include <redis/zmalloc.h>
};


#include "t_listener.h"


SSDBImpl::SSDBImpl() {
    ldb = NULL;
    this->bgtask_quit = true;
    expiration = NULL;
}

SSDBImpl::~SSDBImpl() {
    this->stop();

    if (expiration) {
        delete expiration;
    }


    for (auto handle : handles) {
        log_info("ColumnFamilyHandle %s finalized", handle->GetName().c_str());
        delete handle;
    }

    if (ldb) {
        log_info("DB %s finalized", ldb->GetName().c_str());
        delete ldb;
    }

    log_info("SSDBImpl finalized");

}

SSDB *SSDB::open(const Options &opt, const std::string &dir) {
    SSDBImpl *ssdb = new SSDBImpl();

    ssdb->path = dir;
    ssdb->options.create_if_missing = opt.create_if_missing;
    ssdb->options.create_missing_column_families = opt.create_missing_column_families;
    ssdb->options.max_open_files = opt.max_open_files;



    {
        //BlockBasedTableOptions
        rocksdb::BlockBasedTableOptions op;
        std::shared_ptr<rocksdb::Cache> normal_block_cache = rocksdb::NewLRUCache(opt.cache_size * UNIT_MB);

        if (opt.sim_cache > 0) {
            std::shared_ptr<rocksdb::Cache> sim_cache =
                    rocksdb::NewSimCache(normal_block_cache, opt.sim_cache * UNIT_MB, 10);
            op.block_cache = sim_cache;
            ssdb->simCache = (rocksdb::SimCache *) ((sim_cache).get());
        } else {
            op.block_cache = normal_block_cache;
        }


        op.filter_policy = std::shared_ptr<const rocksdb::FilterPolicy>(rocksdb::NewBloomFilterPolicy(10));
        op.block_size = opt.block_size * UNIT_KB;
        op.cache_index_and_filter_blocks = opt.cache_index_and_filter_blocks;
        op.cache_index_and_filter_blocks_with_high_priority = true;
        op.pin_l0_filter_and_index_blocks_in_cache = true;

        ssdb->options.table_factory = std::shared_ptr<rocksdb::TableFactory>(rocksdb::NewBlockBasedTableFactory(op));
    }

//    {
//        ssdb->options.memtable_factory = std::shared_ptr<rocksdb::SkipListFactory>(new rocksdb::SkipListFactory);
//    }

    ssdb->options.compaction_readahead_size = opt.compaction_readahead_size * UNIT_MB;

    ssdb->options.level0_file_num_compaction_trigger = opt.level0_file_num_compaction_trigger; //start compaction
    ssdb->options.level0_slowdown_writes_trigger = opt.level0_slowdown_writes_trigger; //slow write
    ssdb->options.level0_stop_writes_trigger = opt.level0_stop_writes_trigger;  //block write
    //========
    ssdb->options.target_file_size_base = opt.target_file_size_base * UNIT_MB; //sst file target size

    ssdb->options.min_write_buffer_number_to_merge = opt.min_write_buffer_number_to_merge;
    ssdb->options.max_write_buffer_number = opt.max_write_buffer_number;


    ssdb->options.max_background_flushes = opt.max_background_flushes;
    ssdb->options.env->SetBackgroundThreads(opt.max_background_flushes, rocksdb::Env::HIGH);


    if (opt.max_background_compactions > 2) {
        ssdb->options.max_subcompactions = static_cast<uint32_t>(opt.max_background_compactions / 2);

    }
    ssdb->options.max_background_compactions = opt.max_background_compactions;
    ssdb->options.env->SetBackgroundThreads(opt.max_background_compactions, rocksdb::Env::LOW);

    //level config
    ssdb->options.use_direct_reads = opt.use_direct_reads;  //see : http://rocksdb.org/blog/2015/07/23/dynamic-level.html
    ssdb->options.level_compaction_dynamic_level_bytes = opt.level_compaction_dynamic_level_bytes;  //see : http://rocksdb.org/blog/2015/07/23/dynamic-level.html
    ssdb->options.max_bytes_for_level_base = opt.max_bytes_for_level_base * UNIT_MB; //256M
    ssdb->options.max_bytes_for_level_multiplier = opt.max_bytes_for_level_multiplier; //10  // multiplier between levels

    //rate_limiter
//    ssdb->options.rate_limiter = std::shared_ptr<rocksdb::GenericRateLimiter>(new rocksdb::GenericRateLimiter(1024 * 50, 1000, 10));
    // refill_bytes  refill_period_us  1024, 1000 = 1MB/s

    ssdb->options.listeners.push_back(std::shared_ptr<t_listener>(new t_listener()));

    ssdb->options.write_buffer_size = static_cast<size_t >(opt.write_buffer_size) * UNIT_MB;
    if (opt.compression) {
        ssdb->options.compression = rocksdb::kSnappyCompression;
    } else {
        ssdb->options.compression = rocksdb::kNoCompression;
    }


    rocksdb::Status status;

    // open DB with two column families
    std::vector<rocksdb::ColumnFamilyDescriptor> column_families;

    column_families.emplace_back(rocksdb::ColumnFamilyDescriptor(rocksdb::kDefaultColumnFamilyName, ssdb->options));

    column_families.emplace_back(rocksdb::ColumnFamilyDescriptor(REPOPID_CF, rocksdb::ColumnFamilyOptions()));

    status = rocksdb::DB::Open(ssdb->options, ssdb->getDataPath(), column_families, &ssdb->handles, &ssdb->ldb);
    if (!status.ok()) {
        log_error("open db failed: %s", status.ToString().c_str());
        delete ssdb;
        return nullptr;
    }

    ssdb->expiration = new ExpirationHandler(ssdb, true);
    ssdb->start();

    return ssdb;
}


int SSDBImpl::flush(Context &ctx, bool wait) {

    rocksdb::FlushOptions flushOptions;

    flushOptions.wait = wait;

    ldb->Flush(flushOptions);

    return 0;
}

int SSDBImpl::flushdb(Context &ctx) {
//lock
    PTST(flushdb, 0.03)

    Locking<RecordKeyMutex> gl(&mutex_record_);
    PTE(flushdb, "mutex_record_")

    redisCursorService.ClearAllCursor();

    log_info("[flushdb] using DeleteFilesInRange");
    rocksdb::Slice begin("0");
    rocksdb::Slice end("~");
    rocksdb::DeleteFilesInRange(ldb, ldb->DefaultColumnFamily(), &begin, &end);
    PTE(flushdb, "DeleteFilesInRange")

    if (ROCKSDB_MAJOR >= 5) {
        log_info("[flushdb] using DeleteRange");

        ldb->DeleteRange(rocksdb::WriteOptions(), ldb->DefaultColumnFamily(), begin, end);
        ldb->Flush(rocksdb::FlushOptions(), ldb->DefaultColumnFamily());
        PTE(flushdb, "DeleteRange")

    }


    uint64_t total = 0;
    int ret = 1;
    bool stop = false;

    rocksdb::ReadOptions iterate_options;
    iterate_options.fill_cache = false;
    rocksdb::WriteOptions write_opts;

    write_opts.disableWAL = true;
    unique_ptr<rocksdb::Iterator> it = unique_ptr<rocksdb::Iterator>(
            ldb->NewIterator(iterate_options, ldb->DefaultColumnFamily()));

    it->SeekToFirst();

    while (!stop) {

        rocksdb::WriteBatch writeBatch;

        for (int i = 0; i < 100000; i++) {
            if (!it->Valid()) {
                stop = true;
                break;
            }

            total++;
            writeBatch.Delete(it->key());
            it->Next();
        }

        rocksdb::Status s = ldb->Write(write_opts, &writeBatch);
        if (!s.ok()) {
            log_error("del error: %s", s.ToString().c_str());
            stop = true;
            ret = -1;
        }

    }

    PTE(flushdb, "Iteration")

    ldb->Flush(rocksdb::FlushOptions());
    write_opts.disableWAL = false;
    PTE(flushdb, "Iteration Flush")

    rocksdb::WriteBatch writeBatch;
    rocksdb::Status s = CommitBatch(ctx, write_opts, &writeBatch);
    if (!s.ok()) {
        ret = -1;
    }

    PTE(flushdb, "CommitBatch")

    log_info("[flushdb] %d keys deleted by iteration", total);

    return ret;
}

Iterator *SSDBImpl::iterator(const std::string &start, const std::string &end, uint64_t limit,
                             const rocksdb::Snapshot *snapshot) {
    rocksdb::Iterator *it;
    rocksdb::ReadOptions iterate_options;
    iterate_options.fill_cache = false;
    if (snapshot) {
        iterate_options.snapshot = snapshot;
    }
    it = ldb->NewIterator(iterate_options);
    it->Seek(start);
//	if(it->Valid() && it->key() == start){
//		it->Next();
//	}
    return new Iterator(it, end, limit, Iterator::FORWARD, iterate_options.snapshot);
}

Iterator *SSDBImpl::iterator(const std::string &start, const std::string &end, uint64_t limit,
                             const rocksdb::ReadOptions &iterate_options) {
    rocksdb::Iterator *it;
    it = ldb->NewIterator(iterate_options);
    it->Seek(start);
    return new Iterator(it, end, limit, Iterator::FORWARD, iterate_options.snapshot);
}

Iterator *SSDBImpl::rev_iterator(const std::string &start, const std::string &end, uint64_t limit,
                                 const rocksdb::Snapshot *snapshot) {
    rocksdb::Iterator *it;
    rocksdb::ReadOptions iterate_options;
    iterate_options.fill_cache = false;
    if (snapshot) {
        iterate_options.snapshot = snapshot;
    }
    it = ldb->NewIterator(iterate_options);
    it->Seek(start);
    if (!it->Valid()) {
        it->SeekToLast();
    } else {
        if (memcmp(start.data(), it->key().data(), start.size()) != 0) {
            it->Prev();
        }
    }
    return new Iterator(it, end, limit, Iterator::BACKWARD, iterate_options.snapshot);
}

const rocksdb::Snapshot *SSDBImpl::GetSnapshotWithLock() {
    if (ldb) {
        Locking<RecordKeyMutex> gl(&mutex_record_);
        return ldb->GetSnapshot();
    }
    return nullptr;
}

const rocksdb::Snapshot *SSDBImpl::GetSnapshot() {
    if (ldb) {
        return ldb->GetSnapshot();
    }
    return nullptr;
}

void SSDBImpl::ReleaseSnapshot(const rocksdb::Snapshot *snapshot = nullptr) {
    if (snapshot != nullptr) {
        ldb->ReleaseSnapshot(snapshot);
    }
}

/* raw operates */

int SSDBImpl::raw_set(Context &ctx, const Bytes &key, const Bytes &val) {
    rocksdb::WriteOptions write_opts;
    rocksdb::Status s = ldb->Put(write_opts, slice(key), slice(val));
    if (!s.ok()) {
        log_error("set error: %s", s.ToString().c_str());
        return -1;
    }
    return 1;
}

int SSDBImpl::raw_del(Context &ctx, const Bytes &key) {
    rocksdb::WriteOptions write_opts;
    rocksdb::Status s = ldb->Delete(write_opts, slice(key));
    if (!s.ok()) {
        log_error("del error: %s", s.ToString().c_str());
        return -1;
    }
    return 1;
}

int SSDBImpl::raw_get(Context &ctx, const Bytes &key, std::string *val) {
    return raw_get(ctx, key, handles[0], val);
}

int SSDBImpl::raw_get(Context &ctx, const Bytes &key, rocksdb::ColumnFamilyHandle *column_family, std::string *val) {
    rocksdb::ReadOptions opts;
    opts.fill_cache = false;
    rocksdb::Status s = ldb->Get(opts, column_family, slice(key), val);
    if (s.IsNotFound()) {
        return 0;
    }
    if (!s.ok()) {
        log_error("get error: %s", s.ToString().c_str());
        return -1;
    }
    return 1;
}

uint64_t SSDBImpl::size() {

    std::string num = "0";
    ldb->GetProperty("rocksdb.estimate-num-keys", &num);

    return Bytes(num).Uint64();

}

std::vector<std::string> SSDBImpl::info() {
    //  "leveldb.num-files-at-level<N>" - return the number of files at level <N>,
    //     where <N> is an ASCII representation of a level number (e.g. "0").
    //  "leveldb.stats" - returns a multi-line string that describes statistics
    //     about the internal operation of the DB.
    //  "leveldb.sstables" - returns a multi-line string that describes all
    //     of the sstables that make up the db contents.

    std::vector<std::string> info;
    std::vector<std::string> keys;

    for (int i = 0; i < 7; i++) {
        keys.push_back(rocksdb::DB::Properties::kNumFilesAtLevelPrefix + str(i));
//        keys.push_back(rocksdb::DB::Properties::kCompressionRatioAtLevelPrefix + str(i));
    }

    for (size_t i = 0; i < keys.size(); i++) {
        std::string key = keys[i];
        std::string val;
        if (ldb->GetProperty(key, &val)) {
            info.push_back(key + " : " + val);
        }
    }

    keys.clear();

    keys.push_back(rocksdb::DB::Properties::kStats);

    keys.push_back(rocksdb::DB::Properties::kSSTables);
    keys.push_back(rocksdb::DB::Properties::kLevelStats);
    keys.push_back(rocksdb::DB::Properties::kNumSnapshots);
    keys.push_back(rocksdb::DB::Properties::kOldestSnapshotTime);
    keys.push_back(rocksdb::DB::Properties::kTotalSstFilesSize);
    keys.push_back(rocksdb::DB::Properties::kEstimateLiveDataSize);

    keys.push_back(rocksdb::DB::Properties::kEstimateTableReadersMem);
    keys.push_back(rocksdb::DB::Properties::kCurSizeAllMemTables);

    for (size_t i = 0; i < keys.size(); i++) {
        std::string key = keys[i];
        std::string val;
        if (ldb->GetProperty(key, &val)) {
            info.push_back(key);
            info.push_back(val);

        }
    }

    info.push_back("");


    return info;
}

void SSDBImpl::compact() {
    rocksdb::CompactRangeOptions compactRangeOptions = rocksdb::CompactRangeOptions();
    ldb->CompactRange(compactRangeOptions, NULL, NULL);
}


rocksdb::Status SSDBImpl::CommitBatch(Context &ctx, const rocksdb::WriteOptions &options, rocksdb::WriteBatch *updates) {

    rocksdb::Status s = ldb->Write(options, updates);

    return s;
}

rocksdb::Status SSDBImpl::CommitBatch(Context &ctx, rocksdb::WriteBatch *updates) {

    return CommitBatch(ctx, rocksdb::WriteOptions(), updates);

}


void SSDBImpl::start() {
    this->bgtask_quit = false;
    int err = pthread_create(&bg_tid_, NULL, &thread_func, this);
    if (err != 0) {
        log_fatal("can't create thread: %s", strerror(err));
        exit(0);
    }
}

void SSDBImpl::stop() {
    log_info("del thread stopping");

    if (this->bgtask_quit) {
        return;
    }

    this->bgtask_quit = true;
    for (int i = 0; i < 500; i++) {
        if (!bgtask_quit) {
            break;
        }
        log_info("waiting for del thread stop");
        usleep(1000 * 1000);
    }

    log_info("del thread stopped");
}

void SSDBImpl::load_delete_keys_from_db(int num) {
    std::string start;
    start.append(1, DataType::DELETE);
    auto it = std::unique_ptr<Iterator>(this->iterator(start, "", num));
    while (it->next()) {
        if (it->key().String()[0] != DataType::DELETE) {
            break;
        }
        tasks_.push(it->key().String());
    }
}

int SSDBImpl::delete_meta_key(const DeleteKey &dk, rocksdb::WriteBatch &batch) {
    std::string meta_key = encode_meta_key(dk.key);
    std::string meta_val;
    rocksdb::Status s = ldb->Get(rocksdb::ReadOptions(), meta_key, &meta_val);
    if (!s.ok() && !s.IsNotFound()) {
        return -1;
    } else if (s.ok()) {
        Decoder decoder(meta_val.data(), meta_val.size());
        if (decoder.skip(1) == -1) {
            return -1;
        }

        uint16_t version = 0;
        if (decoder.read_uint16(&version) == -1) {
            return -1;
        } else {
            version = be16toh(version);
        }

        char del = meta_val[3];

        if (del == KEY_DELETE_MASK && version == dk.version) {
            batch.Delete(meta_key);
        }
    }
    return 0;
}

void SSDBImpl::delete_key_loop(const std::string &del_key) {
    DeleteKey dk;
    if (dk.DecodeDeleteKey(del_key) == -1) {
        log_fatal("delete key error! %s", hexstr(del_key).c_str());
        return;
    }

    log_debug("deleting key %s , version %d ", hexstr(dk.key).c_str(), dk.version);
//    char log_type=BinlogType::SYNC;
    std::string start = encode_hash_key(dk.key, "", dk.version);
    std::string z_start = encode_zscore_prefix(dk.key, dk.version);


    auto it = std::unique_ptr<Iterator>(this->iterator(start, "", -1));
    rocksdb::WriteBatch batch;
    while (it->next()) {
        if (it->key().empty() || it->key().data()[0] != DataType::ITEM) {
            break;
        }

        ItemKey ik;
        Bytes item_key = it->key();
        if (ik.DecodeItemKey(item_key) == -1) {
            log_fatal("decode delete key error! %s", hexstr(item_key).c_str());
            break;
        }
        if (ik.key == dk.key && ik.version == dk.version) {
            batch.Delete(slice(item_key));
        } else {
            break;
        }
    }

    //clean z*
    auto zit = std::unique_ptr<Iterator>(this->iterator(z_start, "", -1));
    while (zit->next()) {
        if (zit->key().empty() || zit->key().data()[0] != DataType::ZSCORE) {
            break;
        }

        ZScoreItemKey zk;
        Bytes item_key = zit->key();
        if (zk.DecodeItemKey(item_key) == -1) {
            log_fatal("decode delete key error! %s", hexstr(item_key).c_str());
            break;
        }
        if (zk.key == dk.key && zk.version == dk.version) {
            batch.Delete(slice(item_key));
        } else {
            break;
        }
    }

    batch.Delete(del_key);
    RecordKeyLock l(&mutex_record_, dk.key);
    if (delete_meta_key(dk, batch) == -1) {
        log_fatal("delete meta key error! %s", hexstr(del_key).c_str());
        return;
    }

    rocksdb::WriteOptions write_opts;
//    write_opts.disableWAL = true;
    rocksdb::Status s = ldb->Write(write_opts, &batch);
    if (!s.ok()) {
        log_fatal("SSDBImpl::delKey Backend Task error! %s", hexstr(del_key).c_str());
        return;
    }
}

void SSDBImpl::runBGTask() {
    while (!bgtask_quit) {
        mutex_bgtask_.lock();
        if (tasks_.empty()) {
            load_delete_keys_from_db(1000);
            if (tasks_.empty()) {
                mutex_bgtask_.unlock();
                usleep(1000 * 1000);
                continue;
            }
        }

        std::string del_key;
        if (!tasks_.empty()) {
            del_key = tasks_.front();
            tasks_.pop();
        }
        mutex_bgtask_.unlock();

        if (!del_key.empty()) {
            delete_key_loop(del_key);
            sched_yield();
        }
    }

    Locking<Mutex> l(&this->mutex_bgtask_);
    std::queue<std::string> tmp_tasks_;
    tasks_.swap(tmp_tasks_);
    bgtask_quit = false;
}

void *SSDBImpl::thread_func(void *arg) {
    SSDBImpl *bg_task = (SSDBImpl *) arg;

    bg_task->runBGTask();

    return (void *) NULL;
}


#include "redis/rdb.h"

extern "C" {
#include "redis/crc64.h"
#include "redis/endianconv.h"
};

class RocksdbWritableFileEncoder : public RedisEncoder {
public:
    explicit RocksdbWritableFileEncoder(rocksdb::WritableFile *handle) : handle(handle) {}

    rocksdb::WritableFile *handle;
    rocksdb::Status s;

    uint64_t cksum = 0;

    bool update_cksum = false;

    void genericUpdateChecksum(void *p, size_t n) {
        cksum = crc64_fast(cksum, p, n);
    }

    int rdbWriteRaw(void *p, size_t n) override {
        if (handle != nullptr) {
            s = handle->Append(rocksdb::Slice((const char *) p, n));
            if (update_cksum) {
                genericUpdateChecksum(p, n);
            }
            if (!s.ok()) {
                return -1;
            }
        }
        return 0;
    }
};

int SSDBImpl::save(Context &ctx) {
    Locking<Mutex> l(&this->mutex_backup_);

    rocksdb::Status s;

    int64_t now = time_ms();

    rocksdb::EnvOptions options;
    unique_ptr<rocksdb::WritableFile> saved;
    rocksdb::Env *env = rocksdb::Env::Default();
    s = env->NewWritableFile(path + "/dump.rdb", &saved, options);

    if (!s.ok()) {
        log_error("%s", s.ToString().c_str());
        return -1;
    }

    if (!saved) {
        return -1;
    }

    RocksdbWritableFileEncoder encoder(saved.get());
    encoder.update_cksum = true;

    char magic[10];
    snprintf(magic, sizeof(magic), "REDIS%04d", FAKE_RDB_VERSION);
    if (encoder.rdbWriteRaw(magic, 9) == -1) return -1;

    /* Add a few fields about the state when the RDB was created. */
    int redis_bits = (sizeof(void *) == 8) ? 64 : 32;

    if (encoder.rdbSaveAuxFieldStrStr("redis-ver", FAKE_REDIS_VERSION) == -1) return -1;
    if (encoder.rdbSaveAuxFieldStrInt("redis-bits", redis_bits) == -1) return -1;
    if (encoder.rdbSaveAuxFieldStrInt("ctime", time(NULL)) == -1) return -1;
    if (encoder.rdbSaveAuxFieldStrInt("used-mem", (long long int) zmalloc_get_rss()) == -1) return -1;


    if (encoder.rdbSaveType(RDB_OPCODE_SELECTDB) == -1) return -1;
    if (encoder.rdbSaveLen(0) == -1) return -1;

    if (encoder.rdbSaveType(RDB_OPCODE_RESIZEDB) == -1)return -1;
    if (encoder.rdbSaveLen(UINT32_MAX) == -1) return -1;
    if (encoder.rdbSaveLen(UINT32_MAX) == -1) return -1;


    std::string start;
    start.append(1, DataType::META);

    char dtype;

    const rocksdb::Snapshot *snapshot = GetSnapshot();
    SnapshotPtr spl(ldb, snapshot); //auto release


    auto it = std::unique_ptr<MIterator>(new MIterator(iterator(start, "", -1, snapshot)));
    while (it->next()) {
        const Bytes &key = it->key;
        const std::string &meta_val = it->val.String();

        //decodeMetaVal
        if (meta_val.size() < 4) {
            //invalid
            log_error("invalid MetaVal: %s", s.ToString().c_str());
            continue;
        }

        char del = meta_val[POS_DEL];
        if (del != KEY_ENABLED_MASK) {
            //deleted
            continue;
        }

        int64_t expire = expiration->pttl(ctx, key, TimeUnit::Millisecond);
        if (expire == -2) continue;

        if (expire != -1) {
            /* If this key is already expired skip it */
            if (encoder.rdbSaveType(RDB_OPCODE_EXPIRETIME_MS) == -1) return -1;
            if (encoder.rdbSaveMillisecondTime(expire + now) == -1) return -1;
        }

        //save type
        dtype = meta_val[0];
        if (encoder.rdbSaveObjectType(dtype) == -1) return -1;
        if (encoder.rdbSaveRawString(key.String()) == -1) return -1;
        if (rdbSaveObject(ctx, key, dtype, meta_val, encoder, snapshot) < 0) return -1;

    }


    if (encoder.rdbSaveType(RDB_OPCODE_EOF) == -1) return -1;

    uint64_t cksum = encoder.cksum;
    memrev64ifbe(&cksum);
    if (encoder.rdbWriteRaw(&cksum, 8) == -1) return -1;

    saved->Flush();
    saved->Sync();

    saved->Close();
    return 0;
}
