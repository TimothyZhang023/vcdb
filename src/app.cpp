//
// Created by zts on 12/18/17.
//

#include <iostream>
#include <csignal>
#include <thread>

#include "pink/include/redis_conn.h"


#include "app.h"
#include "conn.h"


extern std::atomic<bool> running;

class RdcProxyServerHandle : public pink::ServerHandle {

    //Client

    int CreateWorkerSpecificData(void **data) const override {
        UNUSED(data);
        return 0;
    }

    int DeleteWorkerSpecificData(void *data) const override {
        UNUSED(data);
        return 0;
    }
};


int App::entrance(int argc, char **argv) {
    this->signalSetup();

    return this->run();
}

int App::run() {

    int port = 6379;

    std::cout << "Server startup on 0.0.0.0:"<< port << std::endl;


    std::unique_ptr<pink::ConnFactory> my_conn_factory = std::unique_ptr<pink::ConnFactory>(new VcServerConnFactory());
    pink::ServerHandle *rdcProxyServerHandle = new RdcProxyServerHandle();

    pink::ServerThread *st = NewDispatchThread(port, 10, my_conn_factory.get(), 1000, 1000, rdcProxyServerHandle);

    if (st->StartThread() != 0) {
        printf("StartThread error happened!\n");
        exit(-1);
    }
    running.store(true);
    while (running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    st->StopThread();

    delete st;
    delete rdcProxyServerHandle;


    std::cout << "Server stopped!" << std::endl;

}

void App::signalSetup() {
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, &IntSigHandle);
    signal(SIGQUIT, &IntSigHandle);
    signal(SIGTERM, &IntSigHandle);
}


void IntSigHandle(const int sig) {
    printf("Catch Signal %d, cleanup...\n", sig);
    running.store(false);
    printf("server Exit");
}

