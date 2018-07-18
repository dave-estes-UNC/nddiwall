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
vector<unsigned int> frameVolumeDimensionalSizes;

bool parseArgs(int argc, char *argv[]) {
    argc--;
    argv++;

    while (argc) {
        if (strcmp(*argv, "--display") == 0) {
            DISPLAY_WIDTH = atoi(argv[1]);
            DISPLAY_HEIGHT = atoi(argv[2]);
            argc -= 3;
            argv += 3;
        } else if (strcmp(*argv, "--record") == 0) {
            RECORDING_FILE = argv[1];
            argc -= 2;
            argv += 2;
        } else if (strcmp(*argv, "--fv") == 0) {
            argc--;
            argv++;
            char *p = strtok(*argv, ",");
            while (p != NULL) {
                frameVolumeDimensionalSizes.push_back(atoi(p));
                p = strtok(NULL, ",");
            }
            argc--;
            argv++;
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
        std::cout << "Ussage: nddiwall_client --display <width> <height> [--record <recording>] [--fv x,y,z,...]" << std::endl;
        return -1;
    }

    // Initialize the GRPC NDDI Display. It requires a channel, out of which the actual RPCs
    // are created. This channel models a connection to an endpoint (in this case,
    // localhost at port 50051). We indicate that the channel isn't authenticated
    // (use of InsecureChannelCredentials()).
    NDimensionalDisplayInterface* myDisplay = NULL;
    RecorderNddiDisplay* myRecorder = NULL;
    GrpcNddiDisplay* myDisplayWall = NULL;
    if (!RECORDING_FILE) {
        myDisplay = myDisplayWall = new GrpcNddiDisplay(frameVolumeDimensionalSizes,
                                                        DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                                        (unsigned int)64, (unsigned int)3,
                                                        true, true);
    } else {
        std::cout << endl <<
                "WARNING: While you can certainly record the commands from a master client," << std::endl <<
                "keep in mind that the master client still kills the display when it's finished." << std::endl <<
                "This command will get recorded, and so when it's played back it will kill the display." << std::endl <<
                "Instead of recording and playing back the master client. It's best to actually launch a live" << std::endl <<
                "master client and then play back the slave client recordings." << std::endl << std::endl;
        myDisplay = myRecorder = new RecorderNddiDisplay(frameVolumeDimensionalSizes,
                                                         DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                                         (unsigned int)64, (unsigned int)3,
                                                         RECORDING_FILE,
                                                         true, true);
    }

    std::cout << "Width is " << myDisplay->DisplayWidth() << std::endl;
    std::cout << "Height is " << myDisplay->DisplayHeight() << std::endl;
    std::cout << "Frame Volume dimensions are";
    for (size_t i = 0; i < frameVolumeDimensionalSizes.size(); i++) {
        std::cout << " " << frameVolumeDimensionalSizes[i];
    }
    std::cout << std::endl;
    std::cout << "Number of Coefficient Planes is " << myDisplay->NumCoefficientPlanes() << std::endl;

    if (!RECORDING_FILE) {
        std::cout << std::endl << "Press <q>-<Enter> to Quit..." << std::endl;
        char key = 'a';
        while (key != 'q') {
            std::cin >> key;
        }
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
