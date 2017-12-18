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


#include "app.h"
#include "conn.h"
#include "util/file.h"


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

int App::usage(int argc, char **argv) {
    printf("Usage:\n");
    printf("    %s [-d] /path/to/app.conf [-s start|stop|restart]\n", argv[0]);
    printf("Options:\n");
    printf("    -d    run as daemon\n");
    printf("    -s    option to start|stop|restart the server\n");
    printf("    -h    show this message\n");
    return 0;
}


int App::parse(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-d") {
            app_args.is_daemon = true;
        } else if (arg == "-v") {
            exit(0);
        } else if (arg == "-h") {
            usage(argc, argv);
            exit(0);
        } else if (arg == "-s") {
            if (argc > i + 1) {
                i++;
                app_args.start_opt = argv[i];
            } else {
                usage(argc, argv);
                exit(1);
            }
            if (app_args.start_opt != "start" && app_args.start_opt != "stop" && app_args.start_opt != "restart") {
                usage(argc, argv);
                fprintf(stderr, "Error: bad argument: '%s'\n", app_args.start_opt.c_str());
                exit(1);
            }
        } else {
            app_args.conf_file = argv[i];
        }
    }

    if (app_args.conf_file.empty()) {
        usage(argc, argv);
        exit(1);
    }
    return 0;
}

int App::entrance(int argc, char **argv) {
    this->signalSetup();

    return this->run();
}

int App::run() {
    if (!is_file(app_args.conf_file.c_str())) {
        fprintf(stderr, "'%s' is not a file or not exists!\n", app_args.conf_file.c_str());
        exit(1);
    }
    Config *conf = Config::load(app_args.conf_file.c_str());
    if (!conf) {
        fprintf(stderr, "error loading conf file: '%s'\n", app_args.conf_file.c_str());
        exit(1);
    }
    {
        std::string conf_dir = real_dirname(app_args.conf_file.c_str());
        if (chdir(conf_dir.c_str()) == -1) {
            fprintf(stderr, "error chdir: %s\n", conf_dir.c_str());
            exit(1);
        }
    }

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
        log_rotate_size = conf->get_int64("logger.rotate_size");
        log_output = conf->get_str("logger.output");
        if (log_output == "") {
            log_output = "stdout";
        }
        if (log_open(log_output.c_str(), level, true, log_rotate_size) == -1) {
            fprintf(stderr, "error opening log file: %s\n", log_output.c_str());
            exit(1);
        }
    }

    app_args.work_dir = conf->get_str("work_dir");
    if (app_args.work_dir.empty()) {
        app_args.work_dir = ".";
    }
    if (!is_dir(app_args.work_dir.c_str())) {
        fprintf(stderr, "'%s' is not a directory or not exists!\n", app_args.work_dir.c_str());
        exit(1);
    }


    Options option;
    option.load(conf);

    std::string data_db_dir = app_args.work_dir;

    log_info("ssdb-server %s", APP_VERSION);
    log_info("build_version %s", APP_GIT_BUILD);
    log_info("build_date %s", APP_BUILD_DATE);
    log_info("conf_file        : %s", app_args.conf_file.c_str());
    log_info("log_level        : %s", Logger::shared()->level_name().c_str());
    log_info("log_output       : %s", Logger::shared()->output_name().c_str());
    log_info("log_rotate_size  : %"
                     PRId64, Logger::shared()->rotate_size());

    log_info("main_db          : %s", data_db_dir.c_str());
    log_info("cache_size       : %d MB", option.cache_size);
    log_info("block_size       : %d KB", option.block_size);
#ifdef USE_LEVELDB
    log_info("compaction_speed : %d MB/s", option.compaction_speed);
#endif
    log_info("write_buffer     : %d MB", option.write_buffer_size);
    log_info("max_open_files   : %d", option.max_open_files);
    log_info("op_filters4hit   : %s", option.optimize_filters_for_hits ? "enable" : "disable");
    log_info("dy_level_bytes   : %s", option.level_compaction_dynamic_level_bytes ? "enable" : "disable");

    log_info("compression      : %s", option.compression ? "enable" : "disable");
    log_info("rdb_compression  : %s", option.rdb_compression ? "enable" : "disable");
    log_info("trans_compression: %s", option.transfer_compression ? "enable" : "disable");
    log_info("sync_speed       : %d MB/s", conf->get_num("replication.sync_speed"));

#ifdef PTIMER
    log_info("ptimer           : enable");
#endif
#ifdef DREPLY
    log_info("dreply           : enable");
#endif

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

//	meta_db = SSDB::open(Options(), meta_db_dir);
//	if(!meta_db){
//		log_fatal("could not open meta db: %s", meta_db_dir.c_str());
//		fprintf(stderr, "could not open meta db: %s\n", meta_db_dir.c_str());
//		exit(1);
//	}

    SSDBServer *server;
    server = new SSDBServer(data_db);

    log_info("pidfile: %s, pid: %d", app_args.pidfile.c_str(), (int) getpid());
    log_info("ssdb server ready.");


    int port = conf->get_num("server.port");

    std::cout << "Server startup on 0.0.0.0:" << port << std::endl;


    std::unique_ptr<pink::ConnFactory> my_conn_factory = std::unique_ptr<pink::ConnFactory>(
            new VcServerConnFactory(server));
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

    delete server;
    delete data_db;
    delete conf;

    log_info("%s exit.", APP_NAME);

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

