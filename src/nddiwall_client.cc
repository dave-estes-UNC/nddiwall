/*
 *
 * Copyright 2015, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <iostream>
#include <memory>
#include <string>

#include <unistd.h>
#include <grpc++/grpc++.h>

#include "GrpcNddiDisplay.h"

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
    unsigned int frameVolumeDimensionalSizes[3];
    frameVolumeDimensionalSizes[0] = DISPLAY_WIDTH;
    frameVolumeDimensionalSizes[1] = DISPLAY_HEIGHT;
    frameVolumeDimensionalSizes[2] = 2;
    GrpcNddiDisplay myDisplay(3, frameVolumeDimensionalSizes, DISPLAY_WIDTH, DISPLAY_HEIGHT, 1, 3);

    std::cout << "Width is " << myDisplay.DisplayWidth() << std::endl;
    std::cout << "Height is " << myDisplay.DisplayHeight() << std::endl;
    std::cout << "Number of Coefficient Planes is " << myDisplay.NumCoefficientPlanes() << std::endl;

    // Initialize CoefficientPlane to all identity matrices
    int coeffs[] = {1, 0, 0, 0, 1, 0, 0, 0, 1};

    unsigned int start[] = {0, 0, 0};
    unsigned int end[] = {DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, 0};

    myDisplay.FillCoefficientMatrix(coeffs, start, end);
    myDisplay.Latch();

    // Set the only plane to full on.
    Scaler s;
    s.r = s.g = s.b = s.a = myDisplay.GetFullScaler();
    myDisplay.FillScaler(s, start, end);
    myDisplay.Latch();

    // Fill FrameBuffer with white and then black
    Pixel p;
    p.r = p.g = p.b = 0xff; p.a = 0xff;
    end[2] = 0;
    myDisplay.FillPixel(p, start, end);
    start[2] = end[2] = 1;
    p.r = p.g = p.b = 0x00;
    myDisplay.FillPixel(p, start, end);
    myDisplay.Latch();

    // Update the FrameBuffer with just one blue pixel at (10,10)
    unsigned int location[] = {10, 10, 0};
    p.b = 0xff;
    myDisplay.PutPixel(p, location);
    myDisplay.Latch();

    // Copy that section of the Frame Volume to another location
    start[0] = 0; start[1] = 0; start[2] = 0;
    end[0] = 10; end[1] = 10; end[2] = 0;
    location[0] = 30; location[1] = 30;
    myDisplay.CopyFrameVolume(start, end, location);
    myDisplay.Latch();

    // Copy a pixel strip and then a pixel array
    Pixel ps[32];
    for (int i = 0; i < 32; i++) {
        ps[i].r = ps[i].b = 00; ps[i].g = ps[i].a = 0xff;
    }
    start[0] = 20; start[1] = 10; start[2] = 0;
    end[0] = 51; end[1] = 10; end[2] = 0;
    myDisplay.CopyPixelStrip(ps, start, end);
    start[0] = 30; start[1] = 30; start[2] = 0;
    end[0] = 33; end[1] = 33; end[2] = 1;
    myDisplay.CopyPixels(ps, start, end);
    myDisplay.Latch();

    // Update coefficient matrix at (20,20) to use pixel at (10,10)
    coeffs[0 * 3 + 2] = coeffs[1 * 3 + 2] = -10;
    myDisplay.PutCoefficientMatrix(coeffs, location);
    myDisplay.Latch();

    // Sleep for 2s, change to black, then sleep for 2s and change back
    sleep(2);
    int input[] = {1};
    myDisplay.UpdateInputVector(input);
    sleep(2);
    start[0] = start[1] = 0;
    end[0] = DISPLAY_WIDTH - 1; end[1] = DISPLAY_HEIGHT - 1;
    start[2] = end[2] = 0;
    myDisplay.FillCoefficient(0, 2, 2, start, end);
    myDisplay.Latch();

    // Sleep again and then do the checkerboard blending
    sleep(2);
    s.r = s.g = s.b = s.a = myDisplay.GetFullScaler() >> 1;
    Scaler scalers[2];
    scalers[0].packed = scalers[1].packed = s.packed;
    unsigned int starts[] {40, 40, 0, 60, 60, 0};
    unsigned int size[] = {10, 10};
    myDisplay.FillScalerTiles(scalers, starts, size, 2);
    myDisplay.Latch();

    while (true) {
        usleep(1000);
        myDisplay.Latch();
    }

    return 0;
}
