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

    int usage(int argc, char **argv);
    int parse(int argc, char **argv);
    int entrance(int argc, char **argv);

    int run();

    void signalSetup();


private:
    struct AppArgs{
        bool is_daemon;
        std::string pidfile;
        std::string conf_file;
        std::string work_dir;
        std::string start_opt;

        AppArgs(){
            is_daemon = false;
            start_opt = "start";
        }
    };

    AppArgs app_args;

};


#endif //VCDB_APP_H
