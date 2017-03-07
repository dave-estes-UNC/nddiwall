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
using nddiwall::ShutdownRequest;

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
    request.set_pixels((void*)p, sizeof(Pixel) * count);

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
    request.set_pixels((void*)p, sizeof(Pixel) * count);

    StatusReply reply;

    ClientContext context;
    Status status = stub_->CopyPixels(&context, request, &reply);

    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
}

void GrpcNddiDisplay::CopyPixelTiles(vector<Pixel*> &p, vector<vector<unsigned int> > &starts, vector<unsigned int> &size) {
    assert(p.size() == starts.size());
    assert(size.size() == 2);

    CopyPixelTilesRequest request;
    for (size_t i = 0; i < starts.size(); i++) {
        for (size_t j = 0; j < starts[i].size(); j++) {
            request.add_starts(starts[i][j]);
        }
    }
    request.add_size(size[0]);
    request.add_size(size[1]);
    size_t tile_count = starts.size();
    size_t tile_size = size[0] * size[1];
    Pixel p_arr[tile_count * tile_size];
    for (size_t i = 0; i < tile_count; i++) {
        memcpy(&p_arr[i * tile_size], p[i], sizeof(Pixel) * tile_size);
    }
    request.set_pixels((void*)p_arr, sizeof(Pixel) * tile_count * tile_size);

    StatusReply reply;

    ClientContext context;
    Status status = stub_->CopyPixelTiles(&context, request, &reply);

    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
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
    assert(start.size() == end.size());
    assert(start.size() == dest.size());

    CopyFrameVolumeRequest request;
    for (size_t i = 0; i < start.size(); i++) {
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
    assert(coefficients.size() == positions.size());
    assert(coefficients.size() == starts.size());
    assert(size.size() == 2);

    FillCoefficientTilesRequest request;
    for (size_t i = 0; i < coefficients.size(); i++) {
        request.add_coefficients(coefficients[i]);
    }
    for (size_t i = 0; i < positions.size(); i++) {
        for (size_t j = 0; j < positions[i].size(); j++) {
            request.add_positions(positions[i][j]);
        }
    }
    for (size_t i = 0; i < starts.size(); i++) {
        for (size_t j = 0; j < starts[i].size(); j++) {
            request.add_starts(starts[i][j]);
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
    assert(scalers.size() == starts.size());
    assert(size.size() == 2);

    FillScalerTilesRequest request;
    for (size_t i = 0; i < scalers.size(); i++) {
        request.add_scalers(scalers[i]);
    }
    for (size_t i = 0; i < starts.size(); i++) {
        for (size_t j = 0; j < starts[i].size(); j++) {
            request.add_starts(starts[i][j]);
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

void GrpcNddiDisplay::FillScalerTileStack(vector<uint64_t> &scalers,
                                          vector<unsigned int> &start,
                                          vector<unsigned int> &size) {
    assert(start.size() == 3);
    assert(size.size() == 2);

    FillScalerTileStackRequest request;
    for (size_t i = 0; i < scalers.size(); i++) {
      request.add_scalers(scalers[i]);
    }
    for (size_t i = 0; i < start.size(); i++) {
      request.add_start(start[i]);
    }
    for (size_t i = 0; i < size.size(); i++) {
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

void GrpcNddiDisplay::Shutdown() {
    ShutdownRequest request;
    StatusReply reply;
    ClientContext context;
    Status status = stub_->Shutdown(&context, request, &reply);
    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
}
