#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "Configuration.h"

#include "GrpcNddiDisplay.h"
#include "RecorderNddiDisplay.h"

using namespace nddi;

Configuration globalConfiguration = Configuration();
char * RECORDING_FILE = nullptr;

bool parseArgs(int argc, char *argv[]) {
    argc--;
    argv++;

    while (argc) {
        if (strcmp(*argv, "--subregion") == 0) {
            globalConfiguration.sub_x = atoi(argv[1]);
            globalConfiguration.sub_y = atoi(argv[2]);
            globalConfiguration.sub_w = atoi(argv[3]);
            globalConfiguration.sub_h = atoi(argv[4]);
            globalConfiguration.isSlave = true;
            argc -= 5;
            argv += 5;
        } else {
            RECORDING_FILE = *argv;
            argc--;
            argv++;
        }
    }

    if (!RECORDING_FILE) {
        return false;
    } else {
        return true;
    }
}

int main(int argc, char *argv[]) {

    if (!parseArgs(argc, argv)) {
        std::cout << "Ussage: nddiwall_player [--subregion <x> <y> <width> <height>] <record-filename>" << std::endl;
        return -1;
    }

    RecorderNddiDisplay* myDisplay = new RecorderNddiDisplay(RECORDING_FILE);
    myDisplay->Play();
    delete(myDisplay);

    return 0;
}
