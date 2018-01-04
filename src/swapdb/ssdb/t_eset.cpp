/*
Copyright (c) 2017, Timothy. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/

#include "ssdb_impl.h"


int SSDBImpl::edel_one(ClientContext &ctx, const Bytes &key,rocksdb::WriteBatch &batch) {

    int64_t old_ts = 0;

    int found = eget(ctx, key, &old_ts);
    if (found == -1) {
        return -1;
    } else if (found == 0) {
        return 0;
    } else {
        std::string old_score_key = encode_escore_key(key, static_cast<uint64_t>(old_ts));
        std::string old_eset_key = encode_eset_key(key);

        batch.Delete(old_score_key);
        batch.Delete(old_eset_key);
    }

    return 1;
}

int SSDBImpl::eget(ClientContext &ctx, const Bytes &key, int64_t *ts) {
    *ts = 0;

    std::string str_score;
    std::string dbkey = encode_eset_key(key);
//    bool found = true;

    rocksdb::Status s;


    s = ldb->Get(commonRdOpt, dbkey, &str_score);

    if (s.IsNotFound()) {
        return 0;
    }
    if (!s.ok()) {
        log_error("%s", s.ToString().c_str());
        return -1;
    }

    *ts = *((uint64_t *) (str_score.data())); // int64_t -> uint64_t
    return 1;
}



int SSDBImpl::eset_one(ClientContext &ctx, const Bytes &key, rocksdb::WriteBatch &batch, int64_t ts_ms) {

    int ret = 1;

    int64_t old_ts = 0;

    int found = this->eget(ctx, key, &old_ts);
    if (found == -1) {
        return -1;
    }

    if (found == 0) {
        //skip
    } else if (found == 1) {
        if (old_ts == ts_ms) {
            //same
        } else {
            string old_score_key = encode_escore_key(key, static_cast<uint64_t>(old_ts));
            batch.Delete(old_score_key);
        }
    } else {
        //error
        return -1;
    }

    string ekey = encode_eset_key(key);

    string ekey_value((char *) (&ts_ms), sizeof(int64_t));
    string score_key = encode_escore_key(key, static_cast<uint64_t>(ts_ms));

    batch.Put(ekey, ekey_value);
    batch.Put(score_key, "");

    return ret;

}

int SSDBImpl::check_meta_key(ClientContext &ctx, const Bytes &key) {
    std::string meta_key = encode_meta_key(key);
    std::string meta_val;
    rocksdb::Status s = ldb->Get(rocksdb::ReadOptions(), meta_key, &meta_val);
    if (s.IsNotFound()) {
        return 0;
    } else if (!s.ok()) {
        return -1;
    } else {
        if (meta_val.size() >= 4) {
            if (meta_val[3] == KEY_DELETE_MASK) {
                return 0;
            }
        } else {
            return -1;
        }
    }
    return 1;
}
