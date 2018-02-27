//
// Created by zts on 1/4/18.
//
#ifndef VCDB_VCSERVER_H
#define VCDB_VCSERVER_H

#include "atomic"

#include "ProcMap.h"
#include "Commands.h"

class SSDB;

class SSDBImpl;

namespace vcdb {
    class Binlog;
}

const int SERVER_RUNNING = (1 << 0);
const int SERVER_LOADING = (1 << 1);
const int SERVER_CLOSE = (1 << 2);

class ServerContext {
public:
    ServerContext(SSDB *ssdb, vcdb::Binlog* binlog) {
        this->db = (SSDBImpl *) ssdb;
        this->binlog = binlog;
        this->regProcs();
        this->status = SERVER_RUNNING;
    }

    ~ServerContext() = default;

    SSDBImpl *db = nullptr;

    vcdb::Binlog *binlog = nullptr;

    vcdb::ProcMap procMap;

    std::atomic<int> status;

    void regProcs() {
        int j;
        int numcommands = sizeof(redisCommandTable) / sizeof(struct redisCommand);

        for (j = 0; j < numcommands; j++) {
            struct redisCommand *c = redisCommandTable + j;

            for (const char f : c->sflags) {
                switch (f) {
                    case 'w':
                        c->flags |= CMD_WRITE;
                        break;
                    case 'r':
                        c->flags |= CMD_READONLY;
                        break;
                    case 'm':
                        c->flags |= CMD_DENYOOM;
                        break;
                    case 'a':
                        c->flags |= CMD_ADMIN;
                        break;
                    case 'p':
                        c->flags |= CMD_PUBSUB;
                        break;
                    case 's':
                        c->flags |= CMD_NOSCRIPT;
                        break;
                    case 'R':
                        c->flags |= CMD_RANDOM;
                        break;
                    case 'S':
                        c->flags |= CMD_SORT_FOR_SCRIPT;
                        break;
                    case 'l':
                        c->flags |= CMD_LOADING;
                        break;
                    case 't':
                        c->flags |= CMD_STALE;
                        break;
                    case 'M':
                        c->flags |= CMD_SKIP_MONITOR;
                        break;
                    case 'k':
                        c->flags |= CMD_ASKING;
                        break;
                    case 'F':
                        c->flags |= CMD_FAST;
                        break;
                    default:
                        assert("Unsupported command flag");
                        break;
                }
            }

            procMap.regProc(c);


        }

    }

};


#endif //VCDB_VCSERVER_H
