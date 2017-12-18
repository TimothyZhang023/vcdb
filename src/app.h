//
// Created by zts on 12/18/17.
//

#ifndef VCDB_APP_H
#define VCDB_APP_H



#include <atomic>

static std::atomic<bool> running(false);

void IntSigHandle(int sig);

class App {
public:
    App() {
    };

    virtual ~App() = default;;

    int entrance(int argc, char **argv);

    int run();

    void signalSetup();

private:
};


#endif //VCDB_APP_H
