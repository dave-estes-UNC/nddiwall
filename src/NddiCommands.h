#ifndef NDDI_COMMANDS_H
#define NDDI_COMMANDS_H

#include "Configuration.h"

#include "GrpcNddiDisplay.h"
#include "nddi/Features.h"
#include "nddi/NDimensionalDisplayInterface.h"

#include <cereal/archives/xml.hpp>
#include <cereal/types/vector.hpp>

#include <iostream>

#define NDDI_COMMAND_LIST(m) \
  /*  1 */  m(Init) \
  /*  2 */  m(DisplayWidth) \
  /*  3 */  m(DisplayHeight) \
  /*  4 */  m(NumCoefficientPlanes) \
  /*  5 */  m(PutPixel) \
  /*  6 */  m(CopyPixelStrip) \
  /*  7 */  m(CopyPixels) \
  /*  8 */  m(CopyPixelTiles) \
  /*  9 */  m(FillPixel) \
  /* 10 */  m(CopyFrameVolume) \
  /* 11 */  m(UpdateInputVector) \
  /* 12 */  m(PutCoefficientMatrix) \
  /* 13 */  m(FillCoefficientMatrix) \
  /* 14 */  m(FillCoefficient) \
  /* 15 */  m(FillCoefficientTiles) \
  /* 16 */  m(FillScaler) \
  /* 17 */  m(FillScalerTiles) \
  /* 18 */  m(FillScalerTileStack) \
  /* 19 */  m(SetPixelByteSignMode) \
  /* 20 */  m(SetFullScaler) \
  /* 21 */  m(GetFullScaler) \
  /* 22 */  m(Latch) \
  /* 23 */  m(Shutdown)

namespace nddi {

    enum CommandID : unsigned int {
        idEOT,
        #define GENERATE_ENUM(m) id ## m ,
        NDDI_COMMAND_LIST(GENERATE_ENUM)
    };

    static const string CommandNames[] = {
        "",
        #define GENERATE_NAMES(m) #m ,
        NDDI_COMMAND_LIST(GENERATE_NAMES)

    };

    class NddiCommandMessage {
    public:
        NddiCommandMessage(CommandID id)
        : id(id) {
        }

        void play(GrpcNddiDisplay* &display) {
            std::cout << "Can\'t do anything!" << std::endl;
        }

        template <class Archive>
        void serialize(Archive& ar) {}

        CommandID id;
    };

    class InitCommandMessage : public NddiCommandMessage {
    public:
        InitCommandMessage() : NddiCommandMessage(idInit) {}

        InitCommandMessage(vector<unsigned int> frameVolumeDimensionalSizes,
                           unsigned int displayWidth,
                           unsigned int displayHeight,
                           unsigned int numCoefficientPlanes,
                           unsigned int inputVectorSize,
                           bool fixed8x8Macroblocks,
                           bool useSingleCoeffcientPlane)
        : NddiCommandMessage(idInit),
          frameVolumeDimensionalSizes(frameVolumeDimensionalSizes),
          displayWidth(displayWidth),
          displayHeight(displayHeight),
          numCoefficientPlanes(numCoefficientPlanes),
          inputVectorSize(inputVectorSize),
          fixed8x8Macroblocks(fixed8x8Macroblocks),
          useSingleCoeffcientPlane(useSingleCoeffcientPlane) {}

        void play(GrpcNddiDisplay* &display) {
            if (!globalConfiguration.isSlave) {
                display = new GrpcNddiDisplay(frameVolumeDimensionalSizes,
                                              displayWidth, displayHeight,
                                              numCoefficientPlanes,
                                              inputVectorSize,
                                              fixed8x8Macroblocks,
                                              useSingleCoeffcientPlane);
            } else {
                display = new GrpcNddiDisplay(globalConfiguration.sub_x, globalConfiguration.sub_y);
            }
        }

        template <class Archive>
        void serialize(Archive& ar) {
          ar(CEREAL_NVP(frameVolumeDimensionalSizes));
          ar(CEREAL_NVP(displayWidth), CEREAL_NVP(displayHeight),
             CEREAL_NVP(numCoefficientPlanes), CEREAL_NVP(inputVectorSize),
             CEREAL_NVP(fixed8x8Macroblocks), CEREAL_NVP(useSingleCoeffcientPlane));
        }

