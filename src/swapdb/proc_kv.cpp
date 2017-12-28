/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
/* kv */
#include "serv.h"


int proc_type(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(2);

    std::string val;
    int ret = serv->db->type(ctx, req[1], &val);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    }

    resp->addReplyStatus(val);

    return 0;
}

int proc_get(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(2);

    std::string val;
    int ret = serv->db->get(ctx, req[1], &val);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else if (ret == 0) {
        resp->addReplyNil();
    } else {
        resp->addReplyString(val);
    }

    return 0;
}

int proc_getset(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(3);

    std::pair<std::string, bool> val;
    int ret = serv->db->getset(ctx, req[1], val, req[2]);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        if (val.second) {
            resp->addReplyString(val.first);
        } else {
            resp->addReplyNil();
        }
    }

    return 0;
}

int proc_append(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(3);

    uint64_t newlen = 0;
    int ret = serv->db->append(ctx, req[1], req[2], &newlen);
    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyInt(newlen);
    }
    return 0;
}

int proc_set(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(3);

    int64_t ttl = 0;

    TimeUnit tu = TimeUnit::Second;
    int flags = OBJ_SET_NO_FLAGS;
    if (req.size() > 3) {
        for (int i = 3; i < req.size(); ++i) {
            std::string key = req[i].String();
            strtolower(&key);

            if (key == "nx" && !(flags & OBJ_SET_XX)) {
                flags |= OBJ_SET_NX;
            } else if (key == "xx" && !(flags & OBJ_SET_NX)) {
                flags |= OBJ_SET_XX;
            } else if (key == "ex" && !(flags & OBJ_SET_PX)) {
                flags |= OBJ_SET_EX;
                tu = TimeUnit::Second;
            } else if (key == "px" && !(flags & OBJ_SET_EX)) {
                flags |= OBJ_SET_PX;
                tu = TimeUnit::Millisecond;
            } else {
                addReplyErrorCodeReturn(SYNTAX_ERR);
            }

            if (key == "nx" || key == "xx") {
                //nothing
            } else if (key == "ex" || key == "px") {
                i++;
                if (i >= req.size()) {
                    addReplyErrorCodeReturn(SYNTAX_ERR);

                } else {
                    ttl = Bytes(req[i]).Int64();
                    if (errno == EINVAL) {
                        addReplyErrorCodeReturn(INVALID_INT);
                    }

                    if (ttl <= 0) {
                        addReplyErrorCodeReturn(INVALID_EX_TIME);
                    }

                }
            } else {
                addReplyErrorCodeReturn(SYNTAX_ERR);

            }
        }
    }

    int t_ret = serv->db->expiration->convert2ms(&ttl, tu);
    if (t_ret < 0) {
        addReplyErrorCodeReturn(t_ret);
    }


    int added = 0;
    int ret = serv->db->set(ctx, req[1], req[2], flags, ttl, &added);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        if (flags & OBJ_SET_NX) {
            if (added == 0) {
                resp->addReplyNil();
                return 0;
            }
        } else if (flags & OBJ_SET_XX) {
            if (added == 0) {
                resp->addReplyNil();
                return 0;
            }
        }

        resp->addReplyStatusOK();
    }

    return 0;
}

int proc_setnx(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(3);

    int added = 0;
    int ret = serv->db->set(ctx, req[1], req[2], OBJ_SET_NX, 0, &added);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyInt(added);
    }

    return 0;
}

int proc_setx(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(4);


    int64_t ttl = req[2].Int64();
    if (errno == EINVAL) {
        addReplyErrorCodeReturn(INVALID_INT);
    }

    if (ttl <= 0) {
        addReplyErrorCodeReturn(INVALID_EX_TIME);
    }

    int t_ret = serv->db->expiration->convert2ms(&ttl, TimeUnit::Second);
    if (t_ret < 0) {
        addReplyErrorCodeReturn(t_ret);
    }

    int added = 0;
    int ret = serv->db->set(ctx, req[1], req[3], OBJ_SET_EX, ttl, &added);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyStatusOK();
    }

    return 0;
}

