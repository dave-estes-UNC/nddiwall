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
        void PutCoefficientMatrix(int* coefficientMatrix, unsigned int* location);
        void FillCoefficientMatrix(int* coefficientMatrix, unsigned int* start, unsigned int* end);
        void FillCoefficient(int coefficient, unsigned int row, unsigned int col, unsigned int* start, unsigned int* end);
        void FillCoefficientTiles(int* coefficients, unsigned int* positions, unsigned int* starts, unsigned int* size, size_t count);
        void FillScaler(Scaler scaler, unsigned int* start, unsigned int* end);
        void FillScalerTiles(Scaler* scalers, unsigned int* starts, unsigned int* size, size_t count);
        void FillScalerTileStack(Scaler* scalers, unsigned int* start, unsigned int* size, size_t count);
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
