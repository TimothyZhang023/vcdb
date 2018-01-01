//
// Created by zts on 12/18/17.
//

#include <iostream>
#include <csignal>
#include <thread>
#include <swapdb/ssdb/options.h>
#include <sstream>
#include <swapdb/ssdb/ssdb.h>
#include <swapdb/util/log.h>
#include <swapdb/serv.h>

#include "pink/include/redis_conn.h"


#include "Application.h"
#include "ClientConn.h"
#include "util/daemon.h"

namespace vcdb {

    class VcServerHandle : public pink::ServerHandle {

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
}

int vcdb::Application::usage(int argc, char **argv) {
    printf("Usage:\n");
    printf("    %s [-d] /path/to/app.conf [-s start|stop|restart]\n", argv[0]);
    printf("Options:\n");
    printf("    -d    run as daemon\n");
    printf("    -s    option to start|stop|restart the server\n");
    printf("    -h    show this message\n");
    return 0;
}


int vcdb::Application::Parse(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-d") {
            appArgs.is_daemon = true;
        } else if (arg == "-v") {
            exit(0);
        } else if (arg == "-h") {
            usage(argc, argv);
            exit(0);
        } else if (arg == "-s") {
            if (argc > i + 1) {
                i++;
                appArgs.action = argv[i];
            } else {
                usage(argc, argv);
                exit(1);
            }
            if (appArgs.action != "start" && appArgs.action != "stop" && appArgs.action != "restart") {
                usage(argc, argv);
                fprintf(stderr, "Error: bad argument: '%s'\n", appArgs.action.c_str());
                exit(1);
            }
        } else if (arg == "-p") {
            if (argc > i + 1) {
                i++;
                appArgs.port = str_to_int(argv[i]);
            } else {
                usage(argc, argv);
                exit(1);
            }
        } else {
            appArgs.conf_file = argv[i];
        }
    }

    return 0;
}

int vcdb::Application::Init() {


    if ((!appArgs.conf_file.empty()) && (!is_file(appArgs.conf_file))) {
        fprintf(stderr, "'%s' is not a file or not exists!\n", appArgs.conf_file.c_str());
        exit(1);
    }

    conf = Config::load(appArgs.conf_file.c_str());
    if (!conf) {
        fprintf(stderr, "error loading conf file: '%s'\n", appArgs.conf_file.c_str());
        exit(1);
    }

    {
        std::string conf_dir = real_dirname(appArgs.conf_file.c_str());
        if (chdir(conf_dir.c_str()) == -1) {
            fprintf(stderr, "error chdir: %s\n", conf_dir.c_str());
            exit(1);
        }
    }

    { //pid
        appArgs.pid_file = conf->get_str("pidfile", "/tmp/vcdb.pid");

        if (appArgs.action == "stop") {
            appArgs.KillByPidFile();
            exit(0);
        }

        if (appArgs.action == "restart") {
            if (file_exists(appArgs.pid_file)) {
                appArgs.KillByPidFile();
            }
        }

        appArgs.CheckPidFile();
    }


    { // logger
        std::string log_output;
        std::string log_level_;
        int64_t log_rotate_size;

        log_level_ = conf->get_str("logger.level", "debug");
        strtolower(&log_level_);
        int level = Logger::get_level(log_level_.c_str());

        log_rotate_size = conf->get_int64("logger.rotate_size", 99999999999);
        log_output = conf->get_str("logger.output", "stdout");

        if (log_open(log_output.c_str(), level, true, log_rotate_size) == -1) {
            fprintf(stderr, "error opening log file: %s\n", log_output.c_str());
            exit(1);
        }
    }

    appArgs.port = conf->get_num("server.port", appArgs.port);
    appArgs.ip = conf->get_str("server.ip", appArgs.ip.c_str());

    appArgs.work_dir = conf->get_str("work_dir");
    if (appArgs.work_dir.empty()) {
        appArgs.work_dir = ".";
    }
    if (!is_dir(appArgs.work_dir)) {
        fprintf(stderr, "'%s' is not a directory or not exists!\n", appArgs.work_dir.c_str());
        exit(1);
    }

    // WARN!!!
    // deamonize() MUST be called before any thread is created!
    if (appArgs.is_daemon) {
        daemonize();
    }

}

int vcdb::Application::Run() {
    appArgs.WritePid();
    go();
    appArgs.RemovePidFile();
}

int vcdb::Application::go() {
    this->SignalSetup();


    Options option;
    option.load(conf);

    std::string data_db_dir = appArgs.work_dir;

//    log_info("vcdb-server %s", APP_VERSION);
//    log_info("build_version %s", APP_GIT_BUILD);
//    log_info("build_date %s", APP_BUILD_DATE);
    log_info("conf_file        : %s", appArgs.conf_file.c_str());
    log_info("log_level        : %s", Logger::shared()->level_name().c_str());
    log_info("log_output       : %s", Logger::shared()->output_name().c_str());
    log_info("log_rotate_size  : %"
                     PRId64, Logger::shared()->rotate_size());

    log_info("main_db          : %s", data_db_dir.c_str());
    log_info("cache_size       : %d MB", option.cache_size);
    log_info("block_size       : %d KB", option.block_size);
    log_info("write_buffer     : %d MB", option.write_buffer_size);
    log_info("max_open_files   : %d", option.max_open_files);
    log_info("op_filters4hit   : %s", option.optimize_filters_for_hits ? "enable" : "disable");
    log_info("dy_level_bytes   : %s", option.level_compaction_dynamic_level_bytes ? "enable" : "disable");

    log_info("compression      : %s", option.compression ? "enable" : "disable");
    log_info("rdb_compression  : %s", option.rdb_compression ? "enable" : "disable");
    log_info("trans_compression: %s", option.transfer_compression ? "enable" : "disable");


    std::stringstream ss;
    ss << option;
    log_info("config           : %s", ss.str().c_str());

    std::unique_ptr<SSDB> data_db(SSDB::open(option, data_db_dir));
    if (!data_db) {
        log_fatal("could not open data db: %s", data_db_dir.c_str());
        fprintf(stderr, "could not open data db: %s\n", data_db_dir.c_str());
        exit(1);
    }

    std::unique_ptr<VcServer> server(new VcServer(data_db.get()));

    log_info("vcdb server starting on 0.0.0.0:%d", appArgs.port);

    std::unique_ptr<pink::ConnFactory> connFactory(new VcServerConnFactory(server.get()));
    std::unique_ptr<pink::ServerHandle> serverHandle(new VcServerHandle());
    std::unique_ptr<pink::ServerThread> serverThread(
            pink::NewDispatchThread(appArgs.ip, appArgs.port, 10, connFactory.get(), 1000, 1000,
                                    serverHandle.get()));

    if (serverThread->StartThread() != 0) {
        fprintf(stderr, "StartThread error happened!");
        exit(-1);
    }
    running.store(true);

    log_info("vcdb server is ready.");
    log_info("pidfile: %s, pid: %d", appArgs.pid_file.c_str(), (int) getpid());
    log_info("vcdb server started.");

    while (running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    serverThread->StopThread();
    log_info("server stopped");
}

void vcdb::Application::SignalSetup() {
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, &IntSigHandle);
    signal(SIGQUIT, &IntSigHandle);
    signal(SIGTERM, &IntSigHandle);
}

vcdb::Application::~Application() {
    if (conf != nullptr) {
        delete conf;
        conf = nullptr;
    }
}


void vcdb::IntSigHandle(const int sig) {
    log_info("catch signal %d, cleanup...", sig);
    running.store(false);
    log_info("server exiting");
}

