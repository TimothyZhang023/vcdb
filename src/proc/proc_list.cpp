/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
/* queue */
#include "proc_common.h"

int proc_llen(ClientContext &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(2);

    uint64_t len = 0;
    int ret = ctx.db->LLen(ctx, req[1], &len);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyInt(len);
    }

    return 0;
}


int proc_lpushx(ClientContext &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(3);

    const Bytes &name = req[1];
    uint64_t len = 0;
    int ret = ctx.db->LPushX(ctx, name, req, 2, &len);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyInt(len);
    }

    return 0;
}


int proc_lpush(ClientContext &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(3);

    const Bytes &name = req[1];
    uint64_t len = 0;
    int ret = ctx.db->LPush(ctx, name, req, 2, &len);
    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyInt(len);
    }

    return 0;
}

int proc_rpushx(ClientContext &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(3);

    uint64_t len = 0;
    int ret = ctx.db->RPushX(ctx, req[1], req, 2, &len);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyInt(len);
    }

    return 0;
}


int proc_rpush(ClientContext &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(3);

    uint64_t len = 0;
    int ret = ctx.db->RPush(ctx, req[1], req, 2, &len);
    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyInt(len);
    }

    return 0;
}


int proc_lpop(ClientContext &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(2);

    const Bytes &name = req[1];

    std::pair<std::string, bool> val;
    int ret = ctx.db->LPop(ctx, name, val);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        if (val.second) {
            resp->addReplyString(val.first);
        } else {
            resp->addReplyNil();
        };

    }

    return 0;
}

int proc_rpop(ClientContext &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(2);

    std::pair<std::string, bool> val;
    int ret = ctx.db->RPop(ctx, req[1], val);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else {
        if (val.second) {
            resp->addReplyString(val.first);
        } else {
            resp->addReplyNil();
        };

    }

    return 0;
}


int proc_ltrim(ClientContext &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(4);

    int64_t begin = req[2].Int64();
    if (errno == EINVAL) {
        addReplyErrorCodeReturn(INVALID_INT);
    }

    int64_t end = req[3].Int64();
    if (errno == EINVAL) {
        addReplyErrorCodeReturn(INVALID_INT);
    }


    int ret = ctx.db->ltrim(ctx, req[1], begin, end);

    if (ret < 0) {
        resp->resp_arr.clear();
        addReplyErrorCodeReturn(ret);
    } else {
        resp->addReplyStatusOK();
    }

    return 0;
}


int proc_lrange(ClientContext &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(4);

    int64_t begin = req[2].Int64();
    if (errno == EINVAL) {
        addReplyErrorCodeReturn(INVALID_INT);
    }

    int64_t end = req[3].Int64();
    if (errno == EINVAL) {
        addReplyErrorCodeReturn(INVALID_INT);
    }

    int ret = ctx.db->lrange(ctx, req[1], begin, end, resp->resp_arr);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    }

    resp->convertReplyToList();

    return 0;
}

int proc_lindex(ClientContext &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(3);

    int64_t index = req[2].Int64();
    if (errno == EINVAL) {
        addReplyErrorCodeReturn(INVALID_INT);
    }

    std::pair<std::string, bool> val;
    int ret = ctx.db->LIndex(ctx, req[1], index, val);

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

int proc_lset(ClientContext &ctx, const Request &req, Response *resp) {
    CHECK_MIN_PARAMS(4);

    const Bytes &name = req[1];
    int64_t index = req[2].Int64();
    if (errno == EINVAL) {
        addReplyErrorCodeReturn(INVALID_INT);
    }

    const Bytes &item = req[3];
    int ret = ctx.db->LSet(ctx, name, index, item);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
    } else if (ret == 0) {
        addReplyErrorInfoReturn("ERR no such key");
    } else {
        resp->addReplyStatusOK();
    }
    return 0;
}
