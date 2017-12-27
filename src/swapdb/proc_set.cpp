/*
Copyright (c) 2017, Timothy. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "serv.h"

int proc_sadd(Context &ctx, const Request &req, Response *resp){
    CHECK_MIN_PARAMS(3);
    VcServer *serv = ctx.serv;

    const Bytes &name = req[1];
    std::string val;
    std::set<Bytes> mem_set;

    for_each(req.begin() + 2, req.end(), [&](Bytes b) {
        mem_set.insert(b);
    });

    int64_t num = 0;

    int ret = serv->db->sadd(ctx, name, mem_set, &num);
    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->reply_int(ret, num);
    }

    return 0;
}

int proc_srem(Context &ctx, const Request &req, Response *resp){
    CHECK_MIN_PARAMS(3);
    VcServer *serv = ctx.serv;

    const Bytes &name = req[1];


    int64_t num = 0;

    int ret = serv->db->srem(ctx, name, req, &num);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->reply_int(ret, num);
    }

    return 0;
}

int proc_scard(Context &ctx, const Request &req, Response *resp){
    VcServer *serv = ctx.serv;
    CHECK_MIN_PARAMS(2);

    uint64_t len = 0;

    int ret = serv->db->scard(ctx, req[1], &len);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->reply_int(ret, len);
    }

    return 0;
}


int proc_sismember(Context &ctx, const Request &req, Response *resp){
    CHECK_MIN_PARAMS(3);
    VcServer *serv = ctx.serv;

    bool ismember = false;
    int ret = serv->db->sismember(ctx, req[1], req[2], &ismember);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else if (ret == 0) {
        resp->reply_bool(ret);
    } else {
        resp->reply_bool(ismember?1:0);
    }

    return 0;
}

int proc_smembers(Context &ctx, const Request &req, Response *resp){
    CHECK_MIN_PARAMS(2);
    VcServer *serv = ctx.serv;

    resp->reply_list_ready();

    int ret = serv->db->smembers(ctx, req[1],  resp->resp_arr);

    if (ret < 0){
        resp->resp_arr.clear();
        addReplyErrorCodeReturn(ret);
    }

    return 0;
}


int proc_spop(Context &ctx, const Request &req, Response *resp){
    CHECK_MIN_PARAMS(2);
    VcServer *serv = ctx.serv;

    int64_t pop_count = 1;
    if (req.size() >2) {
        pop_count = req[2].Int64();
        if (errno == EINVAL){
            addReplyErrorCodeReturn(INVALID_INT);
        }
    }

    resp->reply_list_ready();

    int ret = serv->db->spop(ctx, req[1], resp->resp_arr, pop_count);

    if (ret < 0){
        resp->resp_arr.clear();
        addReplyErrorCodeReturn(ret);
    } else if (ret == 0) {

    }

    return 0;
}

int proc_srandmember(Context &ctx, const Request &req, Response *resp){
    CHECK_MIN_PARAMS(2);
    VcServer *serv = ctx.serv;

    int64_t count = 1;
    if (req.size() >2) {
        count = req[2].Int64();
        if (errno == EINVAL){
            addReplyErrorCodeReturn(INVALID_INT);
        }
    }

    resp->reply_list_ready();

    int ret = serv->db->srandmember(ctx, req[1], resp->resp_arr, count);

    if (ret < 0){
        resp->resp_arr.clear();
        addReplyErrorCodeReturn(ret);
    }

    return 0;
}


int proc_sscan(Context &ctx, const Request &req, Response *resp){
    CHECK_MIN_PARAMS(3);
    VcServer *serv = ctx.serv;


    int cursorIndex = 2;

    Bytes cursor = req[cursorIndex];

    cursor.Uint64();
    if (errno == EINVAL){
        addReplyErrorCodeReturn(INVALID_INT);
    }

    ScanParams scanParams;

    int ret = prepareForScanParams(req, cursorIndex + 1, scanParams);
    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    }

    resp->reply_scan_ready();
    ret =  serv->db->sscan(ctx, req[1], cursor, scanParams.pattern, scanParams.limit, resp->resp_arr);

    if (ret < 0) {
        resp->resp_arr.clear();
        addReplyErrorCodeReturn(ret);
    } else if (ret == 0) {
    }

    return 0;
}
