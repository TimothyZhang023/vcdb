/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include <climits>
#include <stack>
#include "t_zset.h"

/**
 * @return -1: error, 0: item updated, 1: new item inserted
 */

int SSDBImpl::multi_zset(Context &ctx, const Bytes &name, const std::map<Bytes, Bytes> &sortedSet, int flags,
                         int64_t *num) {
    RecordKeyLock l(&mutex_record_, name.String());
    return zsetNoLock<Bytes>(ctx, name, sortedSet, flags, num);
}

int SSDBImpl::multi_zdel(Context &ctx, const Bytes &name, const std::set<Bytes> &keys, int64_t *count) {
    RecordKeyLock l(&mutex_record_, name.String());
    return zdelNoLock(ctx, name, keys, count);
}

int SSDBImpl::zdelNoLock(Context &ctx, const Bytes &name, const std::set<Bytes> &keys, int64_t *count) {
    ZSetMetaVal zv;
    rocksdb::WriteBatch batch;

    std::string meta_key = encode_meta_key(name);
    int ret = GetZSetMetaVal(meta_key, zv);
    if (ret <= 0) {
        return ret;
    }

    for (auto it = keys.begin(); it != keys.end(); ++it) {
        const Bytes &key = *it;
        ret = zdel_one(batch, name, key, zv.version);
        if (ret < 0) {
            return ret;
        }
        *count += ret;
    }

    int iret = incr_zsize(ctx, name, batch, zv, -(*count));
    if (iret < 0) {
        return iret;
    } else if (iret == 0) {
        ret = 0;
    } else {
        ret = 1;
    }

    rocksdb::Status s = CommitBatch(ctx, &(batch));
    if (!s.ok()) {
        log_error("zdel error: %s", s.ToString().c_str());
        return STORAGE_ERR;
    }

    return ret;
}

int SSDBImpl::zincr(Context &ctx, const Bytes &name, const Bytes &key, double by, int &flags, double *new_val) {
    RecordKeyLock l(&mutex_record_, name.String());
    rocksdb::WriteBatch batch;
    ZSetMetaVal zv;
    bool needCheck = false;

    std::string meta_key = encode_meta_key(name);
    int ret = GetZSetMetaVal(meta_key, zv);
    if (ret < 0) {
        return ret;
    } else if (ret == 0) {
        needCheck = false;
    } else {
        ret = 1;
        needCheck = true;
    }

    int retval = zset_one(batch, needCheck, name, key, by, zv.version, &flags, new_val);
    if (retval < 0) {
        return retval;
    }

    if (flags & ZADD_ADDED) {
        int iret = incr_zsize(ctx, name, batch, zv, 1);
        if (iret < 0) {
            return iret;
        } else if (iret == 0) {
            ret = 0;
        } else {
            ret = 1;
        }
    }

    rocksdb::Status s = CommitBatch(ctx, &(batch));
    if (!s.ok()) {
        log_error("zset error: %s", s.ToString().c_str());
        return STORAGE_ERR;
    }

    return ret;
}

int SSDBImpl::zsize(Context &ctx, const Bytes &name, uint64_t *size) {
    ZSetMetaVal zv;
    std::string meta_key = encode_meta_key(name);
    int ret = GetZSetMetaVal(meta_key, zv);
    if (ret <= 0) {
        return ret;
    } else {
        *size = zv.length;
        return 1;
    }
}

int SSDBImpl::GetZSetMetaVal(const std::string &meta_key, ZSetMetaVal &zv) {
    std::string meta_val;
    rocksdb::Status s = ldb->Get(rocksdb::ReadOptions(), meta_key, &meta_val);
    if (s.IsNotFound()) {
        zv.length = 0;
        zv.del = KEY_ENABLED_MASK;
        zv.type = DataType::ZSIZE;
        zv.version = 0;
        return 0;
    } else if (!s.ok() && !s.IsNotFound()) {
        //error
        log_error("GetZSetMetaVal error: %s", s.ToString().c_str());
        return STORAGE_ERR;
    } else {
        int ret = zv.DecodeMetaVal(meta_val);
        if (ret < 0) {
            //error
            return ret;
        } else if (zv.del == KEY_DELETE_MASK) {
            //deleted , reset zv
            if (zv.version == UINT16_MAX) {
                zv.version = 0;
            } else {
                zv.version = (uint16_t) (zv.version + 1);
            }
            zv.length = 0;
            zv.del = KEY_ENABLED_MASK;
            return 0;
        } else if (zv.type != DataType::ZSIZE) {
            return WRONG_TYPE_ERR;
        }
    }
    return 1;
}

