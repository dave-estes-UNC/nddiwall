#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "GrpcNddiDisplay.h"
#include "RecorderNddiDisplay.h"

using namespace nddi;

unsigned int DISPLAY_WIDTH = 0;
unsigned int DISPLAY_HEIGHT = 0;
char* RECORDING_FILE = nullptr;

bool parseArgs(int argc, char *argv[]) {
    argc--;
    argv++;

    while (argc) {
        if (strcmp(*argv, "-d") == 0) {
            DISPLAY_WIDTH = atoi(argv[1]);
            DISPLAY_HEIGHT = atoi(argv[2]);
            argc -= 3;
            argv += 3;
        } else if (strcmp(*argv, "--record") == 0) {
            RECORDING_FILE = argv[1];
            argc -= 2;
            argv += 2;
        } else {
            return false;
        }
    }

    if (!DISPLAY_WIDTH || !DISPLAY_HEIGHT) {
        return false;
    } else {
        return true;
    }
}

int main(int argc, char** argv) {

    if (!parseArgs(argc, argv)) {
        std::cout << "Ussage: nddiwall_client -d <width> <height>[-r <recording>]" << std::endl;
        return -1;
    }

    // Initialize the GRPC NDDI Display. It requires a channel, out of which the actual RPCs
    // are created. This channel models a connection to an endpoint (in this case,
    // localhost at port 50051). We indicate that the channel isn't authenticated
    // (use of InsecureChannelCredentials()).
    vector<unsigned int> frameVolumeDimensionalSizes = {DISPLAY_WIDTH, DISPLAY_HEIGHT, (unsigned int)64};
    NDimensionalDisplayInterface* myDisplay = NULL;
    RecorderNddiDisplay* myRecorder = NULL;
    GrpcNddiDisplay* myDisplayWall = NULL;
    if (!RECORDING_FILE) {
        myDisplay = myDisplayWall = new GrpcNddiDisplay(frameVolumeDimensionalSizes,
                                                        DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                                        (unsigned int)64, (unsigned int)3);
    } else {
        myDisplay = myRecorder = new RecorderNddiDisplay(frameVolumeDimensionalSizes,
                                                         DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                                         (unsigned int)64, (unsigned int)3,
                                                         RECORDING_FILE);
    }

    std::cout << "Width is " << myDisplay->DisplayWidth() << std::endl;
    std::cout << "Height is " << myDisplay->DisplayHeight() << std::endl;
    std::cout << "Number of Coefficient Planes is " << myDisplay->NumCoefficientPlanes() << std::endl;

    std::cout << std::endl << "Press <q>-<Enter> to Quit..." << std::endl;
    char key = 'a';
    while (key != 'q') {
        std::cin >> key;
    }

    if (myDisplayWall) {
        myDisplayWall->Shutdown();
        delete(myDisplayWall);
    }
    if (myRecorder) {
        myRecorder->Shutdown();
        delete(myRecorder);
    }

    return 0;
}
