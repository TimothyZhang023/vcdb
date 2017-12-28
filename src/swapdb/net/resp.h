/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#ifndef NET_RESP_H_
#define NET_RESP_H_

#include <unistd.h>
#include <inttypes.h>
#include <string>
#include <vector>
#include <map>
#include "redis/reponse_redis.h"

#define addReplyErrorCodeReturn(n) resp->addReplyError(GetErrorInfo(n)); return 0
#define addReplyErrorInfoReturn(c) resp->addReplyError((c)); return 0


class Response {
public:
    Response(std::string *output);

    std::vector<std::string> resp_arr;
    std::string *output = nullptr;


    void addReplyError(const std::string &err_msg);

    void addReplyError(const char *err_msg);

    void addReplyNil();

    void addReplyStatus(const std::string &msg);

    void addReplyStatusOK();

    void addReplyString(const std::string &msg);

    void addReplyBulkCBuffer(const void *p, size_t len);

    void addReplyBulkCString(const char *s);

    void addReplyHumanLongDouble(long double d);

    void addReplyDouble(double d);

    void addReplyInt(uint64_t i);

    void addReplyInt(int64_t i);

    void addReplyInt(int i);

    void convertReplyToList();

    void convertReplyToScanResult();

    void addReplyListEmpty();

    void addReplyListHead(int size);

};

#endif
