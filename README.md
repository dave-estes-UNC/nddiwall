nddiwall
========

nddiwall is a simulated implementation of a large display wall
built as a singled NDDI display. Features include:

-   Fast and efficient protocol built using GRPC and protocol
    buffers
-   Multi-client support
-   Simulated NDDI displauy is implemented in C++ for CPU or with
    OpenCL for GPU or CPU. Note: large displays using a great amount
    of RAM, so you're likely stuck with a CPU implementation for
    anything over 1080p.
-   A command recorder/player
-   Complete cost model for evaluating RAM usage patterns,
    transmission cost, command used, etc.

Building
--------

Get the source:

    git clone https://github.com/dave-estes-UNC/nddiwall.git
    cd nddiwall
    git submodule update --init --recursive

From the top-level-directory, you can build Release:

    mkdir release
    cd release
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make -j8

or Debug:

    mkdir debug
    cd debug
    cmake -DCMAKE_BUILD_TYPE=Debug ..
    make -j8

Most configuration options are done using `*Features.h` files. The comments
around each #define should explain their usage.

-   src/PixelBridgeFeatures.h
-   src/nddi/Features.h

Usage
-----

Then to run the simple unit test (nddiwall_client):

    ./nddiwall_server &
    ./nddiwall_test_client

To run a proper pixelbridge client:

    ./nddiwall_server &
    ./nddiwall_pixelbridge_client <options> <path-to-video>

For multiple clients, a master client must first configure the display,
and then slave clients can render to their portions of the display. There's
currently no sophisticated mechanism for reserving areas of the display.
The master client should be run in the foreground in a shell and the pixelbridge
client should run in a seperate shell additionally using the `--subregion` option.

    ./nddiwall_server &
    ./nddiwall_master_client --display 1000 1000
    ./nddiwall_pixelbridge_client --subregion 600 400 <options> <path-to-video>
 
If you want to record commands with pixelbridge and playback later.

    ./pixelbridge <options> --record <record-filename> <path-to-video>
    ./nddiwall_server &
    ./nddiwall_player <record-filename>
