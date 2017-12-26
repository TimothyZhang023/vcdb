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

int Application::usage(int argc, char **argv) {
    printf("Usage:\n");
    printf("    %s [-d] /path/to/app.conf [-s start|stop|restart]\n", argv[0]);
    printf("Options:\n");
    printf("    -d    run as daemon\n");
    printf("    -s    option to start|stop|restart the server\n");
    printf("    -h    show this message\n");
    return 0;
}


int Application::parse(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-d") {
            appArgs.isDaemon = true;
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
                appArgs.port = str_to_int(argv[i]) ;
            } else {
                usage(argc, argv);
                exit(1);
            }
        } else {
            appArgs.confFile = argv[i];
        }
    }

    return 0;
}

int Application::init() {


    if ((!appArgs.confFile.empty()) && (!is_file(appArgs.confFile))) {
        fprintf(stderr, "'%s' is not a file or not exists!\n", appArgs.confFile.c_str());
        exit(1);
    }

    conf = Config::load(appArgs.confFile.c_str());
    if (!conf) {
        fprintf(stderr, "error loading conf file: '%s'\n", appArgs.confFile.c_str());
        exit(1);
    }

    {
        std::string conf_dir = real_dirname(appArgs.confFile.c_str());
        if (chdir(conf_dir.c_str()) == -1) {
            fprintf(stderr, "error chdir: %s\n", conf_dir.c_str());
            exit(1);
        }
    }

    appArgs.pidFile = conf->get_str("pidfile");

    if (appArgs.action == "stop") {
        appArgs.killByPidFile();
        exit(0);
    }
    if (appArgs.action == "restart") {
        if (file_exists(appArgs.pidFile)) {
            appArgs.killByPidFile();
        }
    }


    appArgs.checkPidFile();


    { // logger
        std::string log_output;
        std::string log_level_;
        int64_t log_rotate_size;

        log_level_ = conf->get_str("logger.level");
        strtolower(&log_level_);
        if (log_level_.empty()) {
            log_level_ = "debug";
        }
        int level = Logger::get_level(log_level_.c_str());
        log_rotate_size = conf->get_int64("logger.rotate_size", 99999999999);
        log_output = conf->get_str("logger.output");
        if (log_output.empty()) {
            log_output = "stdout";
        }
        if (log_open(log_output.c_str(), level, true, log_rotate_size) == -1) {
            fprintf(stderr, "error opening log file: %s\n", log_output.c_str());
            exit(1);
        }
    }

    appArgs.port = conf->get_num("server.port", appArgs.port);

    appArgs.workDir = conf->get_str("work_dir");
    if (appArgs.workDir.empty()) {
        appArgs.workDir = ".";
    }
    if (!is_dir(appArgs.workDir)) {
        fprintf(stderr, "'%s' is not a directory or not exists!\n", appArgs.workDir.c_str());
        exit(1);
    }

    // WARN!!!
    // deamonize() MUST be called before any thread is created!
    if (appArgs.isDaemon) {
        daemonize();
    }

}

int Application::run() {
    appArgs.writePid();
    go();
    appArgs.removePidFile();
}

int Application::go() {
    this->signalSetup();


    Options option;
    option.load(conf);

    std::string data_db_dir = appArgs.workDir;

//    log_info("vcdb-server %s", APP_VERSION);
//    log_info("build_version %s", APP_GIT_BUILD);
//    log_info("build_date %s", APP_BUILD_DATE);
    log_info("conf_file        : %s", appArgs.confFile.c_str());
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


    SSDB *data_db = nullptr;
    data_db = SSDB::open(option, data_db_dir);
    if (!data_db) {
        log_fatal("could not open data db: %s", data_db_dir.c_str());
        fprintf(stderr, "could not open data db: %s\n", data_db_dir.c_str());
        exit(1);
    }


    VcServer *server;
    server = new VcServer(data_db);

    log_info("vcdb server starting on 0.0.0.0:%d", appArgs.port);

    std::unique_ptr<pink::ConnFactory> my_conn_factory = std::unique_ptr<pink::ConnFactory>(
            new VcServerConnFactory(server));
    pink::ServerHandle *rdcProxyServerHandle = new RdcProxyServerHandle();

    pink::ServerThread *st = NewDispatchThread(appArgs.port, 10, my_conn_factory.get(), 1000, 1000, rdcProxyServerHandle);

    if (st->StartThread() != 0) {
        printf("StartThread error happened!\n");
        exit(-1);
    }
    running.store(true);

    log_info("vcdb server ready.");
    log_info("pidfile: %s, pid: %d", appArgs.pidFile.c_str(), (int) getpid());
    log_info("vcdb server started.");

    while (running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    st->StopThread();

    delete st;
    delete rdcProxyServerHandle;

    delete server;
    delete data_db;
    delete conf;

    log_info("server stopped");
}

void Application::signalSetup() {
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, &IntSigHandle);
    signal(SIGQUIT, &IntSigHandle);
    signal(SIGTERM, &IntSigHandle);
}


void IntSigHandle(const int sig) {
    log_info("catch signal %d, cleanup...\n", sig);
    running.store(false);
    log_info("server exiting");
}


