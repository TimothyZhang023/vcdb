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
//
// Redis protocol related funcitons
//

// Calculate the number of bytes needed to represent an integer as string.
    static int intlen(int i) {
        int len = 0;
        if (i < 0) {
            len++;
            i = -i;
        }
        do {
            len++;
            i /= 10;
        } while (i);
        return len;
    }

// Helper that calculates the bulk length given a certain string length.
    static size_t bulklen(size_t len) {
        return 1 + intlen(len) + 2 + len + 2;
    }

    inline int redisvFormatCommand(std::string *cmd, const char *format, va_list ap) {
        const char *c = format;
        std::string curarg;
        char buf[1048576];
        std::vector<std::string> args;
        int touched = 0; /* was the current argument touched? */
        size_t totlen = 0;

        while (*c != '\0') {
            if (*c != '%' || c[1] == '\0') {
                if (*c == ' ') {
                    if (touched) {
                        args.push_back(curarg);
                        totlen += bulklen(curarg.size());
                        curarg.clear();
                        touched = 0;
                    }
                } else {
                    curarg.append(c, 1);
                    touched = 1;
                }
            } else {
                char *arg = nullptr;
                size_t size = 0;

                switch (c[1]) {
                    case 's':
                        arg = va_arg(ap, char*);
                        size = strlen(arg);
                        if (size > 0) {
                            curarg.append(arg, size);
                        }
                        break;
                    case 'b':
                        arg = va_arg(ap, char*);
                        size = va_arg(ap, size_t);
                        if (size > 0) {
                            curarg.append(arg, size);
                        }
                        break;
                    case '%':
                        curarg.append(arg, size);
                        break;
                    default:
                        /* Try to detect printf format */
                    {
                        static const char intfmts[] = "diouxX";
                        char _format[16];
                        const char *_p = c + 1;
                        size_t _l = 0;
                        va_list _cpy;
                        bool fmt_valid = false;

                        /* Flags */
                        if (*_p != '\0' && *_p == '#') _p++;
                        if (*_p != '\0' && *_p == '0') _p++;
                        if (*_p != '\0' && *_p == '-') _p++;
                        if (*_p != '\0' && *_p == ' ') _p++;
                        if (*_p != '\0' && *_p == '+') _p++;

                        /* Field width */
                        while (*_p != '\0' && isdigit(*_p)) _p++;

                        /* Precision */
                        if (*_p == '.') {
                            _p++;
                            while (*_p != '\0' && isdigit(*_p)) _p++;
                        }

                        /* Copy va_list before consuming with va_arg */
                        va_copy(_cpy, ap);

                        if (strchr(intfmts, *_p) != NULL) {       /* Integer conversion (without modifiers) */
                            va_arg(ap, int);
                            fmt_valid = true;
                        } else if (strchr("eEfFgGaA", *_p) != NULL) { /* Double conversion (without modifiers) */
                            va_arg(ap, double);
                            fmt_valid = true;
                        } else if (_p[0] == 'h' && _p[1] == 'h') {    /* Size: char */
                            _p += 2;
                            if (*_p != '\0' && strchr(intfmts, *_p) != NULL) {
                                va_arg(ap, int); /* char gets promoted to int */
                                fmt_valid = true;
                            }
                        } else if (_p[0] == 'h') {                /* Size: short */
                            _p += 1;
                            if (*_p != '\0' && strchr(intfmts, *_p) != NULL) {
                                va_arg(ap, int); /* short gets promoted to int */
                                fmt_valid = true;
                            }
                        } else if (_p[0] == 'l' && _p[1] == 'l') { /* Size: long long */
                            _p += 2;
                            if (*_p != '\0' && strchr(intfmts, *_p) != NULL) {
                                va_arg(ap, long long);
                                fmt_valid = true;
                            }
                        } else if (_p[0] == 'l') {           /* Size: long */
                            _p += 1;
                            if (*_p != '\0' && strchr(intfmts, *_p) != NULL) {
                                va_arg(ap, long);
                                fmt_valid = true;
                            }
                        }

                        if (!fmt_valid) {
                            va_end(_cpy);
                            return REDIS_ERR;
                        }

                        _l = (_p + 1) - c;
                        if (_l < sizeof(_format) - 2) {
                            memcpy(_format, c, _l);
                            _format[_l] = '\0';

                            int n = vsnprintf(buf, REDIS_MAX_MESSAGE, _format, _cpy);
                            curarg.append(buf, n);

                            /* Update current position (note: outer blocks
                             * increment c twice so compensate here) */
                            c = _p - 1;
                        }

                        va_end(_cpy);
                        break;
                    }
                }

                if (curarg.empty()) {
                    return REDIS_ERR;
                }

                touched = 1;
                c++;
            }
            c++;
        }

        /* Add the last argument if needed */
        if (touched) {
            args.push_back(curarg);
            totlen += bulklen(curarg.size());
        }

        /* Add bytes needed to hold multi bulk count */
        totlen += 1 + intlen(args.size()) + 2;

        /* Build the command at protocol level */
        cmd->clear();
        cmd->reserve(totlen);

        cmd->append(1, '*');
        cmd->append(std::to_string(args.size()));
        cmd->append("\r\n");
        for (size_t i = 0; i < args.size(); i++) {
            cmd->append(1, '$');
            cmd->append(std::to_string(args[i].size()));
            cmd->append("\r\n");
            cmd->append(args[i]);
            cmd->append("\r\n");
        }
        assert(cmd->size() == totlen);

        return totlen;
    }

    inline int redisvAppendCommand(std::string *cmd, const char *format, va_list ap) {
        int len = redisvFormatCommand(cmd, format, ap);
        if (len == -1) {
            return REDIS_ERR;
        }

        return REDIS_OK;
    }


    inline int redisFormatCommandArgv(pink::RedisCmdArgsType argv, std::string *cmd) {
        size_t argc = argv.size();

        int totlen = 1 + intlen(argc) + 2;
        for (size_t i = 0; i < argc; i++) {
            totlen += bulklen(argv[i].size());
        }

        cmd->clear();
        cmd->reserve(totlen);

        cmd->append(1, '*');
        cmd->append(std::to_string(argc));
        cmd->append("\r\n");
        for (size_t i = 0; i < argc; i++) {
            cmd->append(1, '$');
            cmd->append(std::to_string(argv[i].size()));
            cmd->append("\r\n");
            cmd->append(argv[i]);
            cmd->append("\r\n");
        }

        return REDIS_OK;
    }

    inline int SerializeRedisCommand(std::string *cmd, const char *format, ...) {
        va_list ap;
        va_start(ap, format);
        int result = redisvAppendCommand(cmd, format, ap);
        va_end(ap);
        return result;
    }

    inline int SerializeRedisCommand(pink::RedisCmdArgsType argv, std::string *cmd) {
        return redisFormatCommandArgv(argv, cmd);
    }

};  // namespace pink

#endif //VCDB_REDISCLIENT_H
