//
// Created by zts on 12/18/17.
//

#ifndef VCDB_CONN_H
#define VCDB_CONN_H



#include <pink/include/pink_conn.h>
#include <pink/include/redis_conn.h>
#include <swapdb/common/context.hpp>

class SSDBServer;
class Buffer;

class VcClientConn : public pink::RedisConn {
public:
    VcClientConn(int fd, const std::string &ip_port, pink::ServerThread *thread,
                  void *worker_specific_data);

    ~VcClientConn() override;

protected:
    int DealMessage() override;

private:

    SSDBServer *server = nullptr;
    Context *ctx = nullptr;
};



class VcServerConnFactory : public pink::ConnFactory {
private:
    void *data = nullptr;

public:
    explicit VcServerConnFactory(void *data) : data(data) {}

    virtual pink::PinkConn *NewPinkConn(int connfd, const std::string &ip_port,
                                        pink::ServerThread *thread,
                                        void *worker_specific_data) const {
        return new VcClientConn(connfd, ip_port, thread, data);
    }
};




#endif //VCDB_CONN_H