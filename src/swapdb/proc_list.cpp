/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
/* queue */
#include "serv.h"

int proc_qsize(Context &ctx, const Request &req, Response *resp){
	VcServer *serv = ctx.serv;
	CHECK_MIN_PARAMS(2);

	uint64_t len = 0;
	int ret = serv->db->LLen(ctx, req[1], &len);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
	} else {
		resp->addReplyInt(len);
	}

	return 0;
}


int proc_qpush_frontx(Context &ctx, const Request &req, Response *resp){
	CHECK_MIN_PARAMS(3);
	VcServer *serv = ctx.serv;

	const Bytes &name = req[1];
	uint64_t len = 0;
	int ret = serv->db->LPushX(ctx, name, req, 2, &len);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
	} else {
		resp->addReplyInt(len);
	}

	return 0;
}


int proc_qpush_front(Context &ctx, const Request &req, Response *resp){
	CHECK_MIN_PARAMS(3);
	VcServer *serv = ctx.serv;

	const Bytes &name = req[1];
 	uint64_t len = 0;
	int ret = serv->db->LPush(ctx, name, req, 2, &len);
    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
	} else {
		resp->addReplyInt(len);
	}

	return 0;
}

int proc_qpush_backx(Context &ctx, const Request &req, Response *resp){
	CHECK_MIN_PARAMS(3);
	VcServer *serv = ctx.serv;

	uint64_t len = 0;
	int ret = serv->db->RPushX(ctx, req[1], req, 2, &len);

    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
	} else {
		resp->addReplyInt(len);
	}

	return 0;
}


int proc_qpush_back(Context &ctx, const Request &req, Response *resp){
    CHECK_MIN_PARAMS(3);
    VcServer *serv = ctx.serv;

    uint64_t len = 0;
    int ret = serv->db->RPush(ctx, req[1], req, 2, &len);
    if (ret < 0) {
        addReplyErrorCodeReturn(ret);
	} else {
		resp->addReplyInt(len);
	}

	return 0;
}


int proc_qpop_front(Context &ctx, const Request &req, Response *resp){
	CHECK_MIN_PARAMS(2);
	VcServer *serv = ctx.serv;

	const Bytes &name = req[1];

	std::pair<std::string, bool> val;
	int ret = serv->db->LPop(ctx, name, val);

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

int proc_qpop_back(Context &ctx, const Request &req, Response *resp){
    CHECK_MIN_PARAMS(2);
    VcServer *serv = ctx.serv;

	std::pair<std::string, bool> val;
    int ret = serv->db->RPop(ctx, req[1], val);

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


int proc_qtrim(Context &ctx, const Request &req, Response *resp){
	VcServer *serv = ctx.serv;
	CHECK_MIN_PARAMS(4);

	int64_t begin = req[2].Int64();
	if (errno == EINVAL){
        addReplyErrorCodeReturn(INVALID_INT);
	}

	int64_t end = req[3].Int64();
	if (errno == EINVAL){
        addReplyErrorCodeReturn(INVALID_INT);
	}


	int ret = serv->db->ltrim(ctx, req[1], begin, end);

	if (ret < 0){
		resp->resp_arr.clear();
        addReplyErrorCodeReturn(ret);
	} else {
		resp->addReplyStatusOK();
	}

	return 0;
}



int proc_qslice(Context &ctx, const Request &req, Response *resp){
	VcServer *serv = ctx.serv;
	CHECK_MIN_PARAMS(4);

	int64_t begin = req[2].Int64();
	if (errno == EINVAL){
        addReplyErrorCodeReturn(INVALID_INT);
	}

	int64_t end = req[3].Int64();
	if (errno == EINVAL){
        addReplyErrorCodeReturn(INVALID_INT);
	}

	resp->reply_list_ready();
	int ret = serv->db->lrange(ctx, req[1], begin, end, resp->resp_arr);

	if (ret < 0){
		resp->resp_arr.clear();
        addReplyErrorCodeReturn(ret);
	}

	return 0;
}

int proc_qget(Context &ctx, const Request &req, Response *resp){
	VcServer *serv = ctx.serv;
	CHECK_MIN_PARAMS(3);

	int64_t index = req[2].Int64();
	if (errno == EINVAL){
        addReplyErrorCodeReturn(INVALID_INT);
	}

	std::pair<std::string, bool> val;
	int ret = serv->db->LIndex(ctx, req[1], index, val);

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

int proc_qset(Context &ctx, const Request &req, Response *resp){
	VcServer *serv = ctx.serv;
	CHECK_MIN_PARAMS(4);

	const Bytes &name = req[1];
	int64_t index = req[2].Int64();
	if (errno == EINVAL){
        addReplyErrorCodeReturn(INVALID_INT);
	}

	const Bytes &item = req[3];
	int ret = serv->db->LSet(ctx, name, index, item);

	if(ret < 0){
        addReplyErrorCodeReturn(ret);
	} else if ( ret == 0) {
        addReplyErrorInfoReturn("ERR no such key");
	} else{
		resp->addReplyStatusOK();
	}
	return 0;
}