    private:
        unsigned int  frameVolumeDimensionality;
        vector<unsigned int> frameVolumeDimensionalSizes;
        unsigned int  displayWidth;
        unsigned int  displayHeight;
        unsigned int  numCoefficientPlanes;
        unsigned int  inputVectorSize;
        bool fixed8x8Macroblocks;
        bool useSingleCoeffcientPlane;
    };

    class DisplayWidthCommandMessage : public NddiCommandMessage {
    public:
        DisplayWidthCommandMessage() : NddiCommandMessage(idDisplayWidth) {}

        void play(GrpcNddiDisplay* display) {
            display->DisplayWidth();
        }
    };

    class DisplayHeightCommandMessage : public NddiCommandMessage {
    public:
        DisplayHeightCommandMessage() : NddiCommandMessage(idDisplayHeight) {}

        void play(GrpcNddiDisplay* display) {
            display->DisplayHeight();
        }
    };

    class NumCoefficientPlanesCommandMessage : public NddiCommandMessage {
    public:
        NumCoefficientPlanesCommandMessage() : NddiCommandMessage(idNumCoefficientPlanes) {}

        void play(GrpcNddiDisplay* display) {
            display->NumCoefficientPlanes();
        }
    };

    class PutPixelCommandMessage : public NddiCommandMessage {
    public:
        PutPixelCommandMessage() : NddiCommandMessage(idPutPixel) {}

        PutPixelCommandMessage(Pixel p, vector<unsigned int> &location)
        : NddiCommandMessage(idPutPixel),
          p(p),
          location(location) {
        }

        void play(GrpcNddiDisplay* display) {
            display->PutPixel(p, location);
        }

        template <class Archive>
        void serialize(Archive& ar) {
          ar(CEREAL_NVP(p), CEREAL_NVP(location));
        }

    private:
        Pixel                p;
        vector<unsigned int> location;
    };

    class CopyPixelStripCommandMessage : public NddiCommandMessage {
    public:
        CopyPixelStripCommandMessage() : NddiCommandMessage(idCopyPixelStrip) {}

        CopyPixelStripCommandMessage(Pixel* p, vector<unsigned int> &start, vector<unsigned int> &end)
        : NddiCommandMessage(idCopyPixelStrip),
          start(start),
          end(end) {
            int dimensionToCopyAlong;
            bool dimensionFound = false;

            // Find the dimension to copy along
            for (int i = 0; !dimensionFound && (i < start.size()); i++) {
                if (start[i] != end[i]) {
                    dimensionToCopyAlong = i;
                    dimensionFound = true;
                }
            }
            size_t pixelsToCopy = end[dimensionToCopyAlong] - start[dimensionToCopyAlong] + 1;

            this->p.resize(pixelsToCopy);
            memcpy(this->p.data(), p, sizeof(Pixel) * pixelsToCopy);
        }

        void play(GrpcNddiDisplay* display) {
            display->CopyPixelStrip(p.data(), start, end);
        }

        template <class Archive>
        void serialize(Archive& ar) {
          ar(CEREAL_NVP(p), CEREAL_NVP(start), CEREAL_NVP(end));
        }

    private:
        vector<Pixel>        p;
        vector<unsigned int> start;
        vector<unsigned int> end;
    };

    class CopyPixelsCommandMessage : public NddiCommandMessage {
    public:
        CopyPixelsCommandMessage() : NddiCommandMessage(idCopyPixels) {}

        CopyPixelsCommandMessage(Pixel* p, vector<unsigned int> &start, vector<unsigned int> &end)
        : NddiCommandMessage(idCopyPixels),
          start(start),
          end(end) {
            size_t pixelsToCopy = 1;
            for (int i = 0; i < start.size(); i++) {
                pixelsToCopy *= end[i] - start[i] + 1;
            }

            this->p.resize(pixelsToCopy);
            memcpy(this->p.data(), p, sizeof(Pixel) * pixelsToCopy);
        }