int SSDBImpl::GetZSetItemVal(const std::string &item_key, double *score) {
    std::string str_score;
    rocksdb::Status s = ldb->Get(rocksdb::ReadOptions(), item_key, &str_score);
    if (s.IsNotFound()) {
        return 0;
    }
    if (!s.ok()) {
        log_error("GetZSetItemVal error: %s", s.ToString().c_str());
        return STORAGE_ERR;
    }

    *score = *((double *) (str_score.data()));
    return 1;
}

//zscore
int SSDBImpl::zget(Context &ctx, const Bytes &name, const Bytes &key, double *score) {
    *score = 0;

    ZSetMetaVal zv;
    std::string meta_key = encode_meta_key(name);
    int ret = GetZSetMetaVal(meta_key, zv);
    if (ret != 1) {
        return ret;
    }

    std::string str_score;
    std::string dbkey = encode_zset_key(name, key, zv.version);
    rocksdb::Status s = ldb->Get(rocksdb::ReadOptions(), dbkey, &str_score);
    if (s.IsNotFound()) {
        return 0;
    }
    if (!s.ok()) {
        log_error("%s", s.ToString().c_str());
        return STORAGE_ERR;
    }

    *score = *((double *) (str_score.data()));
    return 1;
}

ZIterator *SSDBImpl::zscan_internal(Context &ctx, const Bytes &name, const Bytes &score_start, const Bytes &score_end,
                                    uint64_t limit, Iterator::Direction direction, uint16_t version,
                                    const rocksdb::Snapshot *snapshot) {
    rocksdb::ReadOptions iterate_options(false, true);
    if (snapshot) {
        iterate_options.snapshot = snapshot;
    }

    if (direction == Iterator::FORWARD) {
        std::string start, end;
        if (score_start.empty()) {
            start = encode_zscore_key(name, "", -std::numeric_limits<double>::quiet_NaN(), version);
        } else {
            start = encode_zscore_key(name, "", score_start.Double(), version);
        }
        if (score_end.empty()) {
            end = encode_zscore_key(name, "", std::numeric_limits<double>::quiet_NaN(), version);
        } else {
            end = encode_zscore_key(name, "", score_end.Double(), version);
        }
        return new ZIterator(this->iterator(start, end, limit, iterate_options), name, version);
    } else {
        std::string start, end;
        if (score_start.empty()) {
            start = encode_zscore_key(name, "", std::numeric_limits<double>::quiet_NaN(), version);
        } else {
            start = encode_zscore_key(name, "", score_start.Double(), version);
        }
        if (score_end.empty()) {
            end = encode_zscore_key(name, "", -std::numeric_limits<double>::quiet_NaN(), version);
        } else {
            end = encode_zscore_key(name, "", score_end.Double(), version);
        }
        return new ZIterator(this->rev_iterator(start, end, limit, snapshot), name, version);
    }
}

ZIteratorByLex *
SSDBImpl::zscanbylex_internal(Context &ctx, const Bytes &name, const Bytes &key_start, const Bytes &key_end,
                              uint64_t limit, Iterator::Direction direction, uint16_t version,
                              const rocksdb::Snapshot *snapshot) {
    std::string start, end;
    start = encode_zset_key(name, key_start, version);
    if (key_end != "") {
        end = encode_zset_key(name, key_end, version);
    } else {
        end = "";
    }

    if (direction == Iterator::FORWARD) {
        return new ZIteratorByLex(this->iterator(start, end, limit, snapshot), name, version);
    } else {
        return new ZIteratorByLex(this->rev_iterator(start, end, limit, snapshot), name, version);
    }
}

int SSDBImpl::zrank(Context &ctx, const Bytes &name, const Bytes &key, int64_t *rank) {
    ZSetMetaVal zv;
    const rocksdb::Snapshot *snapshot = nullptr;

    {
        RecordKeyLock l(&mutex_record_, name.String());
        std::string meta_key = encode_meta_key(name);
        int ret = GetZSetMetaVal(meta_key, zv);
        if (ret != 1) {
            return ret;
        }

        snapshot = GetSnapshot();
    }

    SnapshotPtr spl(ldb, snapshot); //auto release

    bool found = false;
    auto it = std::unique_ptr<ZIterator>(
            this->zscan_internal(ctx, name, "", "", INT_MAX, Iterator::FORWARD, zv.version, snapshot));
    uint64_t ret = 0;
    while (true) {
        if (!it->next()) {
            break;
        }
        if (key == it->key) {
            found = true;
            break;
        }
        ret++;
    }

    *rank = found ? (int64_t) ret : -1;

    return 1;
}

