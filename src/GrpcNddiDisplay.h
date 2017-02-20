#ifndef GRPC_NDDI_DISPLAY_H
#define GRPC_NDDI_DISPLAY_H

#include <grpc++/grpc++.h>

#include "nddi/Features.h"
#include "nddi/NDimensionalDisplayInterface.h"

#include "nddiwall.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using nddiwall::NddiWall;

using namespace std;

namespace nddi {

    /**
     * Implements and NDDI display where each interface is a GRPC call to the NDDI Wall Server.
     */
    class GrpcNddiDisplay : public NDimensionalDisplayInterface {

    public:
        GrpcNddiDisplay();
        GrpcNddiDisplay(unsigned int frameVolumeDimensionality,
                        unsigned int* frameVolumeDimensionalSizes,
                        unsigned int numCoefficientPlanes, unsigned int inputVectorSize);
        GrpcNddiDisplay(unsigned int frameVolumeDimensionality,
                        unsigned int* frameVolumeDimensionalSizes,
                        unsigned int displayWidth, unsigned int displayHeight,
                        unsigned int numCoefficientPlanes, unsigned int inputVectorSize);
        ~GrpcNddiDisplay();
        unsigned int DisplayWidth();
        unsigned int DisplayHeight();
        unsigned int NumCoefficientPlanes();
        void PutPixel(Pixel p, unsigned int* location);
        void CopyPixelStrip(Pixel* p, unsigned int* , unsigned int* );
        void CopyPixels(Pixel* p, unsigned int* , unsigned int* );
        void CopyPixelTiles(Pixel** p, unsigned int* starts, unsigned int* size, size_t count);
        void FillPixel(Pixel p, unsigned int* start, unsigned int* end);
        void CopyFrameVolume(unsigned int* start, unsigned int* end, unsigned int* dest);
        void UpdateInputVector(int* input);
        void PutCoefficientMatrix(vector< vector<int> > &coefficientMatrix, vector<unsigned int> &location);
        void FillCoefficientMatrix(vector< vector<int> > &coefficientMatrix, vector<unsigned int> &start, vector<unsigned int> &end);
        void FillCoefficient(int coefficient, unsigned int row, unsigned int col, vector<unsigned int> &start, vector<unsigned int> &end);
        void FillCoefficientTiles(vector<int> &coefficients, vector<vector<unsigned int> > &positions, vector<vector<unsigned int> > &starts, vector<unsigned int> &size);
        void FillScaler(Scaler scaler, vector<unsigned int> &start, vector<unsigned int> &end);
        void FillScalerTiles(vector<uint64_t> &scalers, vector<vector<unsigned int> > &starts, vector<unsigned int> &size);
        void FillScalerTileStack(vector<uint64_t> &scalers, vector<unsigned int> &start, vector<unsigned int> &size);
        void SetPixelByteSignMode(SignMode mode);
        void SetFullScaler(uint16_t scaler);
        uint16_t GetFullScaler();
        CostModel* GetCostModel();
        void Latch();

    private:
        unsigned int frameVolumeDimensionality_;
        unsigned int inputVectorSize_;
        unsigned int numCoefficientPlanes_;
        unique_ptr<NddiWall::Stub> stub_;

    };

}

#endif // GRPC_NDDI_DISPLAY_H