int proc_psetx(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(4);

    int64_t ttl = req[2].Int64();
    if (errno == EINVAL) {
        addReplyErrorCodeReturn(INVALID_INT);
    }

    if (ttl <= 0) {
        addReplyErrorCodeReturn(INVALID_EX_TIME);
    }

    int t_ret = serv->db->expiration->convert2ms(&ttl, TimeUnit::Millisecond);
    if (t_ret < 0) {
        addReplyErrorCodeReturn(t_ret);
    }


    int added = 0;
    int ret = serv->db->set(ctx, req[1], req[3], OBJ_SET_PX, ttl, &added);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyStatusOK();
    }


    return 0;
}

int proc_pttl(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(2);

    int64_t ttl = serv->db->expiration->pttl(ctx, req[1], TimeUnit::Millisecond);
    if (ttl == -2) {

    }
    resp->addReplyInt(ttl);

    return 0;
}

int proc_ttl(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(2);

    int64_t ttl = serv->db->expiration->pttl(ctx, req[1], TimeUnit::Second);
    if (ttl == -2) {

    }
    resp->addReplyInt(ttl);

    return 0;
}

int proc_pexpire(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(3);

    long long when;
    if (string2ll(req[2].data(), (size_t) req[2].size(), &when) == 0) {
        addReplyErrorCodeReturn(INVALID_INT);
    }

    std::string val;
    int ret = serv->db->expiration->expire(ctx, req[1], (int64_t) when, TimeUnit::Millisecond);
    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyInt(ret);
    }

    return 0;
}

int proc_expire(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(3);

    long long when;
    if (string2ll(req[2].data(), (size_t) req[2].size(), &when) == 0) {
        addReplyErrorCodeReturn(INVALID_INT);
    }

    std::string val;
    int ret = serv->db->expiration->expire(ctx, req[1], (int64_t) when, TimeUnit::Second);
    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyInt(ret == 0 ? 0 : 1);
    }

    return 0;
}

int proc_expireat(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(3);

    long long ts_ms;
    if (string2ll(req[2].data(), (size_t) req[2].size(), &ts_ms) == 0) {
        addReplyErrorCodeReturn(INVALID_INT);
    }

    std::string val;
    int ret = serv->db->expiration->expireAt(ctx, req[1], (int64_t) ts_ms * 1000);
    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyInt(ret);
    }
    return 0;
}

int proc_persist(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(2);

    std::string val;
    int ret = serv->db->expiration->persist(ctx, req[1]);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyInt(ret);
    }
    return 0;
}

int proc_pexpireat(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(3);

    long long ts_ms;
    if (string2ll(req[2].data(), (size_t) req[2].size(), &ts_ms) == 0) {
        addReplyErrorCodeReturn(INVALID_INT);
    }

    std::string val;
    int ret = serv->db->expiration->expireAt(ctx, req[1], (int64_t) ts_ms);
    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyInt(ret);
    }

    return 0;
}

int proc_exists(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(2);

    uint64_t count = 0;
    for_each(req.begin() + 1, req.end(), [&](Bytes key) {
        int ret = serv->db->exists(ctx, key);
        if (ret == 1) {
            count++;
        }
    });

    if (count < (req.size() - 1)) {

    }

    resp->addReplyInt(count);

    return 0;
}

int proc_multi_set(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    if (req.size() < 3 || req.size() % 2 != 1) {
        addReplyErrorInfoReturn("ERR wrong number of arguments for MSET");
    } else {
        int ret = serv->db->multi_set(ctx, req, 1);
        if (ret < 0) {
            addReplyErrorCodeReturn(ret);
        } else {
            resp->addReplyInt((uint64_t) ret);
        }
    }
    return 0;
}

int proc_multi_del(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(2);

    std::set<std::string> distinct_keys;

    for_each(req.begin() + 1, req.end(), [&](Bytes b) {
        distinct_keys.emplace(b.String());
    });

    int64_t num = 0;
    int ret = serv->db->multi_del(ctx, distinct_keys, &num);
    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyInt(num);
    }

    return 0;
}

int proc_multi_get(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(2);

    resp->addReplyListHead(static_cast<int>(req.size() -1));

    for (int i = 1; i < req.size(); i++) {
        std::string val;
        int ret = serv->db->get(ctx, req[i], &val);
        if (ret < 1) {
            resp->addReplyNil();
        } else {
            resp->addReplyString(val);
        }
    }

    return 0;
}


