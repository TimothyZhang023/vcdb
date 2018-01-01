#include "Application.h"

int main(int argc, char **argv) {

    vcdb::Application applicationServer;
    applicationServer.Parse(argc, argv);

    applicationServer.Init();
    applicationServer.Run();

    return 0;
}