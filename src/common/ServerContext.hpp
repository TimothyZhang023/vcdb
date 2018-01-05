//
// Created by zts on 1/4/18.
//
#ifndef VCDB_VCSERVER_H
#define VCDB_VCSERVER_H

#include "common/ProcMap.h"
#include "atomic"
#include "common/Commands.h"

class SSDB;
class SSDBImpl;

class ServerContext {
public:
    ServerContext(SSDB *ssdb) {
        this->db = (SSDBImpl *) ssdb;
        this->regProcs();
        this->status = true;
    }

    ~ServerContext() = default;

    SSDBImpl *db = nullptr;

    vcdb::ProcMap procMap;

    std::atomic<bool> status;

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
