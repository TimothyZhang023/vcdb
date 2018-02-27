//
// Created by zts on 12/18/17.
//

#ifndef VCDB_APP_H
#define VCDB_APP_H


#include <atomic>
#include <string>
#include <util/file.h>
#include "AppArgs.h"

class BinlogManager;
class Config;

namespace vcdb {

    static std::atomic<bool> running(false);

    void IntSigHandle(int sig);


    class Application {
    public:

        Application() = default;;

        virtual ~Application();

        int Parse(int argc, char **argv);

        int Run();

        int Init();

        int CronTask(BinlogManager *bm);

        void SignalSetup();

    private:
        int usage(int argc, char **argv);

        AppArgs appArgs;
        Config *conf = nullptr;

        int go();

    };

}

#endif //VCDB_APP_H