        void play(GrpcNddiDisplay* display) {
            display->CopyPixels(p.data(), start, end);
        }

        template <class Archive>
        void serialize(Archive& ar) {
          ar(CEREAL_NVP(p), CEREAL_NVP(start), CEREAL_NVP(end));
        }

    private:
        vector<Pixel>        p;
        vector<unsigned int> start;
        vector<unsigned int> end;
    };

    class CopyPixelTilesCommandMessage : public NddiCommandMessage {
    public:
        CopyPixelTilesCommandMessage() : NddiCommandMessage(idCopyPixelTiles) {}

        CopyPixelTilesCommandMessage(vector<Pixel*> &p, vector<vector<unsigned int> > &starts, vector<unsigned int> &size)
        : NddiCommandMessage(idCopyPixelTiles),
          starts(starts),
          size(size) {

            this->p.resize(p.size());

            size_t pixelsPerTile = 1;
            for (int i = 0; i < size.size(); i++) {
                pixelsPerTile *= size[i];
            }
            for (int i = 0; i < p.size(); i++) {
                this->p[i].resize(pixelsPerTile);
                memcpy((void*)this->p[i].data(), (void*)p[i], sizeof(Pixel) * pixelsPerTile);
            }
        }

        void play(GrpcNddiDisplay* display) {
            vector<Pixel*> tmp;

            tmp.resize(p.size());
            for (auto i = 0; i < p.size(); i++) {
                tmp[i] = p[i].data();
            }
            display->CopyPixelTiles(tmp, starts, size);
        }

        template <class Archive>
        void serialize(Archive& ar) {
            ar(CEREAL_NVP(p), CEREAL_NVP(starts), CEREAL_NVP(size));
        }

    private:
        vector<vector<Pixel> > p;
        vector<vector<unsigned int> > starts;
        vector<unsigned int> size;
    };

    class FillPixelCommandMessage : public NddiCommandMessage {
    public:
        FillPixelCommandMessage() : NddiCommandMessage(idFillPixel) {}

        FillPixelCommandMessage(Pixel p, vector<unsigned int> &start, vector<unsigned int> &end)
        : NddiCommandMessage(idFillPixel),
          p(p),
          start(start),
          end(end) {
        }

        void play(GrpcNddiDisplay* display) {
            display->FillPixel(p, start, end);
        }

        template <class Archive>
        void serialize(Archive& ar) {
          ar(CEREAL_NVP(p), CEREAL_NVP(start), CEREAL_NVP(end));
        }

    private:
        Pixel                p;
        vector<unsigned int> start;
        vector<unsigned int> end;
    };

    class CopyFrameVolumeCommandMessage : public NddiCommandMessage {
    public:
        CopyFrameVolumeCommandMessage() : NddiCommandMessage(idCopyFrameVolume) {}
        CopyFrameVolumeCommandMessage(vector<unsigned int> &start, vector<unsigned int> &end, vector<unsigned int> &dest)
        : NddiCommandMessage(idCopyFrameVolume),
          start(start),
          end(end),
          dest(dest) {}

        void play(GrpcNddiDisplay* display) {
            display->CopyFrameVolume(start, end, dest);
        }

        template <class Archive>
        void serialize(Archive& ar) {
            ar(CEREAL_NVP(start), CEREAL_NVP(end), CEREAL_NVP(dest));
        }

    private:
        vector<unsigned int> start;
        vector<unsigned int> end;
        vector<unsigned int> dest;
    };

    class UpdateInputVectorCommandMessage : public NddiCommandMessage {
    public:
        UpdateInputVectorCommandMessage() : NddiCommandMessage(idUpdateInputVector) {}

        UpdateInputVectorCommandMessage(vector<int> &input)
        : NddiCommandMessage(idUpdateInputVector),
          input(input) {}

        void play(GrpcNddiDisplay* display) {
            display->UpdateInputVector(input);
        }

        template <class Archive>
        void serialize(Archive& ar) {
            ar(CEREAL_NVP(input));
        }

    private:
        vector<int> input;
    };

    class PutCoefficientMatrixCommandMessage : public NddiCommandMessage {
    public:
        PutCoefficientMatrixCommandMessage() : NddiCommandMessage(idPutCoefficientMatrix) {}