int SSDBImpl::zrrank(Context &ctx, const Bytes &name, const Bytes &key, int64_t *rank) {
    ZSetMetaVal zv;
    const rocksdb::Snapshot *snapshot = nullptr;

    {
        RecordKeyLock l(&mutex_record_, name.String());

        std::string meta_key = encode_meta_key(name);
        int ret = GetZSetMetaVal(meta_key, zv);
        if (ret != 1) {
            return ret;
        }

        snapshot = GetSnapshot();
    }

    SnapshotPtr spl(ldb, snapshot); //auto release
    bool found = false;
    auto it = std::unique_ptr<ZIterator>(
            this->zscan_internal(ctx, name, "", "", INT_MAX, Iterator::BACKWARD, zv.version, snapshot));
    uint64_t ret = 0;
    while (true) {
        if (!it->next()) {
            break;
        }
        if (key == it->key) {
            found = true;
            break;
        }
        ret++;
    }

    *rank = found ? (int64_t) ret : -1;

    return 1;
}

int SSDBImpl::zrangeGeneric(Context &ctx, const Bytes &name, const Bytes &begin, const Bytes &limit, bool withscore,
                            std::vector<string> &key_score,
                            int reverse) {
    long long start, end;
    if (string2ll(begin.data(), (size_t) begin.size(), &start) == 0) {
        return INVALID_INT;
    } else if (start < LONG_MIN || start > LONG_MAX) {
        return INVALID_INT;
    }
    if (string2ll(limit.data(), (size_t) limit.size(), &end) == 0) {
        return INVALID_INT;
    } else if (end < LONG_MIN || end > LONG_MAX) {
        return INVALID_INT;
    }

    uint16_t version = 0;
    ZSetMetaVal zv;
    int llen = 0;
    const rocksdb::Snapshot *snapshot = nullptr;
    ZIterator *it = NULL;

    {
        RecordKeyLock l(&mutex_record_, name.String());
        std::string meta_key = encode_meta_key(name);
        int ret = GetZSetMetaVal(meta_key, zv);
        if (ret <= 0) {
            return ret;
        }

        llen = (int) zv.length;
        if (start < 0) start = llen + start;
        if (end < 0) end = llen + end;
        if (start < 0) start = 0;

        /* Invariant: start >= 0, so this test will be true when end < 0.
         * The range is empty when start > end or start >= length. */
        if (start > end || start >= llen) {
            return 1;
        }
        if (end >= llen) end = llen - 1;

        version = zv.version;
        snapshot = GetSnapshot();
    }

    SnapshotPtr spl(ldb, snapshot);

    if (reverse) {
        it = this->zscan_internal(ctx, name, "", "", end + 1, Iterator::BACKWARD, version, snapshot);
    } else {
        it = this->zscan_internal(ctx, name, "", "", end + 1, Iterator::FORWARD, version, snapshot);
    }

    if (it != NULL) {
        it->skip(start);
        while (it->next()) {
            key_score.emplace_back(it->key.String());
            if (withscore) {
                key_score.push_back(str(it->score));
            }
        }
        delete it;
        it = NULL;
    }

    return 1;
}

int SSDBImpl::zrange(Context &ctx, const Bytes &name, const Bytes &begin, const Bytes &limit, bool withscore,
                     std::vector<std::string> &key_score) {
    return zrangeGeneric(ctx, name, begin, limit, withscore, key_score, 0);
}

int SSDBImpl::zrrange(Context &ctx, const Bytes &name, const Bytes &begin, const Bytes &limit, bool withscore,
                      std::vector<std::string> &key_score) {
    return zrangeGeneric(ctx, name, begin, limit, withscore, key_score, 1);
}

/* Struct to hold a inclusive/exclusive range spec by score comparison. */
typedef struct {
    double min, max;
    int minex, maxex; /* are min or max exclusive? */
} zrangespec;

