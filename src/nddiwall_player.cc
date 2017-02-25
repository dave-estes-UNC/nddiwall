#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "GrpcNddiDisplay.h"
#include "RecorderNddiDisplay.h"

using namespace nddi;

int main(int argc, char** argv) {
    RecorderNddiDisplay* myDisplay = NULL;
    if (argc == 2) {
        myDisplay = new RecorderNddiDisplay(argv[1]);
        myDisplay->Play();
        delete(myDisplay);
    } else {
        std::cout << "Ussage: nddiwall_player <recording>" << std::endl;
        return -1;
    }

    return 0;
}
