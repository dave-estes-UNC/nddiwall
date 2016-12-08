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

#include <grpc++/grpc++.h>

#include "GrpcNddiDisplay.h"

using namespace std;
using namespace nddi;

const size_t DISPLAY_WIDTH = 1024;
const size_t DISPLAY_HEIGHT = 768;

int main(int argc, char** argv) {
    // Initialize the GRPC NDDI Display. It requires a channel, out of which the actual RPCs
    // are created. This channel models a connection to an endpoint (in this case,
    // localhost at port 50051). We indicate that the channel isn't authenticated
    // (use of InsecureChannelCredentials()).
    vector<unsigned int> frameVolumeDimensionalSizes;
    frameVolumeDimensionalSizes.push_back(DISPLAY_WIDTH);
    frameVolumeDimensionalSizes.push_back(DISPLAY_HEIGHT);
    GrpcNddiDisplay myDisplay(frameVolumeDimensionalSizes, DISPLAY_WIDTH, DISPLAY_HEIGHT, 1, 2,
            grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

    cout << "Width is " << myDisplay.DisplayWidth() << endl;
    cout << "Height is " << myDisplay.DisplayHeight() << endl;
    cout << "Numer of Coefficient Planes is " << myDisplay.NumCoefficientPlanes() << endl;


  return 0;
}