        PutCoefficientMatrixCommandMessage(vector< vector<int> > &coefficientMatrix, vector<unsigned int> &location)
        : NddiCommandMessage(idPutCoefficientMatrix),
          coefficientMatrix(coefficientMatrix),
          location(location) {}

        void play(GrpcNddiDisplay* display) {
            display->PutCoefficientMatrix(coefficientMatrix, location);
        }

        template <class Archive>
        void serialize(Archive& ar) {
            ar(CEREAL_NVP(coefficientMatrix), CEREAL_NVP(location));
        }

    private:
        vector< vector<int> > coefficientMatrix;
        vector<unsigned int> location;
    };

    class FillCoefficientMatrixCommandMessage : public NddiCommandMessage {
    public:
        FillCoefficientMatrixCommandMessage() : NddiCommandMessage(idFillCoefficientMatrix) {}

        FillCoefficientMatrixCommandMessage(vector< vector<int> > &coefficientMatrix, vector<unsigned int> &start, vector<unsigned int> &end)
        : NddiCommandMessage(idFillCoefficientMatrix),
          coefficientMatrix(coefficientMatrix),
          start(start),
          end(end) {}

        void play(GrpcNddiDisplay* display) {
            display->FillCoefficientMatrix(coefficientMatrix, start, end);
        }

        template <class Archive>
        void serialize(Archive& ar) {
            ar(CEREAL_NVP(coefficientMatrix), CEREAL_NVP(start), CEREAL_NVP(end));
        }

    private:
        vector< vector<int> > coefficientMatrix;
        vector<unsigned int> start;
        vector<unsigned int> end;
    };

    class FillCoefficientCommandMessage : public NddiCommandMessage {
    public:
        FillCoefficientCommandMessage() : NddiCommandMessage(idFillCoefficient) {}

        FillCoefficientCommandMessage(int coefficient, unsigned int row, unsigned int col, vector<unsigned int> &start, vector<unsigned int> &end)
        : NddiCommandMessage(idFillCoefficient),
          coefficient(coefficient),
          row(row),
          col(col),
          start(start),
          end(end) {}

        void play(GrpcNddiDisplay* display) {
            display->FillCoefficient(coefficient, row, col, start, end);
        }

        template <class Archive>
        void serialize(Archive& ar) {
            ar(CEREAL_NVP(coefficient), CEREAL_NVP(row), CEREAL_NVP(col), CEREAL_NVP(start), CEREAL_NVP(end));
        }

    private:
        int coefficient;
        unsigned int row, col;
        vector<unsigned int> start;
        vector<unsigned int> end;
    };

    class FillCoefficientTilesCommandMessage : public NddiCommandMessage {
    public:
        FillCoefficientTilesCommandMessage() : NddiCommandMessage(idFillCoefficientTiles) {}

        FillCoefficientTilesCommandMessage(vector<int> &coefficients, vector<vector<unsigned int> > &positions, vector<vector<unsigned int> > &starts, vector<unsigned int> &size)
        : NddiCommandMessage(idFillCoefficientTiles),
          coefficients(coefficients),
          positions(positions),
          starts(starts),
          size(size) {}

        void play(GrpcNddiDisplay* display) {
            display->FillCoefficientTiles(coefficients, positions, starts, size);
        }

        template <class Archive>
        void serialize(Archive& ar) {
            ar(CEREAL_NVP(coefficients), CEREAL_NVP(positions), CEREAL_NVP(starts), CEREAL_NVP(size));
        }

    private:
        vector<int> coefficients;
        vector<vector<unsigned int> > positions;
        vector<vector<unsigned int> > starts;
        vector<unsigned int> size;
    };

    class FillScalerCommandMessage : public NddiCommandMessage {
    public:
        FillScalerCommandMessage() : NddiCommandMessage(idFillScaler) {}

        FillScalerCommandMessage(Scaler scaler, vector<unsigned int> &start, vector<unsigned int> &end)
        : NddiCommandMessage(idFillScaler),
          scaler(scaler),
          start(start),
          end(end) {}

