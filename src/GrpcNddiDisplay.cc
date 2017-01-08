#include "GrpcNddiDisplay.h"

using namespace nddi;

using nddiwall::InitializeRequest;
using nddiwall::StatusReply;
using nddiwall::DisplayWidthRequest;
using nddiwall::DisplayHeightRequest;
using nddiwall::DisplayWidthReply;
using nddiwall::DisplayHeightReply;
using nddiwall::NumCoefficientPlanesRequest;
using nddiwall::NumCoefficientPlanesReply;
using nddiwall::PutPixelRequest;
using nddiwall::FillPixelRequest;
using nddiwall::FillCoefficientMatrixRequest;
using nddiwall::FillScalerRequest;
using nddiwall::GetFullScalerRequest;
using nddiwall::GetFullScalerReply;
using nddiwall::SetFullScalerRequest;

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
    for (size_t i = 0; i < frameVolumeDimensionalSizes.size(); i++) {
        request.add_framevolumedimensionalsizes(frameVolumeDimensionalSizes[i]);
    }
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
    DisplayWidthReply reply;
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
    DisplayHeightReply reply;
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
    NumCoefficientPlanesRequest request;
    NumCoefficientPlanesReply reply;
    ClientContext context;
    Status status = stub_->NumCoefficientPlanes(&context, request, &reply);
    if (status.ok()) {
      return reply.planes();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return 0;
    }
}

void GrpcNddiDisplay::PutPixel(Pixel p, vector<unsigned int> &location) {
    PutPixelRequest request;
    request.set_pixel(p.packed);
    for (size_t i = 0; i < location.size(); i++) {
      request.add_location(location[i]);
    }

    StatusReply reply;

    ClientContext context;
    Status status = stub_->PutPixel(&context, request, &reply);

    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
}

void GrpcNddiDisplay::CopyPixelStrip(Pixel* p, vector<unsigned int> &start, vector<unsigned int> &end) {
}

void GrpcNddiDisplay::CopyPixels(Pixel* p, vector<unsigned int> &start, vector<unsigned int> &end) {
}

void GrpcNddiDisplay::CopyPixelTiles(vector<Pixel*> &p, vector<vector<unsigned int> > &starts, vector<unsigned int> &size) {
}

void GrpcNddiDisplay::FillPixel(Pixel p, vector<unsigned int> &start, vector<unsigned int> &end) {
    FillPixelRequest request;
    request.set_pixel(p.packed);
    for (size_t i = 0; i < start.size(); i++) {
      request.add_start(start[i]);
    }
    for (size_t i = 0; i < end.size(); i++) {
      request.add_end(end[i]);
    }

    StatusReply reply;

    ClientContext context;
    Status status = stub_->FillPixel(&context, request, &reply);

    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
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
    FillCoefficientMatrixRequest request;
    for (size_t j = 0; j < coefficientMatrix.size(); j++) {
        for (size_t i = 0; i < coefficientMatrix[j].size(); i++) {
            request.add_coefficientmatrix(coefficientMatrix[j][i]);
        }
    }
    for (size_t i = 0; i < start.size(); i++) {
      request.add_start(start[i]);
    }
    for (size_t i = 0; i < end.size(); i++) {
      request.add_end(end[i]);
    }

    StatusReply reply;

    ClientContext context;
    Status status = stub_->FillCoefficientMatrix(&context, request, &reply);

    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
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
    FillScalerRequest request;
    request.set_scaler(scaler.packed);
    for (size_t i = 0; i < start.size(); i++) {
      request.add_start(start[i]);
    }
    for (size_t i = 0; i < end.size(); i++) {
      request.add_end(end[i]);
    }

    StatusReply reply;

    ClientContext context;
    Status status = stub_->FillScaler(&context, request, &reply);

    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
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

void GrpcNddiDisplay::SetFullScaler(uint16_t fullScaler) {
    SetFullScalerRequest request;
    request.set_fullscaler(fullScaler);

    StatusReply reply;

    ClientContext context;
    Status status = stub_->SetFullScaler(&context, request, &reply);

    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
}

uint16_t GrpcNddiDisplay::GetFullScaler() {
    GetFullScalerRequest request;
    GetFullScalerReply reply;
    ClientContext context;
    Status status = stub_->GetFullScaler(&context, request, &reply);
    if (status.ok()) {
      return (uint16_t)reply.fullscaler();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return 0;
    }
    return 0;
}


CostModel* GrpcNddiDisplay::GetCostModel() {
    return 0;
}
