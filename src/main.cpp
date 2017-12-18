#include "app.h"

int main(int argc, char **argv) {

    App applicationServer;
    applicationServer.parse(argc, argv);

    applicationServer.init();
    applicationServer.run();

    return 0;
}