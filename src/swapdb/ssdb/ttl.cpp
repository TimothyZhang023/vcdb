/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "ttl.h"
#include "ssdb_impl.h"

const int BATCH_SIZE = 1000;

#define CHECK_DISABLD_EXPIRE  if (!expire_enable) {return 1;}

ExpirationHandler::ExpirationHandler(SSDBImpl *ssdb, bool expire_enable) {
    this->ssdb = ssdb;
    this->thread_quit = false;
//	this->list_name = EXPIRATION_LIST_KEY;
    this->expire_enable = expire_enable;
    this->first_timeout = 0;
    this->start();
}

ExpirationHandler::~ExpirationHandler() {
    this->clear();
    this->stop();
    ssdb = nullptr;
}

int ExpirationHandler::start() {

    {
//        Locking<Mutex> exl(&this->mutex);
        CHECK_DISABLD_EXPIRE
    }


    thread_quit = false;

    this->clear();

    first_timeout = 0;
    pthread_t tid;
    int err = pthread_create(&tid, nullptr, &ExpirationHandler::_thread_func, this);
    if (err != 0) {
        log_fatal("can't create thread: %s", strerror(err));
        exit(0);
    }

    return 0;
}

int ExpirationHandler::stop() {

    {
//        Locking<Mutex> exl(&this->mutex);
        CHECK_DISABLD_EXPIRE
    }


    log_info("ExpirationHandler stopping");

    thread_quit = true;
    for (int i = 0; i < 1000; i++) {
        if (!thread_quit) {
            break;
        }
        log_info("waiting for expiration stop");
        usleep(1000 * 1000);
    }

    this->clear();
    first_timeout = 0;

    return 0;
}

int ExpirationHandler::convert2ms(int64_t *ttl, TimeUnit tu) {
    if (tu == TimeUnit::Second) {
        if (INT64_MAX / 1000 <= *ttl) { return INVALID_EX_TIME; }
        *ttl = *ttl * 1000;
    } else if (tu == TimeUnit::Millisecond) {
        if (INT64_MAX <= *ttl) { return INVALID_EX_TIME; }
        //nothing
    } else {
        assert(0);
        return -1;
    }

    return 1;
}

int ExpirationHandler::expire(ClientContext &ctx, const Bytes &key, int64_t ttl, TimeUnit tu) {
    int ret = convert2ms(&ttl,tu);
    if (ret < 0) {
        return ret;
    }

    int64_t pexpireat = time_ms() + ttl;

    return expireAt(ctx, key, pexpireat);
}


int ExpirationHandler::expireAt(ClientContext &ctx, const Bytes &key, int64_t pexpireat_ms, rocksdb::WriteBatch &batch, bool lock) {

    RecordKeyLock l(&ssdb->mutex_record_, key.String(), lock);

    CHECK_DISABLD_EXPIRE

    int ret = 0;

    if (pexpireat_ms <= time_ms()) {

        ret = ssdb->del_key_internal(ctx, key, batch);

    } else {

        if (lock) {
            ret = ssdb->check_meta_key(ctx, key);
            if (ret <= 0) {
                return ret;
            }
        }

        ret = ssdb->eset_one(ctx, key, batch, pexpireat_ms);
        if (ret <= 0) {
            return ret;
        }

        _insertFastKey(key.String(), pexpireat_ms);

    }

    if (lock) {
        if (ret >= 0) {
            rocksdb::Status s = ssdb->CommitBatch(ctx, &(batch));
            if (!s.ok()) {
                log_error("expireAt CommitBatch error: %s", s.ToString().c_str());
                return STORAGE_ERR;
            }
        }
    }


    return 2;

}

void ExpirationHandler::_insertFastKey(const std::string &s_key, int64_t pexpireat_ms) {
    Locking<Mutex> exl(&mutex);
    if (pexpireat_ms < first_timeout) {
        first_timeout = pexpireat_ms;
    }
    if (!fast_keys.empty() && pexpireat_ms <= fast_keys.max_score()) {
        fast_keys.add(s_key, pexpireat_ms);
        if (fast_keys.size() > BATCH_SIZE) {
            fast_keys.pop_back();
        }
    } else {
        fast_keys.del(s_key);
        //log_debug("don't put in fast_keys");
    }
}


