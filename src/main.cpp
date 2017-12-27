#include "Application.h"

int main(int argc, char **argv) {

    Application applicationServer;
    applicationServer.Parse(argc, argv);

    applicationServer.Init();
    applicationServer.Run();

    return 0;
}