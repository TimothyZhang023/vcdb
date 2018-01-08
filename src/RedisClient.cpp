//
// Created by zts on 1/8/18.
//

#include "RedisClient.h"

namespace vcdb {

    RedisClient *NewRedisClient() {
        return new RedisClient();
    }


    RedisClient::RedisClient()
            : rbuf_size_(REDIS_MAX_MESSAGE),
              rbuf_pos_(0),
              rbuf_offset_(0),
              err_(REDIS_OK) {
        rbuf_ = (char *) malloc(sizeof(char) * rbuf_size_);
    }


// We use passed-in send buffer here
    Status RedisClient::Send(void *msg) {
        Status s;

        // TODO anan use socket_->SendRaw instead
        std::string *storage = reinterpret_cast<std::string *>(msg);
        const char *wbuf = storage->data();
        size_t nleft = storage->size();

        int wbuf_pos = 0;

        ssize_t nwritten;

        while (nleft > 0) {
            if ((nwritten = write(fd(), wbuf + wbuf_pos, nleft)) <= 0) {
                if (errno == EINTR) {
                    nwritten = 0;
                    continue;
                    // blocking fd after setting setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO,...)
                    // will return EAGAIN | EWOULDBLOCK for timeout
                } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    s = Status::Timeout("Send timeout");
                } else {
                    s = Status::IOError("write error " + std::string(strerror(errno)));
                }
                return s;
            }

            nleft -= nwritten;
            wbuf_pos += nwritten;
        }

        return s;
    }

// The result is useless
    Status RedisClient::Recv(void *trival) {

        argv_.clear();
        int result = GetReply();
        switch (result) {
            case REDIS_OK:
                if (trival != nullptr) {
                    *static_cast<pink::RedisCmdArgsType *>(trival) = argv_;
                }
                return Status::OK();
            case REDIS_ETIMEOUT:
                return Status::Timeout("");
            case REDIS_EREAD_NULL:
                return Status::IOError("Read null");
            case REDIS_EREAD:
                return Status::IOError("read failed caz " + std::string(strerror(errno)));
            case REDIS_EPARSE_TYPE:
                return Status::IOError("invalid type");
            default: // other error
                return Status::IOError("other error, maybe " + std::string(strerror(errno)));
        }
    }

    ssize_t RedisClient::BufferRead() {
        // memmove the remain chars to rbuf begin
        if (rbuf_pos_ > 0) {
            if (rbuf_offset_ > 0) {
                memmove(rbuf_, rbuf_ + rbuf_pos_, rbuf_offset_);
            }
            rbuf_pos_ = 0;
        }

        ssize_t nread;

        while (true) {
            nread = read(fd(), rbuf_ + rbuf_offset_, rbuf_size_ - rbuf_offset_);

            if (nread == -1) {
                if (errno == EINTR) {
                    continue;
                    // blocking fd after setting setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,...)
                    // will return EAGAIN for timeout
                } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    return REDIS_ETIMEOUT;
                } else {
                    return REDIS_EREAD;
                }
            } else if (nread == 0) {    // we consider read null an error
                return REDIS_EREAD_NULL;
            }

            rbuf_offset_ += nread;
            return nread;
        }
    }

    RedisClient::~RedisClient() {
        free(rbuf_);
    }



    int RedisClient::ProcessLineItem() {
        char *p;
        int len;

        if ((p = ReadLine(&len)) == NULL) {
            return REDIS_HALF;
        }

        std::string arg(p, len);
        argv_.push_back(arg);
        elements_--;

        return REDIS_OK;
    }

    int RedisClient::ProcessBulkItem() {
        char *p, *s;
        int len;
        int bytelen;

        p = rbuf_ + rbuf_pos_;
        s = seekNewline(p, rbuf_offset_);
        if (s != NULL) {
            bytelen = s - p + 2; /* include \r\n */
            len = readLongLong(p);

            if (len < 0 || len + 2 <= rbuf_offset_) {
                argv_.push_back(std::string(p + bytelen, len));
                elements_--;

                bytelen += len + 2; /* include \r\n */
                rbuf_pos_ += bytelen;
                rbuf_offset_ -= bytelen;
                return REDIS_OK;
            }
        }

        return REDIS_HALF;
    }

    int RedisClient::ProcessMultiBulkItem() {
        char *p;
        int len;

        if ((p = ReadLine(&len)) != NULL) {
            elements_ = readLongLong(p);
            return REDIS_OK;
        }

        return REDIS_HALF;
    }

    int RedisClient::GetReply() {
        int result = REDIS_OK;

        elements_ = 1;
        while (elements_ > 0) {
            // Should read again
            if (rbuf_offset_ == 0 || result == REDIS_HALF) {
                if ((result = BufferRead()) < 0) {
                    return result;
                }
            }

            // stop if error occured.
            if ((result = GetReplyFromReader()) < REDIS_OK) {
                break;
            }
        }

        return result;
    }

    char* RedisClient::ReadBytes(unsigned int bytes) {
        char *p = NULL;
        if ((unsigned int)rbuf_offset_ >= bytes) {
            p = rbuf_ + rbuf_pos_;
            rbuf_pos_ += bytes;
            rbuf_offset_ -= bytes;
        }
        return p;
    }

    char *RedisClient::ReadLine(int *_len) {
        char *p, *s;
        int len;

        p = rbuf_ + rbuf_pos_;
        s = seekNewline(p, rbuf_offset_);
        if (s != NULL) {
            len = s - (rbuf_ + rbuf_pos_);
            rbuf_pos_ += len + 2; /* skip \r\n */
            rbuf_offset_ -= len + 2;
            if (_len) *_len = len;
            return p;
        }
        return NULL;
    }

    int RedisClient::GetReplyFromReader() {
        // if (err_) {
        //   return REDIS_ERR;
        // }

        if (rbuf_offset_ == 0) {
            return REDIS_HALF;
        }

        char *p;
        if ((p = ReadBytes(1)) == NULL) {
            return REDIS_HALF;
        }

        int type;
        // Check reply type
        switch (*p) {
            case '-':
                type = REDIS_REPLY_ERROR;
                break;
            case '+':
                type = REDIS_REPLY_STATUS;
                break;
            case ':':
                type = REDIS_REPLY_INTEGER;
                break;
            case '$':
                type = REDIS_REPLY_STRING;
                break;
            case '*':
                type = REDIS_REPLY_ARRAY;
                break;
            default:
                return REDIS_EPARSE_TYPE;
        }

        switch (type) {
            case REDIS_REPLY_ERROR:
            case REDIS_REPLY_STATUS:
            case REDIS_REPLY_INTEGER:
                //elements_ = 1;
                return ProcessLineItem();
            case REDIS_REPLY_STRING:
                // need processBulkItem();
                //elements_ = 1;
                return ProcessBulkItem();
            case REDIS_REPLY_ARRAY:
                // need processMultiBulkItem();
                return ProcessMultiBulkItem();
            default:
                return REDIS_EPARSE_TYPE; // Avoid warning.
        }
    }

}