int ExpirationHandler::persist(ClientContext &ctx, const Bytes &key) {

    {
        Locking<Mutex> exl(&this->mutex);
        CHECK_DISABLD_EXPIRE
    }


    int ret = 0;
    if (first_timeout != INT64_MAX) {
        RecordKeyLock l(&ssdb->mutex_record_, key.String());
        rocksdb::WriteBatch batch;

        ret = cancelExpiration(ctx, key, batch);
        if (ret >= 0) {
            rocksdb::Status s = ssdb->CommitBatch(ctx, &(batch));
            if (!s.ok()) {
                log_error("edel error: %s", s.ToString().c_str());
                return -1;
            }
        }

    }
    return ret;
}

int64_t ExpirationHandler::pttl(ClientContext &ctx, const Bytes &key, TimeUnit tu) {

    {
        Locking<Mutex> exl(&this->mutex);
        CHECK_DISABLD_EXPIRE
    }


    int64_t ex = 0;
    int64_t ttl = 0;
    int ret = ssdb->check_meta_key(ctx, key);
    if (ret <= 0) {
        return -2;
    }
    ret = ssdb->eget(ctx, key, &ex);
    if (ret == 1) {
        ttl = ex - time_ms();
        if (ttl < 0) return -2;

        if (tu == TimeUnit::Second) {
            return (ttl + 500) / 1000;
        } else if (tu == TimeUnit::Millisecond) {
            return (ttl);
        } else {
            assert(0);
            return -1;
        }

    }
    return -1;
}

void ExpirationHandler::_load_expiration_keys_from_db(int num) {

    std::string start;
    start.append(1, DataType::ESCORE);

    auto it = std::unique_ptr<EIterator>(new EIterator(ssdb->iterator(start, "", num))); //  +
    int n = 0;
    while (it->next()) {
        n++;
        std::string key = it->key.String();
        int64_t score = it->score;
        {
            Locking<Mutex> exl(&this->mutex);
            fast_keys.add(key, score);
        }
    }

    log_debug("load %d keys into fast_keys", n);
}

void ExpirationHandler::_expire_loop() {

    bool needLoad = false;
    {
        Locking<Mutex> exl(&this->mutex);
        if (!this->ssdb) {
            return;
        }

        if (this->fast_keys.empty()) {
            needLoad = true;

        }
    }

    if (needLoad) {
        this->_load_expiration_keys_from_db(BATCH_SIZE);

        {
            Locking<Mutex> exl(&this->mutex);
            if (this->fast_keys.empty()) {
                this->first_timeout = INT64_MAX;
                return;
            }

        }

    }

    int64_t score;
    std::string key;


    std::set<std::string> keys;

    {
        Locking<Mutex> exl(&this->mutex);
        int count = 0;
        while (this->fast_keys.front(&key, &score)) {
            this->first_timeout = score;
            count++;
            int64_t latency = time_ms() - score;

            if (latency >= 0) {
                log_debug("expired %s latency %d", hexstr(key).c_str(), latency);
                keys.insert(key);
                this->fast_keys.pop_front();
            } else {
                break;
            }

            if (count > 101) break;
        }
    }

    ClientContext ctx;

    int64_t num = 0;
    ssdb->multi_del(ctx, keys, &num);

}

void *ExpirationHandler::_thread_func(void *arg) {
    ExpirationHandler *handler = (ExpirationHandler *) arg;

    while (!handler->thread_quit) {
        if (handler->first_timeout > time_ms()) {
            usleep(10 * 1000);
            continue;
        }

        handler->_expire_loop();
    }

    log_info("ExpirationHandler thread quit");
    handler->thread_quit = false;
    return (void *) nullptr;
}

int ExpirationHandler::cancelExpiration(ClientContext &ctx, const Bytes &key, rocksdb::WriteBatch &batch) {
    Locking<Mutex> exl(&this->mutex);

    CHECK_DISABLD_EXPIRE

    this->fast_keys.del(key.String());

    return ssdb->edel_one(ctx, key, batch);
}

int ExpirationHandler::clear() {
    Locking<Mutex> exl(&this->mutex);

    CHECK_DISABLD_EXPIRE

    fast_keys.clear();

    return 0;
}

int ExpirationHandler::contrlExpiration(bool if_enable) {
    Locking<Mutex> exl(&this->mutex);

    if (if_enable) {
        if (!expire_enable) {
            expire_enable= true;
            start();
        }

    } else {

        if (expire_enable) {
            expire_enable= false;
            stop();
        }

    }
    return 0;
}

