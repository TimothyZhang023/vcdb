//
// Created by zts on 1/8/18.
//
#include <pink/include/server_thread.h>
#include <util/log.h>
#include <pink/include/redis_conn.h>
#include "SyncRedisMaster.h"


SyncRedisMaster::~SyncRedisMaster() {
    delete master_client;
}

SyncRedisMaster::SyncRedisMaster(ServerContext *ctx, const std::string &ip, int port) :
        ctx(ctx), master_ip(ip), master_port(port) {
    master_client = vcdb::NewRedisClient();
}

ServerContext *SyncRedisMaster::getCtx() const {
    return ctx;
}


void *SyncRedisMaster::ThreadMain() {
    Status s = Status::OK(), result;

    while (!should_stop()) {
        result = master_client->Connect(master_ip, master_port);

        log_info ("Connect <%s:%d> %s ", master_ip.c_str(), master_port, result.ToString().c_str());
        if (result.ok()) {
            std::string cmd;

            std::string my_id = "0";
            std::string offset = "0";

            cmd = RestoreRequest(std::vector<std::string>{"PSYNC", my_id, offset});
            master_client->Send(&cmd);

            pink::RedisCmdArgsType res;
            s = master_client->Recv(&res);
            if (res.empty()) {
                //error
                break;
            }

            std::string next = res[0];
            std::vector<std::string> v_res = strsplit(next, " ");
            if (v_res.size() < 3) {
                //error
                break;
            }

            std::string next_action = v_res[0];
            my_id = v_res[1];
            offset = v_res[2];
            log_info ("%s", hexcstr(InlineRequest(v_res)));


            int64_t rdb_size = 0;
            {
                auto size = master_client->BufferRead();
                if (size <= 0) {
                    break;
                }

                auto buf = master_client->ReadBytes(1);
                char *p;
                int len;

                if ((p = master_client->ReadLine(&len)) == nullptr) {
                    break;
                }
                std::string arg(p, len);
                rdb_size = str_to_int64(arg);
                log_info ("rdb: %d", rdb_size);
            }


            auto size = rdb_size;
            while (true) {
                if (rdb_size == 0) {
                    break;
                }
                if (size > 0) {
                    auto buf = master_client->ReadBytes(static_cast<unsigned int>(size));
                    if (buf != nullptr) {
                        std::string sbuf(buf, static_cast<unsigned long>(size));
                        log_info ("sbuf<%d> :%s", size, hexcstr(sbuf));
                        log_info ("got :%d bytes", size);
                        rdb_size = rdb_size - size;
                    }
                    size = master_client->BufferRead();

                } else {
                    log_error("read %d", size);
                    break;
                }
            }

            if (rdb_size != 0) {
                break;
            }


            while (true) {
                s = master_client->Recv(&res);
                if (!s.ok()) {
                    log_error("%s ", s.ToString().c_str());
                    break;
                }
                if (res.empty()) {
                    //error
                    break;
                }

                log_info ("%s", hexcstr(InlineRequest(res)));

                for (const auto &r : res) {
                    if (r == "PING") {
                        cmd = RestoreRequest(std::vector<std::string>{"PING"});
                        master_client->Send(&cmd);
                    }

                    if (r == "PONG") {
//                        cmd = RestoreRequest(std::vector<std::string>{"PING"});
//                        master_client->Send(&cmd);
                    }

                }


            }

//            size = master_client->BufferRead();
//            while (true) {
//                if (size > 0) {
//                    auto buf = master_client->ReadBytes(static_cast<unsigned int>(size));
//                    if (buf != nullptr) {
//                        std::string sbuf(buf, static_cast<unsigned long>(size));
//                        log_info ("sbuf<%d> :%s", size, hexcstr(sbuf));
//
//                        size = master_client->BufferRead();
//                        log_info ("got :%d bytes", size);
//                        rdb_size = rdb_size - size;
//                    }
//                } else {
//                    log_error("read %d", size);
//                    break;
//                }
//            }


            sleep(2000);

        }
        sleep(2);
    }
    return nullptr;
}

