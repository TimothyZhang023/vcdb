/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
/* hash */
#include "serv.h"

int proc_hexists(Context &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(3);
    VcServer *serv = ctx.serv;

    const Bytes &name = req[1];
    const Bytes &key = req[2];
    std::pair<std::string, bool> val;
    int ret = serv->db->hget(ctx, name, key, val);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else if (ret == 0) {
        resp->addReplyInt(0);
    } else {
        if (val.second) {
            resp->addReplyInt(1);
        } else {
            resp->addReplyInt(0);
        }
    }

    return 0;
}


int proc_hmset(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    if (req.size() < 4 || req.size() % 2 != 0) {
        addReplyErrorInfoReturn("ERR wrong number of arguments for 'hmset' command");
    }

    std::map<Bytes, Bytes> kvs;
    const Bytes &name = req[1];
    auto it = req.begin() + 2;
    for (; it != req.end(); it += 2) {
        const auto &key = *it;
        const auto &val = *(it + 1);
        kvs[key] = val;
    }

    int ret = serv->db->hmset(ctx, name, kvs);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyStatusOK();
    }

    return 0;
}

int proc_hdel(Context &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(3);
    VcServer *serv = ctx.serv;

    const Bytes &name = req[1];

    std::set<Bytes> fields;
    for_each(req.begin() + 2, req.end(), [&](Bytes b) {
        fields.insert(b);
    });

    int deleted = 0;
    int ret = serv->db->hdel(ctx, name, fields, &deleted);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    }

    resp->addReplyInt(deleted);
    return 0;
}

int proc_hmget(Context &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(3);
    VcServer *serv = ctx.serv;

    const Bytes &name = req[1];


    std::vector<std::string> reqKeys;
    std::map<std::string, std::string> resMap;

    for_each(req.begin() + 2, req.end(), [&](Bytes b) {
        reqKeys.emplace_back(b.String());
    });


    int ret = serv->db->hmget(ctx, name, reqKeys, resMap);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);

    } else {

        resp->addReplyListHead(static_cast<int>(reqKeys.size()));

        for (const auto &reqKey : reqKeys) {
            auto pos = resMap.find(reqKey);
            if (pos == resMap.end()) {
                resp->addReplyNil();
            } else {
                resp->addReplyString(pos->second);
            }
        }

    }

    return 0;
}

int proc_hlen(Context &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(2);
    VcServer *serv = ctx.serv;

    uint64_t size = 0;
    int ret = serv->db->hsize(ctx, req[1], &size);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyInt(size);
    }

    return 0;
}

int proc_hset(Context &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(4);
    VcServer *serv = ctx.serv;

    int added = 0;
    int ret = serv->db->hset(ctx, req[1], req[2], req[3], &added);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else if (ret == 0) {
        resp->addReplyInt(0);
    } else {
        resp->addReplyInt(added);
    }

    return 0;
}

int proc_hsetnx(Context &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(4);
    VcServer *serv = ctx.serv;

    int added = 0;
    int ret = serv->db->hsetnx(ctx, req[1], req[2], req[3], &added);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else if (ret == 0) {
        resp->addReplyInt(0);
    } else {
        resp->addReplyInt(added);
    }

    return 0;
}

int proc_hget(Context &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(3);
    VcServer *serv = ctx.serv;


    std::pair<std::string, bool> val;
    int ret = serv->db->hget(ctx, req[1], req[2], val);

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


int proc_hgetall(Context &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(2);
    VcServer *serv = ctx.serv;


    std::map<std::string, std::string> resMap;
    int ret = serv->db->hgetall(ctx, req[1], resp->resp_arr, true, true);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else if (ret == 0) {
        resp->addReplyListEmpty();
    } else {
        resp->convertReplyToList();
    }

    return 0;
}

int proc_hscan(Context &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(3);
    VcServer *serv = ctx.serv;


    int cursorIndex = 2;

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

    ret = serv->db->hscan(ctx, req[1], cursor, scanParams.pattern, scanParams.limit, resp->resp_arr);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    }

    resp->convertReplyToScanResult();

    return 0;
}


int proc_hkeys(Context &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(2);
    VcServer *serv = ctx.serv;

//	uint64_t limit = recv_bytes[4].Uint64();

    std::map<std::string, std::string> resMap;
    int ret = serv->db->hgetall(ctx, req[1], resp->resp_arr, true, false);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else if (ret == 0) {
        resp->addReplyListEmpty();

    } else {
        resp->convertReplyToList();
    }


    return 0;
}

int proc_hvals(Context &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(2);
    VcServer *serv = ctx.serv;

//	uint64_t limit = recv_bytes[4].Uint64();

    std::map<std::string, std::string> resMap;
    int ret = serv->db->hgetall(ctx, req[1], resp->resp_arr, false, true);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else if (ret == 0) {
        resp->addReplyListEmpty();

    } else {
        resp->convertReplyToList();
    }

    return 0;
}

int proc_hincrbyfloat(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(4);

    long double by = req[3].LDouble();
    if (errno == EINVAL) {
        addReplyErrorCodeReturn(INVALID_DBL);
    }

    long double new_val;
    int ret = serv->db->hincrbyfloat(ctx, req[1], req[2], by, &new_val);
    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyHumanLongDouble(new_val);
    }
    return 0;

}

int proc_hincrby(Context &ctx, const Request &req, Response *resp) {
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(4);

    int64_t by = req[3].Int64();

    if (errno == EINVAL) {
        addReplyErrorCodeReturn(INVALID_INT);
    }

    int64_t new_val = 0;
    int ret = serv->db->hincr(ctx, req[1], req[2], by, &new_val);
    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyInt(new_val);
    }
    return 0;

}