int proc_scan(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(2);

    int cursorIndex = 1;

    Bytes cursor = req[cursorIndex];

    cursor.Uint64();
    if (errno == EINVAL) {
        addReplyErrorCodeReturn(INVALID_INT);
    }

    ScanParams scanParams;

    int ret = prepareForScanParams(req, cursorIndex + 1, scanParams);
    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    }

    serv->db->scan(cursor, scanParams.pattern, scanParams.limit, resp->resp_arr);

    resp->convertReplyToScanResult();

    return 0;
}

int proc_ssdb_dbsize(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;

    uint64_t count = 0;

    std::string start;
    start.append(1, DataType::META);
    auto ssdb_it = std::unique_ptr<Iterator>(serv->db->iterator(start, "", -1));
    while (ssdb_it->next()) {
        Bytes ks = ssdb_it->key();

        if (ks.data()[0] != DataType::META) {
            break;
        }

        count++;
    }

    resp->addReplyInt(count);
    return 0;
}

int proc_ssdb_scan(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(2);

    uint64_t limit = 1000;

    std::string pattern = "*";
    bool need_value = false;
    std::vector<Bytes>::const_iterator it = req.begin() + 2;
    for (; it != req.end(); it += 2) {
        std::string key = (*it).String();
        strtolower(&key);

        if (key == "match") {
            pattern = (*(it + 1)).String();
        } else if (key == "count") {
            limit = (*(it + 1)).Uint64();
            if (errno == EINVAL) {
                addReplyErrorCodeReturn(INVALID_INT);
            }
        } else if (key == "value") {
            std::string has_value_s = (*(it + 1)).String();
            strtolower(&has_value_s);

            need_value = (has_value_s == "on");
        } else {
            addReplyErrorCodeReturn(SYNTAX_ERR);
        }
    }
    bool fulliter = (pattern == "*");

    auto resp_arr = &resp->resp_arr;


    auto ssdb_it = std::unique_ptr<Iterator>(serv->db->iterator("", "", -1));
    while (ssdb_it->next()) {

        if (fulliter ||
            stringmatchlen(pattern.data(), pattern.length(), ssdb_it->key().data(), ssdb_it->key().size(), 0)) {
            resp_arr->emplace_back(hexmem(ssdb_it->key().data(), ssdb_it->key().size()));
            if (need_value) {
                resp_arr->emplace_back(hexmem(ssdb_it->val().data(), ssdb_it->val().size()));
            }

            limit--;
            if (limit == 0) {
                break; //stop now
            }

        } else {
            //skip
        }

    }

    resp->convertReplyToList();


    return 0;
}

int proc_keys(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;

    auto resp_arr = &resp->resp_arr;

    std::string pattern = "*";
    if (req.size() > 1) {
        pattern = req[1].String();
    }

    bool fulliter = (pattern == "*");

    std::string start;
    start.append(1, DataType::META);

    auto mit = std::unique_ptr<MIterator>(new MIterator(serv->db->iterator(start, "", -1)));
    while (mit->next()) {
        if (fulliter || stringmatchlen(pattern.data(), pattern.size(), mit->key.data(), mit->key.size(), 0)) {
            resp_arr->emplace_back(mit->key.String());
        } else {
            //skip
        }

    }

    resp->convertReplyToList();

    return 0;
}

// dir := +1|-1
static int _incr(Context &ctx, SSDB *ssdb, const Request &req, Response *resp, int dir) {
    CHECK_MIN_PARAMS(2);
    int64_t by = 1;
    if (req.size() > 2) {
        by = req[2].Int64();
        if (errno == EINVAL) {
            addReplyErrorCodeReturn(INVALID_INT);
        }
    }
    int64_t new_val;
    int ret = ssdb->incr(ctx, req[1], dir * by, &new_val);
    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyInt(new_val);
    }
    return 0;
}

int proc_incrbyfloat(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;

    CHECK_MIN_PARAMS(3);
    long double by = req[2].LDouble();
    if (errno == EINVAL) {
        addReplyErrorCodeReturn(INVALID_INT);
    }

    long double new_val = 0.0L;

    int ret = serv->db->incrbyfloat(ctx, req[1], by, &new_val);
    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyHumanLongDouble(new_val);
    }

    return 0;
}

