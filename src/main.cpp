#include "Application.h"

int main(int argc, char **argv) {

    Application applicationServer;
    applicationServer.parse(argc, argv);

    applicationServer.init();
    applicationServer.run();

    return 0;
}