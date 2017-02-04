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
    vector< vector<int> > coeffs;
    coeffs.resize(3);
    coeffs[0].push_back(1); coeffs[0].push_back(0); coeffs[0].push_back(0);
    coeffs[1].push_back(0); coeffs[1].push_back(1); coeffs[1].push_back(0);
    coeffs[2].push_back(0); coeffs[2].push_back(0); coeffs[2].push_back(1);

    vector<unsigned int> start, end;
    start.clear(); end.clear();

    start.push_back(0); start.push_back(0); start.push_back(0);
    end.push_back(DISPLAY_WIDTH - 1); end.push_back(DISPLAY_HEIGHT - 1); end.push_back(0);

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
    vector<uint32_t> location;
    location.push_back(10); location.push_back(10); location.push_back(0);
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
    coeffs[2][0] = coeffs[2][1] = -10;
    location[0] = location[1] = 20;
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
    vector<uint64_t> scalers;
    scalers.push_back(s.packed); scalers.push_back(s.packed);
    vector<unsigned int> start1;
    start1.push_back(40); start1.push_back(40); start1.push_back(0);
    vector<unsigned int> start2;
    start2.push_back(60); start2.push_back(60); start2.push_back(0);
    vector<vector<unsigned int> > starts;
    starts.push_back(start1); starts.push_back(start2);
    vector<unsigned int> size;
    size.push_back(10); size.push_back(10);
    myDisplay.FillScalerTiles(scalers, starts, size);
    myDisplay.Latch();

    while (true) {
        usleep(1000);
        myDisplay.Latch();
    }

    return 0;
}