        void play(GrpcNddiDisplay* display) {
            display->FillScaler(scaler, start, end);
        }

        template <class Archive>
        void serialize(Archive& ar) {
            ar(CEREAL_NVP(scaler), CEREAL_NVP(start), CEREAL_NVP(end));
        }

    private:
        Scaler scaler;
        vector<unsigned int> start;
        vector<unsigned int> end;
    };

    class FillScalerTilesCommandMessage : public NddiCommandMessage {
    public:
        FillScalerTilesCommandMessage() : NddiCommandMessage(idFillScalerTiles) {}

        FillScalerTilesCommandMessage(vector<uint64_t> &scalers, vector<vector<unsigned int> > &starts, vector<unsigned int> &size)
        : NddiCommandMessage(idFillScalerTiles),
          scalers(scalers),
          starts(starts),
          size(size) {}

        void play(GrpcNddiDisplay* display) {
            display->FillScalerTiles(scalers, starts, size);
        }

        template <class Archive>
        void serialize(Archive& ar) {
            ar(CEREAL_NVP(scalers), CEREAL_NVP(starts), CEREAL_NVP(size));
        }

    private:
        vector<uint64_t> scalers;
        vector<vector<unsigned int> > starts;
        vector<unsigned int> size;
    };

    class FillScalerTileStackCommandMessage : public NddiCommandMessage {
    public:
        FillScalerTileStackCommandMessage() : NddiCommandMessage(idFillScalerTileStack) {}

        FillScalerTileStackCommandMessage(vector<uint64_t> &scalers, vector<unsigned int> &start, vector<unsigned int> &size)
        : NddiCommandMessage(idFillScalerTileStack),
          scalers(scalers),
          start(start),
          size(size) {}

        void play(GrpcNddiDisplay* display) {
            display->FillScalerTileStack(scalers, start, size);
        }

        template <class Archive>
        void serialize(Archive& ar) {
            ar(CEREAL_NVP(scalers), CEREAL_NVP(start), CEREAL_NVP(size));
        }

    private:
        vector<uint64_t> scalers;
        vector<unsigned int> start;
        vector<unsigned int> size;
    };

    class SetPixelByteSignModeCommandMessage : public NddiCommandMessage {
    public:
        SetPixelByteSignModeCommandMessage() : NddiCommandMessage(idSetPixelByteSignMode) {}

        SetPixelByteSignModeCommandMessage(SignMode mode)
        : NddiCommandMessage(idSetPixelByteSignMode),
          mode(mode) {}

        void play(GrpcNddiDisplay* display) {
            display->SetPixelByteSignMode(mode);
        }

        template <class Archive>
        void serialize(Archive& ar) {
            ar(CEREAL_NVP(mode));
        }

    private:
        SignMode mode;
    };

    class SetFullScalerCommandMessage : public NddiCommandMessage {
    public:
        SetFullScalerCommandMessage() : NddiCommandMessage(idSetFullScaler) {}

        SetFullScalerCommandMessage(uint16_t scaler)
        : NddiCommandMessage(idSetFullScaler),
          scaler(scaler) {}

        void play(GrpcNddiDisplay* display) {
            display->SetFullScaler(scaler);
        }

        template <class Archive>
        void serialize(Archive& ar) {
            ar(CEREAL_NVP(scaler));
        }

    private:
        uint16_t scaler;
    };

    class GetFullScalerCommandMessage : public NddiCommandMessage {
    public:
        GetFullScalerCommandMessage() : NddiCommandMessage(idGetFullScaler) {}

        void play(GrpcNddiDisplay* display) {
            display->GetFullScaler();
        }
    };

    class LatchCommandMessage : public NddiCommandMessage {
    public:
        LatchCommandMessage() : NddiCommandMessage(idLatch) {}

        void play(GrpcNddiDisplay* display) {
            display->Latch();
        }
    };

    class ShutdownCommandMessage : public NddiCommandMessage {
    public:
        ShutdownCommandMessage() : NddiCommandMessage(idShutdown) {}

        void play(GrpcNddiDisplay* display) {
            display->Shutdown();
        }
    };

}

#endif // NDDI_COMMANDS_H
