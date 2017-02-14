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
using nddiwall::CopyFrameVolumeRequest;
using nddiwall::CopyPixelStripRequest;
using nddiwall::CopyPixelsRequest;
using nddiwall::CopyPixelTilesRequest;
using nddiwall::PutCoefficientMatrixRequest;
using nddiwall::FillCoefficientMatrixRequest;
using nddiwall::FillCoefficientRequest;
using nddiwall::FillCoefficientTilesRequest;
using nddiwall::FillScalerRequest;
using nddiwall::FillScalerTilesRequest;
using nddiwall::FillScalerTileStackRequest;
using nddiwall::SetPixelByteSignModeRequest;
using nddiwall::GetFullScalerRequest;
using nddiwall::GetFullScalerReply;
using nddiwall::SetFullScalerRequest;
using nddiwall::UpdateInputVectorRequest;
using nddiwall::LatchRequest;

// public

GrpcNddiDisplay::GrpcNddiDisplay() {}

GrpcNddiDisplay::GrpcNddiDisplay(unsigned int frameVolumeDimensionality,
                                 unsigned int* frameVolumeDimensionalSizes,
                                 unsigned int numCoefficientPlanes, unsigned int inputVectorSize)
: GrpcNddiDisplay(frameVolumeDimensionality, frameVolumeDimensionalSizes, 640, 480, numCoefficientPlanes, inputVectorSize) {
}

GrpcNddiDisplay::GrpcNddiDisplay(unsigned int frameVolumeDimensionality,
                                 unsigned int* frameVolumeDimensionalSizes,
                                 unsigned int displayWidth, unsigned int displayHeight,
                                 unsigned int numCoefficientPlanes, unsigned int inputVectorSize)