/* Populate the rangespec according to the objects min and max. */
static int zslParseRange(const Bytes &min, const Bytes &max, zrangespec *spec) {
    char *eptr;
    errno = 0;
    spec->minex = spec->maxex = 0;

    /* Parse the min-max interval. If one of the values is prefixed
     * by the "(" character, it's considered "open". For instance
     * ZRANGEBYSCORE zset (1.5 (2.5 will match min < x < max
     * ZRANGEBYSCORE zset 1.5 2.5 will instead match min <= x <= max */
//INVALID_MIN_MAX_DBL

    if (min[0] == '(') {
        spec->min = strtod(min.data() + 1, &eptr);
        if (std::isnan(spec->min)) {
            return NAN_SCORE;
        }
        if (eptr[0] != '\0') {
            return INVALID_MIN_MAX_DBL;
        }

        spec->minex = 1;
    } else {
        spec->min = strtod(min.data(), &eptr);
        if (std::isnan(spec->min)) {
            return INVALID_MIN_MAX_DBL;
        }
        if (eptr[0] != '\0') {
            return INVALID_MIN_MAX_DBL;
        }
    }

    if (max[0] == '(') {
        spec->max = strtod(max.data() + 1, &eptr);
        if (std::isnan(spec->max)) {
            return INVALID_MIN_MAX_DBL;
        }
        if (eptr[0] != '\0') {
            return INVALID_MIN_MAX_DBL;
        }
        spec->maxex = 1;
    } else {
        spec->max = strtod(max.data(), &eptr);
        if (std::isnan(spec->max)) {
            return INVALID_MIN_MAX_DBL;
        }
        if (eptr[0] != '\0') {
            return INVALID_MIN_MAX_DBL;
        }
    }

    return 1;
}

int zslValueGteMin(double value, zrangespec *spec) {
    return spec->minex ? (value > spec->min) : (value >= spec->min);
}

int zslValueLteMax(double value, zrangespec *spec) {
    return spec->maxex ? (value < spec->max) : (value <= spec->max);
}

int SSDBImpl::genericZrangebyscore(Context &ctx, const Bytes &name, const Bytes &start_score, const Bytes &end_score,
                                   std::vector<std::string> &key_score, int withscores, long offset, long limit,
                                   int reverse) {
    zrangespec range;
    std::string score_start;
    std::string score_end;
    double score = 0;

    int ret = 0;
    if (reverse) {
        ret = zslParseRange(end_score, start_score, &range);
        if (ret < 0) {
            return ret;
        }


        score = range.max + eps;
        score_start = str(score);
        score = range.min - eps;
        score_end = str(score);
    } else {
        ret = zslParseRange(start_score, end_score, &range);
        if (ret < 0) {
            return ret;
        }

        if (std::isinf(range.min)) {
            score_start = "";
        } else {
            score = range.min - eps;
            score_start = str(score);
        }

        score = range.min - eps;
        score_start = str(score);

        if (std::isinf(range.max)) {
            score_end = "";
        } else {
            score = range.max + eps;
            score_end = str(score);
        }

    }


    ZSetMetaVal zv;
    const rocksdb::Snapshot *snapshot = nullptr;
    ZIterator *it = NULL;
    {
        RecordKeyLock l(&mutex_record_, name.String());
        std::string meta_key = encode_meta_key(name);
        int ret = GetZSetMetaVal(meta_key, zv);
        if (ret <= 0) {
            return ret;
        }
        snapshot = GetSnapshot();
    }

    SnapshotPtr spl(ldb, snapshot);


    if (reverse) {
        it = this->zscan_internal(ctx, name, score_start, score_end, -1, Iterator::BACKWARD, zv.version, snapshot);
        if (it == NULL) {
            return 1;
        }

        while (it->next()) {
            if (!zslValueLteMax(it->score, &range))
                continue;
            /* Check if score >= min. */
            if (!zslValueGteMin(it->score, &range))
                break;

            if (!offset) {
                if (limit--) {
                    key_score.emplace_back(it->key.String());
                    if (withscores) {
                        key_score.push_back(str(it->score));
                    }
                } else
                    break;
            } else {
                offset--;
            }
        }
        delete it;
        it = NULL;
    } else {
        it = this->zscan_internal(ctx, name, score_start, score_end, -1, Iterator::FORWARD, zv.version, snapshot);
        if (it == NULL) {
            return 1;
        }

        while (it->next()) {
            if (!zslValueGteMin(it->score, &range))
                continue;
            /* Check if score <= max. */
            if (!zslValueLteMax(it->score, &range))
                break;

            if (!offset) {
                if (limit--) {
                    key_score.emplace_back(it->key.String());
                    if (withscores) {
                        key_score.push_back(str(it->score));
                    }
                } else
                    break;
            } else {
                offset--;
            }
        }
        delete it;
        it = NULL;
    }

    return 1;
}

