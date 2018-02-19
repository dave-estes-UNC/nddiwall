#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "GrpcNddiDisplay.h"
#include "RecorderNddiDisplay.h"

using namespace nddi;

//const size_t DISPLAY_WIDTH = 1024;
//const size_t DISPLAY_HEIGHT = 768;
const size_t DISPLAY_WIDTH = 100;
const size_t DISPLAY_HEIGHT = 100;

int main(int argc, char** argv) {
    // Initialize the GRPC NDDI Display. It requires a channel, out of which the actual RPCs
    // are created. This channel models a connection to an endpoint (in this case,
    // localhost at port 50051). We indicate that the channel isn't authenticated
    // (use of InsecureChannelCredentials()).
    vector<unsigned int> frameVolumeDimensionalSizes = {DISPLAY_WIDTH, DISPLAY_HEIGHT, 2};
    NDimensionalDisplayInterface* myDisplay = NULL;
    RecorderNddiDisplay* myRecorder = NULL;
    GrpcNddiDisplay* myDisplayWall = NULL;
    if (argc == 1) {
        myDisplay = myDisplayWall = new GrpcNddiDisplay(frameVolumeDimensionalSizes,
                                                        DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                                        (unsigned int)1, (unsigned int)3);
    } else if (argc == 3 && strcmp(argv[1], "-r") == 0) {
        myDisplay = myRecorder = new RecorderNddiDisplay(frameVolumeDimensionalSizes,
                                                         DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                                         (unsigned int)1, (unsigned int)3, argv[2]);
    } else {
        std::cout << "Ussage: nddiwall_client [-r <recording>]" << std::endl;
        return -1;
    }

    std::cout << "Width is " << myDisplay->DisplayWidth() << std::endl;
    std::cout << "Height is " << myDisplay->DisplayHeight() << std::endl;
    std::cout << "Number of Coefficient Planes is " << myDisplay->NumCoefficientPlanes() << std::endl;

    // Initialize CoefficientPlane to all identity matrices
    vector< vector<int> > coeffs;
    coeffs.resize(3);
    coeffs[0].push_back(1); coeffs[0].push_back(0); coeffs[0].push_back(0);
    coeffs[1].push_back(0); coeffs[1].push_back(1); coeffs[1].push_back(0);
    coeffs[2].push_back(0); coeffs[2].push_back(0); coeffs[2].push_back(1);

    vector<unsigned int> start = {0, 0, 0};
    vector<unsigned int> end = {DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, 0};

    myDisplay->FillCoefficientMatrix(coeffs, start, end);
    if (myDisplayWall) { myDisplayWall->Latch(); } else if (myRecorder) { myRecorder->Latch(); } else if (myRecorder) { myRecorder->Latch(); }

    // Set the only plane to full on.
    Scaler s;
    s.r = s.g = s.b = s.a = myDisplay->GetFullScaler();
    myDisplay->FillScaler(s, start, end);
    if (myDisplayWall) { myDisplayWall->Latch(); } else if (myRecorder) { myRecorder->Latch(); }

    // Fill FrameBuffer with white and then black
    Pixel p;
    p.r = p.g = p.b = 0xff; p.a = 0xff;
    end[2] = 0;
    myDisplay->FillPixel(p, start, end);
    start[2] = end[2] = 1;
    p.r = p.g = p.b = 0x00;
    myDisplay->FillPixel(p, start, end);
    if (myDisplayWall) { myDisplayWall->Latch(); } else if (myRecorder) { myRecorder->Latch(); }

    // Update the FrameBuffer with just one blue pixel at (10,10)
    vector<uint32_t> location = {10, 10, 0};
    p.b = 0xff;
    myDisplay->PutPixel(p, location);
    if (myDisplayWall) { myDisplayWall->Latch(); } else if (myRecorder) { myRecorder->Latch(); }

    // Copy that section of the Frame Volume to another location
    start[0] = 0; start[1] = 0; start[2] = 0;
    end[0] = 10; end[1] = 10; end[2] = 0;
    location[0] = 30; location[1] = 30;
    myDisplay->CopyFrameVolume(start, end, location);
    if (myDisplayWall) { myDisplayWall->Latch(); } else if (myRecorder) { myRecorder->Latch(); }

    // Copy a pixel strip and then a pixel array
    Pixel ps[32];
    for (int i = 0; i < 32; i++) {
        ps[i].r = ps[i].b = 00; ps[i].g = ps[i].a = 0xff;
    }
    start[0] = 20; start[1] = 10; start[2] = 0;
    end[0] = 51; end[1] = 10; end[2] = 0;
    myDisplay->CopyPixelStrip(ps, start, end);
    start[0] = 30; start[1] = 30; start[2] = 0;
    end[0] = 33; end[1] = 33; end[2] = 1;
    myDisplay->CopyPixels(ps, start, end);
    if (myDisplayWall) { myDisplayWall->Latch(); } else if (myRecorder) { myRecorder->Latch(); }

    // Update coefficient matrix at (20,20) to use pixel at (10,10)
    coeffs[2][0] = coeffs[2][1] = -10;
    location[0] = location[1] = 20;
    myDisplay->PutCoefficientMatrix(coeffs, location);
    if (myDisplayWall) { myDisplayWall->Latch(); } else if (myRecorder) { myRecorder->Latch(); }

    // Sleep for 2s, change to black, then sleep for 2s and change back
    if (myDisplayWall) { sleep(2); }
    vector<int> input = {1};
    myDisplay->UpdateInputVector(input);
    if (myDisplayWall) { sleep(2); }
    start[0] = start[1] = 0;
    end[0] = DISPLAY_WIDTH - 1; end[1] = DISPLAY_HEIGHT - 1;
    start[2] = end[2] = 0;
    myDisplay->FillCoefficient(0, 2, 2, start, end);
    if (myDisplayWall) { myDisplayWall->Latch(); } else if (myRecorder) { myRecorder->Latch(); }

    // Sleep again and then do the checkerboard blending
    if (myDisplayWall) { sleep(2); }
    s.r = s.g = s.b = s.a = myDisplay->GetFullScaler() >> 1;
    vector<uint64_t> scalers = {s.packed, s.packed};
    vector<unsigned int> start1 = {40, 40, 0};
    vector<unsigned int> start2 = {60, 60, 0};
    vector<vector<unsigned int> > starts = {start1, start2};
    vector<unsigned int> size = {10, 10};
    myDisplay->FillScalerTiles(scalers, starts, size);
    if (myDisplayWall) { myDisplayWall->Latch(); } else if (myRecorder) { myRecorder->Latch(); }

    sleep(3);
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
