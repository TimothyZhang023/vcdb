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

class SSDB;

class Binlog;

namespace vcdb {

    class ApplicationContextContainer {
    public:
        ApplicationContextContainer() = default;

        SSDB *ssdb;
        Binlog *binlog;
        BinlogManager *bm;


    };

    static std::atomic<bool> running(false);

    void IntSigHandle(int sig);


    class Application {
    public:

        Application() = default;;

        virtual ~Application();

        int Parse(int argc, char **argv);

        int Run();

        int Init();

        int CronTask();

        void SignalSetup();

        ApplicationContextContainer container;

    private:
        int usage(int argc, char **argv);

        AppArgs appArgs;
        Config *conf = nullptr;

        int go();

    };

}

#endif //VCDB_APP_H
