//
// Created by zts on 1/8/18.
//

#ifndef VCDB_REDISCLIENT_H
#define VCDB_REDISCLIENT_H
// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
#include "pink/include/redis_cli.h"

#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>

#include <string>
#include <vector>

#include "pink/include/pink_define.h"
#include "pink/include/pink_cli.h"


namespace vcdb {

    class RedisClient : public pink::PinkCli {
    public:
        RedisClient();

        virtual ~RedisClient();

        // msg should have been parsed
        virtual Status Send(void *msg);

        // Read, parse and store the reply
        virtual Status Recv(void *result = NULL);


        ssize_t BufferRead();

        int32_t RemainBufferd() {
            return rbuf_size_ - rbuf_offset_;
        }

        char *ReadBytes(unsigned int bytes);

        char *ReadLine(int *_len);

    private:

        pink::RedisCmdArgsType argv_;   // The parsed result

        char *rbuf_;
        int32_t rbuf_size_;
        int32_t rbuf_pos_;
        int32_t rbuf_offset_;
        int elements_;    // the elements number of this current reply
        int err_;

        int GetReply();

        int GetReplyFromReader();

        int ProcessLineItem();

        int ProcessBulkItem();

        int ProcessMultiBulkItem();



        // No copyable
        RedisClient(const RedisClient &);

        void operator=(const RedisClient &);
    };

    enum REDIS_STATUS {
        REDIS_ETIMEOUT = -5,
        REDIS_EREAD_NULL = -4,
        REDIS_EREAD = -3,     // errno is set
        REDIS_EPARSE_TYPE = -2,
        REDIS_ERR = -1,
        REDIS_OK = 0,
        REDIS_HALF,
        REDIS_REPLY_STRING,
        REDIS_REPLY_ARRAY,
        REDIS_REPLY_INTEGER,
        REDIS_REPLY_NIL,
        REDIS_REPLY_STATUS,
        REDIS_REPLY_ERROR
    };


/* Find pointer to \r\n. */
    static char *seekNewline(char *s, size_t len) {
        int pos = 0;
        int _len = len - 1;

        /* Position should be < len-1 because the character at "pos" should be
         * followed by a \n. Note that strchr cannot be used because it doesn't
         * allow to search a limited length and the buffer that is being searched
         * might not have a trailing NULL character. */
        while (pos < _len) {
            while (pos < _len && s[pos] != '\r') pos++;
            if (s[pos] != '\r' || pos >= _len) {
                /* Not found. */
                return NULL;
            } else {
                if (s[pos + 1] == '\n') {
                    /* Found. */
                    return s + pos;
                } else {
                    /* Continue searching. */
                    pos++;
                }
            }
        }
        return NULL;
    }

/* Read a long long value starting at *s, under the assumption that it will be
 * terminated by \r\n. Ambiguously returns -1 for unexpected input. */
    static long long readLongLong(char *s) {
        long long v = 0;
        int dec, mult = 1;
        char c;

        if (*s == '-') {
            mult = -1;
            s++;
        } else if (*s == '+') {
            mult = 1;
            s++;
        }

        while ((c = *(s++)) != '\r') {
            dec = c - '0';
            if (dec >= 0 && dec < 10) {
                v *= 10;
                v += dec;
            } else {
                /* Should not happen... */
                return -1;
            }
        }

        return mult * v;
    }

    vcdb::RedisClient *NewRedisClient();


};  // namespace pink

#endif //VCDB_REDISCLIENT_H
