//
// Created by zts on 1/4/18.
//

#ifndef VCDB_REQUEST_H
#define VCDB_REQUEST_H

#include "util/bytes.h"

typedef std::vector<Bytes> Request;

template<typename T>
std::string RestoreRequest(const std::vector<T> &req) {
    std::string ori_request;
    ori_request.append("*").append(str((int64_t) req.size())).append("\r\n");

    for_each(req.begin(), req.end(), [&](T s) {
        ori_request.append("$").append(str((int64_t) s.size())).append("\r\n");
        ori_request.append(s.data(), s.size()).append("\r\n");
    });

    return ori_request;
}

template<typename T>
std::string SerializeRequest(T &req) {
    std::string ret;
    char buf[50];
    for (int i = 0; i < req.size(); i++) {
        if (i >= 5 && i < req.size() - 1) {
            sprintf(buf, "[%d more...]", (int) req.size() - i);
            ret.append(buf);
            break;
        }
        if (((req[0] == "get" || req[0] == "set") && i == 1) || req[i].size() < 50) {
            if (req[i].size() == 0) {
                ret.append("\"\"");
            } else {
                std::string h = hexmem(req[i].data(), req[i].size());
                ret.append(h);
            }
        } else {
            sprintf(buf, "[%d]", (int) req[i].size());
            ret.append(buf);
        }
        if (i < req.size() - 1) {
            ret.append(" ");
        }
    }
    return ret;
}

#endif //VCDB_REQUEST_H