int SSDBImpl::zrangebyscore(Context &ctx, const Bytes &name, const Bytes &start_score, const Bytes &end_score,
                            std::vector<std::string> &key_score, int withscores, long offset, long limit) {
    return genericZrangebyscore(ctx, name, start_score, end_score, key_score, withscores, offset, limit, 0);
}

int SSDBImpl::zrevrangebyscore(Context &ctx, const Bytes &name, const Bytes &start_score, const Bytes &end_score,
                               std::vector<std::string> &key_score, int withscores, long offset, long limit) {
    return genericZrangebyscore(ctx, name, start_score, end_score, key_score, withscores, offset, limit, 1);
}

int SSDBImpl::zremrangebyscore(Context &ctx, const Bytes &name, const Bytes &score_start, const Bytes &score_end,
                               bool remove, int64_t *count) {
    zrangespec range;

    std::string start_score;
    std::string end_score;
    int ret = 0;

    ret = zslParseRange(score_start, score_end, &range);
    if (ret  < 0) {
        return ret;
    }
    double score = range.min - eps;
    start_score = str(score);
    score = range.max + eps;
    end_score = str(score);

    RecordKeyLock l(&mutex_record_, name.String());
    ZSetMetaVal zv;
    std::string meta_key = encode_meta_key(name);
    ret = GetZSetMetaVal(meta_key, zv);
    if (ret <= 0) {
        return ret;
    }

    rocksdb::WriteBatch batch;
    const rocksdb::Snapshot *snapshot = nullptr;
    auto it = std::unique_ptr<ZIterator>(
            this->zscan_internal(ctx, name, start_score, end_score, -1, Iterator::FORWARD, zv.version, snapshot));
    if (it == NULL) {
        return 1;
    }

    while (it->next()) {
        if (!zslValueGteMin(it->score, &range))
            continue;
        /* Check if score <= max. */
        if (!zslValueLteMax(it->score, &range))
            break;

        if (remove) {
            int dret = zdel_one(batch, name, it->key, it->version);
            if (dret < 0) {
                return dret;
            }
        }
        (*count)++;
    }

    ret = 1;

    if (remove) {
        int iret = incr_zsize(ctx, name, batch, zv, -1 * (*count));
        if (iret < 0) {
            return iret;
        } else if (iret == 0) {
            ret = 0;
        }

        rocksdb::Status s = CommitBatch(ctx, &(batch));
        if (!s.ok()) {
            log_error("zdel error: %s", s.ToString().c_str());
            return STORAGE_ERR;
        }
    }

    return ret;
}


int SSDBImpl::zdel_one(rocksdb::WriteBatch &batch, const Bytes &name, const Bytes &key, uint16_t version) {
    double old_score = 0;
    std::string item_key = encode_zset_key(name, key, version);
    int ret = GetZSetItemVal(item_key, &old_score);
    if (ret <= 0) {
        return ret;
    } else {
        //found
        std::string old_score_key = encode_zscore_key(name, key, old_score, version);
        std::string old_zset_key = encode_zset_key(name, key, version);

        batch.Delete(old_score_key);
        batch.Delete(old_zset_key);
    }

    return 1;
}

