/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "resp.h"
#include <stdio.h>
#include <sstream>
#include <iomanip>
#include <util/strings.h>


void Response::push_back(const std::string &s) {
    resp_arr.push_back(s);
}

void Response::emplace_back(std::string &&s) {
    resp_arr.emplace_back(s);
}


void Response::add(const std::string &s) {
    resp_arr.push_back(s);
}


void Response::reply_ok() {
    resp_arr.emplace_back("ok");
}


void Response::reply_status(int status) {
    if (status == -1) {
        resp_arr.emplace_back("error");
    } else {
        resp_arr.emplace_back("ok");
    }
}

void Response::reply_bool(int status) {
    if (status == -1) {
        resp_arr.emplace_back("error");
    } else if (status == 0) {
        resp_arr.emplace_back("ok");
        resp_arr.emplace_back("0");
    } else {
        resp_arr.emplace_back("ok");
        resp_arr.emplace_back("1");
    }
}


void Response::reply_get(int status, const std::string *val) {
    if (status < 0) {
        resp_arr.emplace_back("error");
    } else if (status == 0) {
        resp_arr.emplace_back("not_found");
    } else {
        resp_arr.emplace_back("ok");
        if (val) {
            resp_arr.emplace_back(*val);
        }
        return;
    }
}

void Response::reply_scan_ready() {
    resp_arr.clear();
    resp_arr.emplace_back("ok");
    resp_arr.emplace_back("0");
}

void Response::reply_list_ready() {
    resp_arr.emplace_back("ok");
}


void Response::addReplyError(const std::string &err_msg) {
    output->append("-");
    output->append(err_msg);
    output->append("\r\n");
}

void Response::addReplyNil() {
    output->append("$-1\r\n");
}


void Response::addStatus(const std::string &msg) {
    output->append("+");
    output->append(msg);
    output->append("\r\n");
}

void Response::addStatusOK() {
    output->append("+OK\r\n");
}


void Response::addReplyString(const std::string &msg) {
    char buf[32];
    snprintf(buf, sizeof(buf), "$%d\r\n", (int) msg.size());
    output->append(buf);
    output->append(msg.data(), msg.size());
    output->append("\r\n");
}

void Response::addReplyBulkCBuffer(const void *p, size_t len) {
    char buf[32];
    snprintf(buf, sizeof(buf), "$%d\r\n", (int) len);
    output->append(buf);
    output->append(static_cast<const char *>(p), len);
    output->append("\r\n");
}

void Response::addReplyBulkCString(const char *s) {
    if (s == nullptr) {
        addReplyNil();
    } else {
        addReplyBulkCBuffer(s, strlen(s));
    }
}

void Response::addReplyHumanLongDouble(long double d) {
    char buf[256];
    int len = ld2string(buf, sizeof(buf), d, 1);
    addReplyBulkCBuffer(buf, static_cast<size_t>(len));
}

void Response::addReplyDouble(double d) {
    char buf[128];
    int len = ld2string(buf, sizeof(buf), d, 0);
    addReplyBulkCBuffer(buf, static_cast<size_t>(len));
}


void Response::addReplyInt(uint64_t i) {
    char buf[24] = {0};
    snprintf(buf, sizeof(buf), ":%" PRIu64 "\r\n", i);
    output->append(buf);
}

void Response::addReplyInt(int64_t i) {
    char buf[24] = {0};
    snprintf(buf, sizeof(buf), ":%" PRId64 "\r\n", i);
    output->append(buf);

}

void Response::addReplyInt(int i) {
    addReplyInt(static_cast<int64_t>(i));
}


Response::Response(std::string *output) : output(output) {}
