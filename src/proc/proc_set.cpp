/*
Copyright (c) 2017, Timothy. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "proc_common.h"

int proc_sadd(ClientContext &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(3);

    const Bytes &name = req[1];
    std::string val;
    std::set<Bytes> mem_set;

    for_each(req.begin() + 2, req.end(), [&](Bytes b) {
        mem_set.insert(b);
    });

    int64_t num = 0;

    int ret = ctx.db->sadd(ctx, name, mem_set, &num);
    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyInt(num);
    }

    return 0;
}

int proc_srem(ClientContext &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(3);

    const Bytes &name = req[1];


    int64_t num = 0;

    int ret = ctx.db->srem(ctx, name, req, &num);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyInt(num);
    }

    return 0;
}

int proc_scard(ClientContext &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(2);

    uint64_t len = 0;

    int ret = ctx.db->scard(ctx, req[1], &len);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyInt(len);
    }

    return 0;
}


int proc_sismember(ClientContext &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(3);

    bool ismember = false;
    int ret = ctx.db->sismember(ctx, req[1], req[2], &ismember);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else if (ret == 0) {
        resp->addReplyInt(0);
    } else {
        resp->addReplyInt(ismember ? 1 : 0);
    }

    return 0;
}

int proc_smembers(ClientContext &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(2);

    int ret = ctx.db->smembers(ctx, req[1], resp->resp_arr);

    if (ret < 0) {
        resp->resp_arr.clear();
        addReplyErrorCodeReturn(ret);
    }

    resp->convertReplyToList();


    return 0;
}


int proc_spop(ClientContext &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(2);

    int64_t pop_count = 1;
    if (req.size() > 2) {
        pop_count = req[2].Int64();
        if (errno == EINVAL) {
            addReplyErrorCodeReturn(INVALID_INT);
        }
    }


    int ret = ctx.db->spop(ctx, req[1], resp->resp_arr, pop_count);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else if (ret == 0) {

    }

    resp->convertReplyToList();


    return 0;
}

int proc_srandmember(ClientContext &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(2);

    int64_t count = 1;
    if (req.size() > 2) {
        count = req[2].Int64();
        if (errno == EINVAL) {
            addReplyErrorCodeReturn(INVALID_INT);
        }
    }

    int ret = ctx.db->srandmember(ctx, req[1], resp->resp_arr, count);

    if (ret < 0) {
        resp->resp_arr.clear();
        addReplyErrorCodeReturn(ret);
    }
    resp->convertReplyToList();

    return 0;
}


int proc_sscan(ClientContext &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(3);


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

    ret = ctx.db->sscan(ctx, req[1], cursor, scanParams.pattern, scanParams.limit, resp->resp_arr);

    if (ret < 0) {
        resp->resp_arr.clear();
        addReplyErrorCodeReturn(ret);
    }

    resp->convertReplyToScanResult();

    return 0;
}