: frameVolumeDimensionality_(frameVolumeDimensionality),
  inputVectorSize_(inputVectorSize),
  numCoefficientPlanes_(numCoefficientPlanes),
  stub_(NddiWall::NewStub(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()))) {

    // Data we are sending to the server.
    InitializeRequest request;
    for (size_t i = 0; i < frameVolumeDimensionality; i++) {
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

void GrpcNddiDisplay::PutPixel(Pixel p, unsigned int* location) {
    PutPixelRequest request;
    request.set_pixel(p.packed);
    for (size_t i = 0; i < frameVolumeDimensionality_; i++) {
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

void GrpcNddiDisplay::CopyPixelStrip(Pixel* p, unsigned int* start, unsigned int* end) {
    CopyPixelStripRequest request;
    size_t count = 1;
    for (size_t i = 0; i < frameVolumeDimensionality_; i++) {
      request.add_start(start[i]);
      request.add_end(end[i]);
      count *= end[i] - start[i] + 1;
    }
    request.set_pixels((void*)p, sizeof(Pixel) * count);

    StatusReply reply;

    ClientContext context;
    Status status = stub_->CopyPixelStrip(&context, request, &reply);

    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
}

void GrpcNddiDisplay::CopyPixels(Pixel* p, unsigned int* start, unsigned int* end) {
    CopyPixelsRequest request;
    size_t count = 1;
    for (size_t i = 0; i < frameVolumeDimensionality_; i++) {
      request.add_start(start[i]);
      request.add_end(end[i]);
      count *= end[i] - start[i] + 1;
    }
    request.set_pixels((void*)p, sizeof(Pixel) * count);

    StatusReply reply;

    ClientContext context;
    Status status = stub_->CopyPixels(&context, request, &reply);

    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
}

void GrpcNddiDisplay::CopyPixelTiles(Pixel** p, unsigned int* starts, unsigned int* size, size_t count) {
    CopyPixelTilesRequest request;
    for (size_t i = 0; i < count; i++) {
        for (size_t j = 0; j < frameVolumeDimensionality_; j++) {
            request.add_starts(starts[i * frameVolumeDimensionality_ + j]);
        }
    }
    request.add_size(size[0]);
    request.add_size(size[1]);
    size_t tile_size = size[0] * size[1];
    Pixel p_arr[count * tile_size];
    for (size_t i = 0; i < count; i++) {
        memcpy(&p_arr[i * tile_size], p[i], sizeof(Pixel) * tile_size);
    }
    request.set_pixels((void*)p_arr, sizeof(Pixel) * count * tile_size);

    StatusReply reply;

    ClientContext context;
    Status status = stub_->CopyPixelTiles(&context, request, &reply);

    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
}

void GrpcNddiDisplay::FillPixel(Pixel p, unsigned int* start, unsigned int* end) {
    FillPixelRequest request;
    request.set_pixel(p.packed);
    for (size_t i = 0; i < frameVolumeDimensionality_; i++) {
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

void GrpcNddiDisplay::CopyFrameVolume(unsigned int* start, unsigned int* end, unsigned int* dest) {
    CopyFrameVolumeRequest request;
    for (size_t i = 0; i < frameVolumeDimensionality_; i++) {
      request.add_start(start[i]);
      request.add_end(end[i]);
      request.add_dest(dest[i]);
    }

    StatusReply reply;

    ClientContext context;
    Status status = stub_->CopyFrameVolume(&context, request, &reply);

    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
}

void GrpcNddiDisplay::UpdateInputVector(int* input) {
    UpdateInputVectorRequest request;
    for (size_t i = 0; i < inputVectorSize_; i++) {
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

void GrpcNddiDisplay::PutCoefficientMatrix(int* coefficientMatrix, unsigned int* location) {
    PutCoefficientMatrixRequest request;
    for (size_t j = 0; j < frameVolumeDimensionality_; j++) {
        for (size_t i = 0; i < inputVectorSize_; i++) {
            request.add_coefficientmatrix(coefficientMatrix[j * frameVolumeDimensionality_ + i]);
        }
    }
    for (size_t i = 0; i < 3; i++) {
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

void GrpcNddiDisplay::FillCoefficientMatrix(int* coefficientMatrix, unsigned int* start, unsigned int* end) {
    FillCoefficientMatrixRequest request;
    for (size_t j = 0; j < frameVolumeDimensionality_; j++) {
        for (size_t i = 0; i < inputVectorSize_; i++) {
            request.add_coefficientmatrix(coefficientMatrix[j * frameVolumeDimensionality_ + i]);
        }
    }
    for (size_t i = 0; i < 3; i++) {
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

void GrpcNddiDisplay::FillCoefficient(int coefficient, unsigned int row, unsigned int col, unsigned int* start, unsigned int* end) {
    FillCoefficientRequest request;
    request.set_coefficient(coefficient);
    request.set_row(row);
    request.set_col(col);
    for (size_t i = 0; i < 3; i++) {
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

void GrpcNddiDisplay::FillCoefficientTiles(int* coefficients, unsigned int* positions, unsigned int* starts, unsigned int* size, size_t count) {
    FillCoefficientTilesRequest request;
    for (size_t i = 0; i < count; i++) {
        request.add_coefficients(coefficients[i]);
        for (size_t j = 0; j < 2; j++) {
            request.add_positions(positions[i * 2 + j]);
        }
        for (size_t j = 0; j < 3; j++) {
            request.add_starts(starts[i * 3 + j]);
        }
    }
    request.add_size(size[0]);
    request.add_size(size[1]);

    StatusReply reply;

    ClientContext context;
    Status status = stub_->FillCoefficientTiles(&context, request, &reply);

    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
}

void GrpcNddiDisplay::FillScaler(Scaler scaler, unsigned int* start, unsigned int* end) {
    FillScalerRequest request;
    request.set_scaler(scaler.packed);
    for (size_t i = 0; i < 3; i++) {
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

void GrpcNddiDisplay::FillScalerTiles(Scaler* scalers, unsigned int* starts, unsigned int* size, size_t count) {
    FillScalerTilesRequest request;
    for (size_t i = 0; i < count; i++) {
        request.add_scalers(scalers[i].packed);
        for (size_t j = 0; j < 3; j++) {
            request.add_starts(starts[i * 3 + j]);
        }
    }
    request.add_size(size[0]);
    request.add_size(size[1]);

    StatusReply reply;

    ClientContext context;
    Status status = stub_->FillScalerTiles(&context, request, &reply);

    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
}

void GrpcNddiDisplay::FillScalerTileStack(Scaler* scalers, unsigned int* start, unsigned int* size, size_t count) {
    FillScalerTileStackRequest request;
    for (size_t i = 0; i < count; i++) {
      request.add_scalers(scalers[i].packed);
    }
    for (size_t i = 0; i < 3; i++) {
      request.add_start(start[i]);
    }
    for (size_t i = 0; i < 2; i++) {
      request.add_size(size[i]);
    }

    StatusReply reply;

    ClientContext context;
    Status status = stub_->FillScalerTileStack(&context, request, &reply);

    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
}

void GrpcNddiDisplay::SetPixelByteSignMode(SignMode mode) {
    SetPixelByteSignModeRequest request;
    request.set_mode(mode);

    StatusReply reply;

    ClientContext context;
    Status status = stub_->SetPixelByteSignMode(&context, request, &reply);

    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
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

void GrpcNddiDisplay::Latch() {
    LatchRequest request;
    StatusReply reply;
    ClientContext context;
    Status status = stub_->Latch(&context, request, &reply);
    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
}