int
SSDBImpl::incr_zsize(Context &ctx, const Bytes &name, rocksdb::WriteBatch &batch, const ZSetMetaVal &zv, int64_t incr) {
    std::string size_key = encode_meta_key(name);

    int ret = 1;
    if (zv.length == 0) {
        if (incr > 0) {
            std::string meta_val = encode_zset_meta_val((uint64_t) incr, zv.version);
            batch.Put(size_key, meta_val);
        } else if (incr == 0) {
            return 0;
        } else {
            return INVALID_INCR_LEN;
        }
    } else {
        uint64_t len = zv.length;
        if (incr > 0) {
            //TODO 溢出检查
            len += incr;
        } else if (incr < 0) {
            uint64_t u64 = static_cast<uint64_t>(-incr);
            if (len < u64) {
                return INVALID_INCR_LEN;
            }
            len = len - u64;
        } else {
            //incr == 0
            return ret;
        }
        if (len == 0) {
            std::string del_key = encode_delete_key(name, zv.version);
            std::string meta_val = encode_zset_meta_val(zv.length, zv.version, KEY_DELETE_MASK);
            batch.Put(del_key, "");
            batch.Put(size_key, meta_val);

            expiration->cancelExpiration(ctx, name, batch); //del expire ET key

            ret = 0;
        } else {
            std::string meta_val = encode_zset_meta_val(len, zv.version);
            batch.Put(size_key, meta_val);
        }
    }

    log_debug("incr_zsize %s %d", hexstr(name).c_str(), incr);

    return ret;
}


int SSDBImpl::zset_one(rocksdb::WriteBatch &batch, bool needCheck, const Bytes &name, const Bytes &key, double score,
                       uint16_t cur_version, int *flags, double *newscore) {

    /* Turn options into simple to check vars. */
    int incr = (*flags & ZADD_INCR) != 0;
    int nx = (*flags & ZADD_NX) != 0;
    int xx = (*flags & ZADD_XX) != 0;
    *flags = 0; /* We'll return our response flags. */

    /* NaN as input is an error regardless of all the other parameters. */
    if (std::isnan(score)) {
        *flags = ZADD_NAN;
        return NAN_SCORE;
    }


    std::string zkey = encode_zset_key(name, key, cur_version);

    if (needCheck) {
        double old_score = 0;

        int found = GetZSetItemVal(zkey, &old_score);
        if (found < 0) {
            return found;
        }

        if (found == 0) {
            if (!xx) {
                if (newscore) *newscore = score;

                std::string buf((char *) (&score), sizeof(double));
                batch.Put(zkey, buf);
                string score_key = encode_zscore_key(name, key, score, cur_version);
                batch.Put(score_key, "");

                *flags |= ZADD_ADDED;
            } else {
                *flags |= ZADD_NOP;
            }

            return 1;

        } else if (found == 1) {
            if (nx) {
                *flags |= ZADD_NOP;
                return 1;
            }

            if (incr) {
                score += old_score;
                if (std::isnan(score)) {
                    *flags |= ZADD_NAN;
                    return NAN_SCORE;
                }

                if (newscore) *newscore = score;
            }

            if (old_score == score) {
                //same
            } else {
                if (newscore) *newscore = score;

                string old_score_key = encode_zscore_key(name, key, old_score, cur_version);
                batch.Delete(old_score_key);

                std::string buf((char *) (&score), sizeof(double));
                batch.Put(zkey, buf);
                string score_key = encode_zscore_key(name, key, score, cur_version);
                batch.Put(score_key, "");

                *flags |= ZADD_UPDATED;
            }
            return 1;
        } else {
            //error
            return -1;
        }
    } else {

        if (!xx) {
            if (newscore) *newscore = score;

            std::string buf((char *) (&score), sizeof(double));
            batch.Put(zkey, buf);
            string score_key = encode_zscore_key(name, key, score, cur_version);
            batch.Put(score_key, "");

            *flags |= ZADD_ADDED;
        } else {
            *flags |= ZADD_NOP;
        }

        return 1;
    }

}


/* Struct to hold an inclusive/exclusive range spec by lexicographic comparison. */
typedef struct {
    std::string min, max;     /* May be set to shared.(minstring|maxstring) */
    int minex, maxex; /* are min or max exclusive? */
} zlexrangespec;

int zslParseLexRangeItem(const Bytes &item, std::string *dest, int *ex) {
    std::string tmp(item.data() + 1, item.size() - 1);
    switch (item[0]) {
        case '+':
            if (item[1] != '\0') return -1;
            *ex = 0;
            *dest = "";
            return 0;
        case '-':
            if (item[1] != '\0') return -1;
            *ex = 0;
            *dest = "";
            return 0;
        case '(':
            *ex = 1;
            *dest = tmp;
            return 0;
        case '[':
            *ex = 0;
            *dest = tmp;
            return 0;
        default:
            return -1;
    }
}

