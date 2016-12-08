#include "GrpcNddiDisplay.h"

using namespace nddi;
using namespace std;

// public

GrpcNddiDisplay::GrpcNddiDisplay() {
}

GrpcNddiDisplay::GrpcNddiDisplay(vector<unsigned int> &frameVolumeDimensionalSizes,
                                 unsigned int numCoefficientPlanes, unsigned int inputVectorSize) {
}

GrpcNddiDisplay::GrpcNddiDisplay(vector<unsigned int> &frameVolumeDimensionalSizes,
                                 unsigned int displayWidth, unsigned int displayHeight,
                                 unsigned int numCoefficientPlanes, unsigned int inputVectorSize) {
}

GrpcNddiDisplay::~GrpcNddiDisplay() {}

unsigned int GrpcNddiDisplay::DisplayWidth() {
    return 0;
}

unsigned int GrpcNddiDisplay::DisplayHeight() {
    return 0;
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
