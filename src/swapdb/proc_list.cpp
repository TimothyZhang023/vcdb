/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
/* queue */
#include "serv.h"

int proc_qsize(Context &ctx, const Request &req, Response *resp){
	SSDBServer *serv = (SSDBServer *) ctx.serv;
	CHECK_MIN_PARAMS(2);

	uint64_t len = 0;
	int ret = serv->ssdb->LLen(ctx, req[1], &len);

    if (ret < 0) {
		reply_err_return(ret);
	} else {
		resp->reply_int(ret, len);
	}

	return 0;
}


int proc_qpush_frontx(Context &ctx, const Request &req, Response *resp){
	CHECK_MIN_PARAMS(3);
	SSDBServer *serv = (SSDBServer *) ctx.serv;

	const Bytes &name = req[1];
	uint64_t len = 0;
	int ret = serv->ssdb->LPushX(ctx, name, req, 2, &len);

    if (ret < 0) {
		reply_err_return(ret);
	} else {
		resp->reply_int(ret, len);
	}

	return 0;
}


int proc_qpush_front(Context &ctx, const Request &req, Response *resp){
	CHECK_MIN_PARAMS(3);
	SSDBServer *serv = (SSDBServer *) ctx.serv;

	const Bytes &name = req[1];
 	uint64_t len = 0;
	int ret = serv->ssdb->LPush(ctx, name, req, 2, &len);
    if (ret < 0) {
		reply_err_return(ret);
	} else {
		resp->reply_int(ret, len);
	}

	return 0;
}

int proc_qpush_backx(Context &ctx, const Request &req, Response *resp){
	CHECK_MIN_PARAMS(3);
	SSDBServer *serv = (SSDBServer *) ctx.serv;

	uint64_t len = 0;
	int ret = serv->ssdb->RPushX(ctx, req[1], req, 2, &len);

    if (ret < 0) {
		reply_err_return(ret);
	} else {
		resp->reply_int(ret, len);
	}

	return 0;
}


int proc_qpush_back(Context &ctx, const Request &req, Response *resp){
    CHECK_MIN_PARAMS(3);
    SSDBServer *serv = (SSDBServer *) ctx.serv;

    uint64_t len = 0;
    int ret = serv->ssdb->RPush(ctx, req[1], req, 2, &len);
    if (ret < 0) {
		reply_err_return(ret);
	} else {
		resp->reply_int(ret, len);
	}

	return 0;
}


int proc_qpop_front(Context &ctx, const Request &req, Response *resp){
	CHECK_MIN_PARAMS(2);
	SSDBServer *serv = (SSDBServer *) ctx.serv;

	const Bytes &name = req[1];

	std::pair<std::string, bool> val;
	int ret = serv->ssdb->LPop(ctx, name, val);

    if (ret < 0) {
		reply_err_return(ret);
	} else {
		if (val.second) {
			resp->reply_get(1, &val.first);
		} else {
			resp->reply_get(0, &val.first);
		};

	}

	return 0;
}

int proc_qpop_back(Context &ctx, const Request &req, Response *resp){
    CHECK_MIN_PARAMS(2);
    SSDBServer *serv = (SSDBServer *) ctx.serv;

	std::pair<std::string, bool> val;
    int ret = serv->ssdb->RPop(ctx, req[1], val);

    if (ret < 0) {
		reply_err_return(ret);
	} else {
		if (val.second) {
			resp->reply_get(1, &val.first);
		} else {
			resp->reply_get(0, &val.first);
		};

	}

	return 0;
}


int proc_qtrim(Context &ctx, const Request &req, Response *resp){
	SSDBServer *serv = (SSDBServer *) ctx.serv;
	CHECK_MIN_PARAMS(4);

	int64_t begin = req[2].Int64();
	if (errno == EINVAL){
		reply_err_return(INVALID_INT);
	}

	int64_t end = req[3].Int64();
	if (errno == EINVAL){
		reply_err_return(INVALID_INT);
	}


	int ret = serv->ssdb->ltrim(ctx, req[1], begin, end);

	if (ret < 0){
		resp->resp.clear();
		reply_err_return(ret);
	} else {
		resp->reply_status(ret);
	}

	return 0;
}



int proc_qslice(Context &ctx, const Request &req, Response *resp){
	SSDBServer *serv = (SSDBServer *) ctx.serv;
	CHECK_MIN_PARAMS(4);

	int64_t begin = req[2].Int64();
	if (errno == EINVAL){
		reply_err_return(INVALID_INT);
	}

	int64_t end = req[3].Int64();
	if (errno == EINVAL){
		reply_err_return(INVALID_INT);
	}

	resp->reply_list_ready();
	int ret = serv->ssdb->lrange(ctx, req[1], begin, end, resp->resp);

	if (ret < 0){
		resp->resp.clear();
		reply_err_return(ret);
	}

	return 0;
}

int proc_qget(Context &ctx, const Request &req, Response *resp){
	SSDBServer *serv = (SSDBServer *) ctx.serv;
	CHECK_MIN_PARAMS(3);

	int64_t index = req[2].Int64();
	if (errno == EINVAL){
		reply_err_return(INVALID_INT);
	}

	std::pair<std::string, bool> val;
	int ret = serv->ssdb->LIndex(ctx, req[1], index, val);

    if (ret < 0) {
		reply_err_return(ret);
	} else {
		if (val.second) {
			resp->reply_get(1, &val.first);
		} else {
			resp->reply_get(0, &val.first);
		}
	}

	return 0;
}

int proc_qset(Context &ctx, const Request &req, Response *resp){
	SSDBServer *serv = (SSDBServer *) ctx.serv;
	CHECK_MIN_PARAMS(4);

	const Bytes &name = req[1];
	int64_t index = req[2].Int64();
	if (errno == EINVAL){
		reply_err_return(INVALID_INT);
	}

	const Bytes &item = req[3];
	int ret = serv->ssdb->LSet(ctx, name, index, item);

	if(ret < 0){
		reply_err_return(ret);
	} else if ( ret == 0) {
		//???
		reply_errinfo_return("ERR no such key");
	} else{
		//TODO CHECK HERE
		resp->reply_ok();
	}
	return 0;
}