int zslParseLexRange(const Bytes &min, const Bytes &max, zlexrangespec *spec) {
    /* The range can't be valid if objects are integer encoded.
     * Every item must start with ( or [. */

    spec->min = spec->max = "";
    if (zslParseLexRangeItem(min, &spec->min, &spec->minex) == -1 ||
        zslParseLexRangeItem(max, &spec->max, &spec->maxex) == -1) {
        return SYNTAX_ERR;
    } else {
        return 0;
    }
}

int zslLexValueGteMin(std::string value, zlexrangespec *spec) {
    return spec->minex ?
           (value > spec->min) :
           (value >= spec->min);
}

int zslLexValueLteMax(std::string value, zlexrangespec *spec) {
    if (spec->max == "")
        return 1;
    return spec->maxex ?
           (value < spec->max) :
           (value <= spec->max);
}

int SSDBImpl::genericZrangebylex(Context &ctx, const Bytes &name, const Bytes &key_start, const Bytes &key_end,
                                 std::vector<string> &keys, long offset, long limit, int save, int64_t *count) {
    zlexrangespec range;

    /* Parse the range arguments */
    if (zslParseLexRange(key_start, key_end, &range) != 0) {
        return ZSET_INVALID_STR;
    }
    if ((key_start[0] == '+') || (key_end[0] == '-') ||
        (range.min > range.max && key_end[0] != '+') ||
        (range.min == "" && range.max == "" && key_end[0] != '+') ||
        (range.min == range.max && range.max != "" && (range.minex || range.maxex))) {
        return 1;
    }

    ZSetMetaVal zv;
    const rocksdb::Snapshot *snapshot = nullptr;
    {
        RecordKeyLock l(&mutex_record_, name.String());
        std::string meta_key = encode_meta_key(name);
        int ret = GetZSetMetaVal(meta_key, zv);
        if (ret <= 0) {
            return ret;
        }
        snapshot = GetSnapshot();
    }
    SnapshotPtr spl(ldb, snapshot); //auto release

    auto it = std::unique_ptr<ZIteratorByLex>(
            this->zscanbylex_internal(ctx, name, range.min, range.max, -1, Iterator::FORWARD, zv.version, snapshot));

    while (it->next()) {
        if (!zslLexValueGteMin(it->key.String(), &range))
            continue;
        if (!zslLexValueLteMax(it->key.String(), &range))
            break;

        (*count)++;

        if (save) {
            if (!offset) {
                if (limit--) {
                    keys.emplace_back(it->key.String());
                } else
                    break;
            } else {
                offset--;
            }
        }
    }

    return 1;
}

int SSDBImpl::zlexcount(Context &ctx, const Bytes &name, const Bytes &key_start, const Bytes &key_end, int64_t *count) {
    std::vector<string> keys;
    return genericZrangebylex(ctx, name, key_start, key_end, keys, 0, -1, 0, count);
}

int SSDBImpl::zrangebylex(Context &ctx, const Bytes &name, const Bytes &key_start, const Bytes &key_end,
                          std::vector<std::string> &keys, long offset, long limit) {
    int64_t count = 0;
    return genericZrangebylex(ctx, name, key_start, key_end, keys, offset, limit, 1, &count);
}

int SSDBImpl::zremrangebylex(Context &ctx, const Bytes &name, const Bytes &key_start, const Bytes &key_end,
                             int64_t *count) {
    zlexrangespec range;

    /* Parse the range arguments */
    if (zslParseLexRange(key_start, key_end, &range) != 0) {
        return SYNTAX_ERR;
    }
    if ((key_start[0] == '+') || (key_end[0] == '-') ||
        (range.min > range.max && key_end[0] != '+') ||
        (range.min == "" && range.max == "" && key_end[0] != '+') ||
        (range.min == range.max && range.max != "" && (range.minex || range.maxex))) {
        return 1;
    }

    RecordKeyLock l(&mutex_record_, name.String());
    ZSetMetaVal zv;
    std::string meta_key = encode_meta_key(name);
    int ret = GetZSetMetaVal(meta_key, zv);
    if (ret <= 0) {
        return ret;
    }

    rocksdb::WriteBatch batch;
    const rocksdb::Snapshot *snapshot = nullptr;
    auto it = std::unique_ptr<ZIteratorByLex>(
            this->zscanbylex_internal(ctx, name, range.min, range.max, -1, Iterator::FORWARD, zv.version, snapshot));

    while (it->next()) {
        if (zslLexValueGteMin(it->key.String(), &range)) {
            if (zslLexValueLteMax(it->key.String(), &range)) {
                (*count)++;
                ret = zdel_one(batch, name, it->key, it->version);
                if (ret < 0) {
                    return ret;
                }
            } else
                break;
        }
    }

    int iret = incr_zsize(ctx, name, batch, zv, -1 * (*count));
    if (iret < 0) {
        return iret;
    } else if (iret == 0) {
        ret = 0;
    } else {
        ret = 1;
    }


    rocksdb::Status s = CommitBatch(ctx, &(batch));
    if (!s.ok()) {
        log_error("zdel error: %s", s.ToString().c_str());
        return STORAGE_ERR;
    }

    return ret;
}

