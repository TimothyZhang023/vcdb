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

void Response::addReplyError(const std::string &err_msg) {
    output->append("-");
    output->append(err_msg);
    output->append("\r\n");
}

void Response::addReplyError(const char *err_msg) {
    output->append("-");
    output->append(err_msg);
    output->append("\r\n");
}

void Response::addReplyNil() {
    output->append("$-1\r\n");
}


void Response::addReplyStatus(const std::string &msg) {
    output->append("+");
    output->append(msg);
    output->append("\r\n");
}

void Response::addReplyStatusOK() {
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
    char buf[256] = {0};
    int len = ld2string(buf, sizeof(buf), d, 1);
    addReplyBulkCBuffer(buf, static_cast<size_t>(len));
}

void Response::addReplyDouble(double d) {
    char buf[128] = {0};
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

void Response::addReplyListEmpty() {
    output->append("*0\r\n");
}

void Response::addReplyListHead(int size) {
    {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "*%d\r\n", size);
        output->append(buf);
    }
}


void Response::convertReplyToList() {

    {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "*%d\r\n", (int) resp_arr.size());
        output->append(buf);
    }

    for (const auto &val : resp_arr) {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "$%d\r\n", (int) val.size());
        output->append(buf);
        output->append(val.data(), val.size());
        output->append("\r\n");
    }
}


void Response::convertReplyToScanResult() {
    addReplyListHead(2);

    {
        addReplyString(resp_arr[0]);
    }
    {
        {
            char buf[32] = {0};
            snprintf(buf, sizeof(buf), "*%d\r\n", (int) resp_arr.size() - 1);
            output->append(buf);
        }
        for (int i = 1; i < resp_arr.size(); i++) {
            auto val = resp_arr[i];
            char buf[32] = {0};
            snprintf(buf, sizeof(buf), "$%d\r\n", (int) val.size());
            output->append(buf);
            output->append(val.data(), val.size());
            output->append("\r\n");
        }
    }
}

Response::Response(std::string *output) : output(output) {}
