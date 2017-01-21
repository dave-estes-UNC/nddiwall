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
using nddiwall::CopyPixelStripRequest;
using nddiwall::CopyPixelsRequest;
using nddiwall::PutCoefficientMatrixRequest;
using nddiwall::FillCoefficientMatrixRequest;
using nddiwall::FillCoefficientRequest;
using nddiwall::FillScalerRequest;
using nddiwall::GetFullScalerRequest;
using nddiwall::GetFullScalerReply;
using nddiwall::SetFullScalerRequest;
using nddiwall::UpdateInputVectorRequest;

// public

GrpcNddiDisplay::GrpcNddiDisplay() {}

GrpcNddiDisplay::GrpcNddiDisplay(vector<unsigned int> &frameVolumeDimensionalSizes,
                                 unsigned int numCoefficientPlanes, unsigned int inputVectorSize)
: GrpcNddiDisplay(frameVolumeDimensionalSizes, 640, 480, numCoefficientPlanes, inputVectorSize) {
}

GrpcNddiDisplay::GrpcNddiDisplay(vector<unsigned int> &frameVolumeDimensionalSizes,
                                 unsigned int displayWidth, unsigned int displayHeight,
                                 unsigned int numCoefficientPlanes, unsigned int inputVectorSize)
: stub_(NddiWall::NewStub(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()))) {

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
    assert(start.size() == end.size());

    CopyPixelStripRequest request;
    size_t count = 1;
    for (size_t i = 0; i < start.size(); i++) {
      request.add_start(start[i]);
      request.add_end(end[i]);
      count *= end[i] - start[i] + 1;
    }
    for (size_t i = 0; i < count; i++) {
      request.add_pixels(p[i].packed);
    }

    StatusReply reply;

    ClientContext context;
    Status status = stub_->CopyPixelStrip(&context, request, &reply);

    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
}

void GrpcNddiDisplay::CopyPixels(Pixel* p, vector<unsigned int> &start, vector<unsigned int> &end) {
    assert(start.size() == end.size());

    CopyPixelsRequest request;
    size_t count = 1;
    for (size_t i = 0; i < start.size(); i++) {
      request.add_start(start[i]);
      request.add_end(end[i]);
      count *= end[i] - start[i] + 1;
    }
    for (size_t i = 0; i < count; i++) {
      request.add_pixels(p[i].packed);
    }

    StatusReply reply;

    ClientContext context;
    Status status = stub_->CopyPixels(&context, request, &reply);

    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
}

void GrpcNddiDisplay::CopyPixelTiles(vector<Pixel*> &p, vector<vector<unsigned int> > &starts, vector<unsigned int> &size) {
    assert(false && "CopyPixelTiles Not Implemented.");
}

void GrpcNddiDisplay::FillPixel(Pixel p, vector<unsigned int> &start, vector<unsigned int> &end) {
    assert(start.size() == end.size());

    FillPixelRequest request;
    request.set_pixel(p.packed);
    for (size_t i = 0; i < start.size(); i++) {
      request.add_start(start[i]);
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
    assert(false && "CopyFrameVolume Not Implemented.");
}

void GrpcNddiDisplay::UpdateInputVector(vector<int> &input) {
    UpdateInputVectorRequest request;
    for (size_t i = 0; i < input.size(); i++) {
      request.add_input(input[i]);
    }

    StatusReply reply;

    ClientContext context;
    Status status = stub_->UpdateInputVector(&context, request, &reply);

    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
}

void GrpcNddiDisplay::PutCoefficientMatrix(vector< vector<int> > &coefficientMatrix,
                                           vector<unsigned int> &location) {
    PutCoefficientMatrixRequest request;
    for (size_t j = 0; j < coefficientMatrix.size(); j++) {
        for (size_t i = 0; i < coefficientMatrix[j].size(); i++) {
            request.add_coefficientmatrix(coefficientMatrix[j][i]);
        }
    }
    for (size_t i = 0; i < location.size(); i++) {
      request.add_location(location[i]);
    }

    StatusReply reply;

    ClientContext context;
    Status status = stub_->PutCoefficientMatrix(&context, request, &reply);

    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
}

void GrpcNddiDisplay::FillCoefficientMatrix(vector< vector<int> > &coefficientMatrix,
                                            vector<unsigned int> &start,
                                            vector<unsigned int> &end) {
    assert(start.size() == end.size());

    FillCoefficientMatrixRequest request;
    for (size_t j = 0; j < coefficientMatrix.size(); j++) {
        for (size_t i = 0; i < coefficientMatrix[j].size(); i++) {
            request.add_coefficientmatrix(coefficientMatrix[j][i]);
        }
    }
    for (size_t i = 0; i < start.size(); i++) {
      request.add_start(start[i]);
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
    assert(start.size() == end.size());

    FillCoefficientRequest request;
    request.set_coefficient(coefficient);
    request.set_row(row);
    request.set_col(col);
    for (size_t i = 0; i < start.size(); i++) {
      request.add_start(start[i]);
      request.add_end(end[i]);
    }

    StatusReply reply;

    ClientContext context;
    Status status = stub_->FillCoefficient(&context, request, &reply);

    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
}

void GrpcNddiDisplay::FillCoefficientTiles(vector<int> &coefficients,
                                           vector<vector<unsigned int> > &positions,
                                           vector<vector<unsigned int> > &starts,
                                           vector<unsigned int> &size) {
    assert(false && "FillCoefficientTiles Not Implemented.");
}

void GrpcNddiDisplay::FillScaler(Scaler scaler,
                                 vector<unsigned int> &start,
                                 vector<unsigned int> &end) {
    assert(start.size() == end.size());

    FillScalerRequest request;
    request.set_scaler(scaler.packed);
    for (size_t i = 0; i < start.size(); i++) {
      request.add_start(start[i]);
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
    assert(false && "FillScalerTiles Not Implemented.");
}

void GrpcNddiDisplay::FillScalerTileStack(vector<uint64_t> &scalers,
                                          vector<unsigned int> &start,
                                          vector<unsigned int> &size) {
    assert(false && "FillScalerTileStack Not Implemented.");
}

void GrpcNddiDisplay::SetPixelByteSignMode(SignMode mode) {
    assert(false && "SetPixelByteSignMode Not Implemented.");
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
