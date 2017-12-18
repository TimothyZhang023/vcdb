//
// Created by zts on 12/18/17.
//

#ifndef VCDB_APP_H
#define VCDB_APP_H


#include <atomic>
#include <string>

static std::atomic<bool> running(false);

void IntSigHandle(int sig);

class Config;

class App {
public:
    App() {
    };

    virtual ~App() = default;;

    int usage(int argc, char **argv);
    int parse(int argc, char **argv);

    int run();
    int init();

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
    Config *conf;

    int go();


    int read_pid();
    void write_pid();
    void check_pidfile();
    void remove_pidfile();
    void kill_process();
};


#endif //VCDB_APP_H
