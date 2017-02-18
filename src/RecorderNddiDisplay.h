#ifndef RECORDER_NDDI_DISPLAY_H
#define RECORDER_NDDI_DISPLAY_H

#include "nddi/Features.h"
#include "nddi/NDimensionalDisplayInterface.h"

#include <fstream>

#include <cereal/archives/xml.hpp>

using Archive = cereal::XMLOutputArchive;

using namespace std;

namespace nddi {

    /**
     * Implements and NDDI display where each interface is a recorder of NDDI Commands.
     */
    class RecorderNddiDisplay : public NDimensionalDisplayInterface {

    public:
        RecorderNddiDisplay();
        RecorderNddiDisplay(unsigned int frameVolumeDimensionality,
                            unsigned int* frameVolumeDimensionalSizes,
                            unsigned int numCoefficientPlanes, unsigned int inputVectorSize)
        : RecorderNddiDisplay(frameVolumeDimensionality, frameVolumeDimensionalSizes, 640, 480, numCoefficientPlanes, inputVectorSize) {
        }

        RecorderNddiDisplay(unsigned int frameVolumeDimensionality,
                            unsigned int* frameVolumeDimensionalSizes,
                            unsigned int displayWidth, unsigned int displayHeight,
                            unsigned int numCoefficientPlanes, unsigned int inputVectorSize)
        : recording(true),
          frameVolumeDimensionality_(frameVolumeDimensionality),
          inputVectorSize_(inputVectorSize),
          numCoefficientPlanes_(numCoefficientPlanes) {

            std::ofstream os ("recording.xml", std::ofstream::out);
            Archive oarchive(os);
            oarchive(cereal::make_nvp("command", idInit),
                     CEREAL_NVP(displayWidth),
                     CEREAL_NVP(displayHeight),
                     CEREAL_NVP(numCoefficientPlanes),
                     CEREAL_NVP(inputVectorSize));
        }

        ~RecorderNddiDisplay() {
            recording = false;
        }

        unsigned int DisplayWidth() {}
        unsigned int DisplayHeight() {}
        unsigned int NumCoefficientPlanes() {}
        void PutPixel(Pixel p, unsigned int* location) {}
        void CopyPixelStrip(Pixel* p, unsigned int* , unsigned int* ) {
#if 0
            std::ofstream os ("recording.xml", std::ofstream::app);
            Archive oarchive(os);
            oarchive(cereal::make_nvp("command", idCopyPixelStrip),
                    CEREAL_NVP(count));
            oarchive.saveBinaryValue(p, sizeof(Pixel) * count, "p" );
            oarchive.saveBinaryValue(start, sizeof(unsigned int) * frameVolumeDimensionality_, "start" );
            oarchive.saveBinaryValue(end, sizeof(unsigned int) * frameVolumeDimensionality_, "end" );
#endif
        }
        void CopyPixels(Pixel* p, unsigned int* , unsigned int* ) {}
        void CopyPixelTiles(Pixel** p, unsigned int* starts, unsigned int* size, size_t count) {}
        void FillPixel(Pixel p, unsigned int* start, unsigned int* end) {}
        void CopyFrameVolume(unsigned int* start, unsigned int* end, unsigned int* dest) {}
        void UpdateInputVector(int* input) {}
        void PutCoefficientMatrix(int* coefficientMatrix, unsigned int* location) {}
        void FillCoefficientMatrix(int* coefficientMatrix, unsigned int* start, unsigned int* end) {}
        void FillCoefficient(int coefficient, unsigned int row, unsigned int col, unsigned int* start, unsigned int* end) {}
        void FillCoefficientTiles(int* coefficients, unsigned int* positions, unsigned int* starts, unsigned int* size, size_t count) {}
        void FillScaler(Scaler scaler, unsigned int* start, unsigned int* end) {}
        void FillScalerTiles(Scaler* scalers, unsigned int* starts, unsigned int* size, size_t count) {}
        void FillScalerTileStack(Scaler* scalers, unsigned int* start, unsigned int* size, size_t count) {}
        void SetPixelByteSignMode(SignMode mode) {}
        void SetFullScaler(uint16_t scaler) {}
        uint16_t GetFullScaler() {}
        CostModel* GetCostModel() {}
        void Latch() {}

    private:
        bool recording;
        unsigned int frameVolumeDimensionality_;
        unsigned int inputVectorSize_;
        unsigned int numCoefficientPlanes_;

        enum API_id : unsigned int {
            idInit,
            idDisplayWidth,
            idDisplayHeight,
            idNumCoefficientPlanes,
            idPutPixel,
            idCopyPixelStrip,
            idCopyPixels,
            idCopyPixelTiles,
            idFillPixel,
            idCopyFrameVolume
        };
    };
}

#endif // RECORDER_NDDI_DISPLAY_H
