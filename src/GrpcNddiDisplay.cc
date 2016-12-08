#include "GrpcNddiDisplay.h"

using namespace nddi;
using namespace std;

// public

GrpcNddiDisplay::GrpcNddiDisplay() {}

GrpcNddiDisplay::GrpcNddiDisplay(vector<unsigned int> &frameVolumeDimensionalSizes,
                                 unsigned int numCoefficientPlanes, unsigned int inputVectorSize,
                                 shared_ptr<Channel> channel)
: GrpcNddiDisplay(frameVolumeDimensionalSizes, 640, 480, numCoefficientPlanes, inputVectorSize, channel) {
}

GrpcNddiDisplay::GrpcNddiDisplay(vector<unsigned int> &frameVolumeDimensionalSizes,
                                 unsigned int displayWidth, unsigned int displayHeight,
                                 unsigned int numCoefficientPlanes, unsigned int inputVectorSize,
                                 shared_ptr<Channel> channel)
: stub_(NddiWall::NewStub(channel)) {

    // Data we are sending to the server.
    InitializeRequest request;
    request.add_framevolumedimensionalsizes(displayWidth);
    request.add_framevolumedimensionalsizes(displayHeight);
    request.set_displaywidth(displayWidth);
    request.set_displayheight(displayHeight);
    request.set_numcoefficientplanes(numCoefficientPlanes);
    request.set_inputvectorsize(inputVectorSize);

    // Container for the data we expect from the server.
    StatusReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->Initialize(&context, request, &reply);

    // Act upon its status.
    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }

}

GrpcNddiDisplay::~GrpcNddiDisplay() {}

unsigned int GrpcNddiDisplay::DisplayWidth() {
    DisplayWidthRequest request;
    WidthReply reply;
    ClientContext context;
    Status status = stub_->DisplayWidth(&context, request, &reply);
    if (status.ok()) {
        return reply.width();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return 0;
    }
}

unsigned int GrpcNddiDisplay::DisplayHeight() {
    DisplayHeightRequest request;
    HeightReply reply;
    ClientContext context;
    Status status = stub_->DisplayHeight(&context, request, &reply);
    if (status.ok()) {
        return reply.height();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return 0;
    }
}

unsigned int GrpcNddiDisplay::NumCoefficientPlanes() {
    return 0;
}

void GrpcNddiDisplay::PutPixel(Pixel p, vector<unsigned int> &location) {
}

void GrpcNddiDisplay::CopyPixelStrip(Pixel* p, vector<unsigned int> &start, vector<unsigned int> &end) {
}

void GrpcNddiDisplay::CopyPixels(Pixel* p, vector<unsigned int> &start, vector<unsigned int> &end) {
}

void GrpcNddiDisplay::CopyPixelTiles(vector<Pixel*> &p, vector<vector<unsigned int> > &starts, vector<unsigned int> &size) {
}

void GrpcNddiDisplay::FillPixel(Pixel p, vector<unsigned int> &start, vector<unsigned int> &end) {
}

void GrpcNddiDisplay::CopyFrameVolume(vector<unsigned int> &start, vector<unsigned int> &end, vector<unsigned int> &dest) {
}

void GrpcNddiDisplay::UpdateInputVector(vector<int> &input) {
}

void GrpcNddiDisplay::PutCoefficientMatrix(vector< vector<int> > &coefficientMatrix,
                                           vector<unsigned int> &location) {
}

void GrpcNddiDisplay::FillCoefficientMatrix(vector< vector<int> > &coefficientMatrix,
                                            vector<unsigned int> &start,
                                            vector<unsigned int> &end) {
}

void GrpcNddiDisplay::FillCoefficient(int coefficient,
                                      unsigned int row, unsigned int col,
                                      vector<unsigned int> &start,
                                      vector<unsigned int> &end) {
}

void GrpcNddiDisplay::FillCoefficientTiles(vector<int> &coefficients,
                                           vector<vector<unsigned int> > &positions,
                                           vector<vector<unsigned int> > &starts,
                                           vector<unsigned int> &size) {
}

void GrpcNddiDisplay::FillScaler(Scaler scaler,
                                 vector<unsigned int> &start,
                                 vector<unsigned int> &end) {
}

void GrpcNddiDisplay::FillScalerTiles(vector<uint64_t> &scalers,
                                      vector<vector<unsigned int> > &starts,
                                      vector<unsigned int> &size) {
}

void GrpcNddiDisplay::FillScalerTileStack(vector<uint64_t> &scalers,
                                          vector<unsigned int> &start,
                                          vector<unsigned int> &size) {
}

void GrpcNddiDisplay::SetPixelByteSignMode(SignMode mode) {
}

void GrpcNddiDisplay::SetFullScaler(uint16_t scaler) {
}

uint16_t GrpcNddiDisplay::GetFullScaler() {
    return 0;
}


CostModel* GrpcNddiDisplay::GetCostModel() {
    return 0;
}
