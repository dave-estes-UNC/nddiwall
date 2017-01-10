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
    vector<unsigned int> frameVolumeDimensionalSizes;
    frameVolumeDimensionalSizes.push_back(DISPLAY_WIDTH);
    frameVolumeDimensionalSizes.push_back(DISPLAY_HEIGHT);
    frameVolumeDimensionalSizes.push_back(2);
    GrpcNddiDisplay myDisplay(frameVolumeDimensionalSizes, DISPLAY_WIDTH, DISPLAY_HEIGHT, 1, 3,
            grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

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

    // Set the only plane to full on.
    Scaler s;
    s.r = s.g = s.b = s.a = myDisplay.GetFullScaler();
    myDisplay.FillScaler(s, start, end);

    // Fill FrameBuffer with white and then black
    Pixel p;
    p.r = p.g = p.b = 0xff; p.a = 0xff;
    end[2] = 0;
    myDisplay.FillPixel(p, start, end);
    start[2] = end[2] = 1;
    p.r = p.g = p.b = 0x00;
    myDisplay.FillPixel(p, start, end);

    // Update the FrameBuffer with just one blue pixel at (10,10)
    vector<uint32_t> location;
    location.push_back(10); location.push_back(10); location.push_back(0);
    p.b = 0xff;
    myDisplay.PutPixel(p, location);

    // Sleep for 2s, change to black, then sleep for 2s and change back
    sleep(2);
    vector<int> input;
    input.push_back(0);
    input[0] = 1;
    myDisplay.UpdateInputVector(input);
    sleep(2);
    input[0] = 0;
    myDisplay.UpdateInputVector(input);

    return 0;
}