int SSDBImpl::zrevrangebylex(Context &ctx, const Bytes &name, const Bytes &key_start, const Bytes &key_end,
                             std::vector<std::string> &keys, long offset, long limit) {
    zlexrangespec range;
    int count = 0;

    /* Parse the range arguments */
    if (zslParseLexRange(key_end, key_start, &range) != 0) {
        return ZSET_INVALID_STR;
    }
    /*
     * in some of the following special cases, return nil
     * { zrevrangebylex zkey - + } / { zrevrangebylex zkey - (g } / { zrevrangebylex zkey [f + }
     * { zrevrangebylex zkey [a (f } / but except { zrevrangebylex zkey + (f } even if range.min > range.max
     * { zrevrangebylex zkey [ ( } / but except { zrevrangebylex zkey + ( } even if range.min == range.max == ""
     * { zrevrangebylex zkey (f (f } / but except { zrevrangebylex zkey + ( }
     */
    if ((key_start[0] == '-') || (key_end[0] == '+') ||
        (range.min > range.max && key_start[0] != '+') ||
        (range.min == "" && range.max == "" && key_start[0] != '+') ||
        (range.min == range.max && range.max != "" && (range.minex || range.maxex))) {
        return 1;
    }

    ZSetMetaVal zv;
    const rocksdb::Snapshot *snapshot = nullptr;
    {
        RecordKeyLock l(&mutex_record_, name.String());
        std::string meta_key = encode_meta_key(name);
        int ret = GetZSetMetaVal(meta_key, zv);
        if (ret <= 0) {
            return ret;
        }
        snapshot = GetSnapshot();
    }
    SnapshotPtr spl(ldb, snapshot); //auto release

    auto it = std::unique_ptr<ZIteratorByLex>(
            this->zscanbylex_internal(ctx, name, range.min, range.max, -1, Iterator::FORWARD, zv.version, snapshot));

    std::stack<std::string> tmp_stack;
    while (it->next()) {
        if (zslLexValueGteMin(it->key.String(), &range)) {
            if (zslLexValueLteMax(it->key.String(), &range)) {
                count++;
                tmp_stack.push(it->key.String());
            } else
                break;
        }
    }

    if (count > 0) {
        while (!tmp_stack.empty()) {
            if (!offset) {
                if (limit--) {
                    std::string &top = tmp_stack.top();
                    keys.push_back(top);
                    tmp_stack.pop();
                } else
                    break;
            } else {
                tmp_stack.pop();
                offset--;
            }
        }
    }

    return 1;
}


int SSDBImpl::zscan(Context &ctx, const Bytes &name, const Bytes &cursor, const std::string &pattern, uint64_t limit,
                    std::vector<std::string> &resp) {

    ZSetMetaVal hv;
    std::string meta_key = encode_meta_key(name);
    int ret = GetZSetMetaVal(meta_key, hv);
    if (ret != 1) {
        return ret;
    }

    resp.emplace_back("0");


    std::string start;
    if (cursor == "0") {
//        start = encode_zset_key(name, "", hv.version);
        start = encode_zscore_prefix(name, hv.version);
    } else {
        redisCursorService.FindElementByRedisCursor(cursor.String(), start);
    }

    Iterator *iter = this->iterator(start, "", -1);
    auto mit = std::unique_ptr<ZIterator>(new ZIterator(iter, name, hv.version));

    bool end = doScanGeneric<ZIterator>(mit.get(), pattern, limit, resp);

    if (!end) {
        //get new;
        uint64_t tCursor = redisCursorService.GetNewRedisCursor(iter->key().String()); //we already got it->next
        resp[0] = str(tCursor);
    }

    return 1;
}