int proc_incr(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    return _incr(ctx, serv->db, req, resp, 1);
}

int proc_decr(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    return _incr(ctx, serv->db, req, resp, -1);
}

int proc_getbit(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(3);
    long long offset = 0;
    string2ll(req[2].data(), (size_t) req[2].size(), &offset);
    if (offset < 0 || ((uint64_t) offset >> 3) >= MAX_PACKET_SIZE * 4) {
        std::string msg = "ERR offset is is not an integer or out of range";
        addReplyErrorInfoReturn(msg);
    }

    int res = 0;
    int ret = serv->db->getbit(ctx, req[1], (int64_t) offset, &res);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyInt(res);
    }

    return 0;
}

int proc_setbit(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(4);

    const Bytes &name = req[1];
    long long offset = 0;
    string2ll(req[2].data(), (size_t) req[2].size(), &offset);

    int on = req[3].Int();
    if (on & ~1) {
        addReplyErrorInfoReturn("ERR bit is not an integer or out of range");
    }
    if (offset < 0 || ((uint64_t) offset >> 3) >= MAX_PACKET_SIZE * 4) {
        addReplyErrorInfoReturn("ERR offset is out of range [0, 4294967296)");
    }

    int res = 0;
    int ret = serv->db->setbit(ctx, name, (int64_t) offset, on, &res);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else if (res == 0) {
        resp->addReplyInt(0);
    } else {
        resp->addReplyInt(1);
    }

    return 0;
}

int proc_countbit(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(2);

    const Bytes &key = req[1];
    int start = 0;
    if (req.size() > 2) {
        start = req[2].Int();
    }
    std::string val;
    int ret = serv->db->get(ctx, key, &val);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        std::string str;
        int size = -1;
        if (req.size() > 3) {
            size = req[3].Int();
            if (errno == EINVAL) {
                addReplyErrorCodeReturn(INVALID_INT);
            }

            str = substr(val, start, size);
        } else {
            str = substr(val, start, val.size());
        }
        int count = bitcount(str.data(), str.size());
        resp->addReplyInt((uint64_t) count);
    }
    return 0;
}

int proc_bitcount(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(2);

    const Bytes &name = req[1];
    int start = 0;
    if (req.size() > 2) {
        start = req[2].Int();
        if (errno == EINVAL) {
            addReplyErrorCodeReturn(INVALID_INT);
        }
    }
    int end = -1;
    if (req.size() > 3) {
        end = req[3].Int();
        if (errno == EINVAL) {
            addReplyErrorCodeReturn(INVALID_INT);
        }
    }
    std::string val;
    int ret = serv->db->get(ctx, name, &val);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        std::string str = str_slice(val, start, end);
        int count = bitcount(str.data(), str.size());
        resp->addReplyInt(count);
    }
    return 0;
}


int proc_getrange(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(4);

    const Bytes &name = req[1];
    int64_t start = req[2].Int64();
    if (errno == EINVAL) {
        addReplyErrorCodeReturn(INVALID_INT);
    }


    int64_t end = req[3].Int64();
    if (errno == EINVAL) {
        addReplyErrorCodeReturn(INVALID_INT);
    }

    std::pair<std::string, bool> val;
    int ret = serv->db->getrange(ctx, name, start, end, val);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyString(val.first);
    }
    return 0;
}


int proc_substr(Context &ctx, const Request &req, Response *resp) {
    return proc_getrange(ctx, req, resp);
}


int proc_setrange(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(4);

    int64_t start = req[2].Int64();
    if (errno == EINVAL) {
        addReplyErrorCodeReturn(INVALID_INT);
    }

    if (start < 0) {
        addReplyErrorCodeReturn(INDEX_OUT_OF_RANGE);
    }

    uint64_t new_len = 0;

    int ret = serv->db->setrange(ctx, req[1], start, req[3], &new_len);
    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyInt(new_len);
    }
    return 0;
}

int proc_strlen(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(2);

    const Bytes &name = req[1];
    std::string val;
    int ret = serv->db->get(ctx, name, &val);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyInt((int64_t) val.size());
    }
    return 0;
}
