#include <iostream>
#include "app.h"

int main(int argc, char **argv) {

    App applicationServer;
    applicationServer.parse(argc, argv);
    applicationServer.entrance(argc, argv);

    return 0;
}