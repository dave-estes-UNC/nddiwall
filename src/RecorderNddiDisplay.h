#ifndef RECORDER_NDDI_DISPLAY_H
#define RECORDER_NDDI_DISPLAY_H

#include "nddi/Features.h"
#include "nddi/NDimensionalDisplayInterface.h"

#include <fstream>
#include <pthread.h>
#include <queue>

#include <cereal/archives/xml.hpp>

namespace nddi {

    enum CommandID : unsigned int {
        idNone,
        idInit,
        idDisplayWidth,
        idDisplayHeight,
        idNumCoefficientPlanes,
        idPutPixel,
        idCopyPixelStrip,
        idCopyPixels,
        idCopyPixelTiles,
        idFillPixel,
        idCopyFrameVolume,
        idUpdateInputVector,
        idPutCoefficientMatrix,
        idFillCoefficientMatrix,
        idFillCoefficient,
        idFillCoefficientTiles,
        idFillScaler,
        idFillScalerTiles,
        idFillScalerTileStack,
        idSetPixelByteSignMode,
        idSetFullScaler,
        idGetFullScaler
    };

    class NddiCommandMessage {
    public:
        NddiCommandMessage(CommandID id)
        : id(id) {
        }

        template <class Archive>
        void serialize(Archive& ar) {
          ar(CEREAL_NVP(id));
        }

        CommandID id;
    };

    class InitCommandMessage : public NddiCommandMessage {
    public:
        InitCommandMessage(unsigned int frameVolumeDimensionality,
                           unsigned int* frameVolumeDimensionalSizes,
                           unsigned int displayWidth,
                           unsigned int displayHeight,
                           unsigned int numCoefficientPlanes,
                           unsigned int inputVectorSize)
        : frameVolumeDimensionality(frameVolumeDimensionality),
          displayWidth(displayWidth),
          displayHeight(displayHeight),
          numCoefficientPlanes(numCoefficientPlanes),
          inputVectorSize(inputVectorSize),
          NddiCommandMessage(idInit) {
            this->frameVolumeDimensionalSizes = (unsigned int*)malloc(sizeof(unsigned int) * frameVolumeDimensionality);
            memcpy(this->frameVolumeDimensionalSizes, frameVolumeDimensionalSizes, sizeof(unsigned int) * frameVolumeDimensionality);
        }

        ~InitCommandMessage() {
            if (frameVolumeDimensionalSizes) { free((void*)frameVolumeDimensionalSizes); }
        }

        template <class Archive>
        void serialize(Archive& ar) {
          ar(CEREAL_NVP(id), CEREAL_NVP(frameVolumeDimensionality));
          ar.saveBinaryValue(frameVolumeDimensionalSizes, sizeof(unsigned int) * frameVolumeDimensionality, "frameVolumeDimensionalSizes" );
          ar(CEREAL_NVP(displayWidth), CEREAL_NVP(displayHeight),
             CEREAL_NVP(numCoefficientPlanes), CEREAL_NVP(inputVectorSize));
        }

    private:
        unsigned int  frameVolumeDimensionality;
        unsigned int* frameVolumeDimensionalSizes;
        unsigned int  displayWidth;
        unsigned int  displayHeight;
        unsigned int  numCoefficientPlanes;
        unsigned int  inputVectorSize;
    };

    class DisplayWidthCommandMessage : public NddiCommandMessage {
    public:
        DisplayWidthCommandMessage() : NddiCommandMessage(idDisplayWidth) {}
    };

    class DisplayHeightCommandMessage : public NddiCommandMessage {
    public:
        DisplayHeightCommandMessage() : NddiCommandMessage(idDisplayHeight) {}
    };

    class NumCoefficientPlanesCommandMessage : public NddiCommandMessage {
    public:
        NumCoefficientPlanesCommandMessage() : NddiCommandMessage(idNumCoefficientPlanes) {}
    };

    class PutPixelCommandMessage : public NddiCommandMessage {
    public:
        PutPixelCommandMessage(Pixel p, unsigned int* location,
                               unsigned int frameVolumeDimensionality)
        : NddiCommandMessage(idPutPixel),
          p(p),
          frameVolumeDimensionality(frameVolumeDimensionality) {
            this->location = (unsigned int*)malloc(sizeof(unsigned int) * frameVolumeDimensionality);
            memcpy(this->location, location, sizeof(unsigned int) * frameVolumeDimensionality);
        }

        ~PutPixelCommandMessage() {
            if (location) { free((void*)location); }
        }

        template <class Archive>
        void serialize(Archive& ar) {
          ar(CEREAL_NVP(id));
          ar.saveBinaryValue(&p, sizeof(Pixel), "p");
          ar.saveBinaryValue(location, sizeof(unsigned int) * frameVolumeDimensionality, "location");
        }

    private:
        Pixel         p;
        unsigned int* location;
        unsigned int  frameVolumeDimensionality;
    };

    class CopyPixelStripCommandMessage : public NddiCommandMessage {
    public:
        CopyPixelStripCommandMessage(Pixel* p, unsigned int count,
                                     unsigned int* start, unsigned int* end,
                                     unsigned int frameVolumeDimensionality)
        : NddiCommandMessage(idCopyPixelStrip),
          count(count),
          frameVolumeDimensionality(frameVolumeDimensionality) {
            this->p = (Pixel*)malloc(sizeof(Pixel) * count);
            memcpy(this->p, p, sizeof(Pixel) * count);
            this->start = (unsigned int*)malloc(sizeof(unsigned int) * frameVolumeDimensionality);
            memcpy(this->start, start, sizeof(unsigned int) * frameVolumeDimensionality);
            this->end = (unsigned int*)malloc(sizeof(unsigned int) * frameVolumeDimensionality);
            memcpy(this->end, end, sizeof(unsigned int) * frameVolumeDimensionality);
        }

        ~CopyPixelStripCommandMessage() {
            if (p) { free((void*)p); }
            if (start) { free((void*)start); }
            if (end) { free((void*)end); }
        }

        template <class Archive>
        void serialize(Archive& ar) {
          ar(CEREAL_NVP(id));
          ar.saveBinaryValue(p, sizeof(Pixel) * count, "p");
          ar.saveBinaryValue(start, sizeof(unsigned int) * frameVolumeDimensionality, "start");
          ar.saveBinaryValue(end, sizeof(unsigned int) * frameVolumeDimensionality, "end");
        }

    private:
        Pixel*        p;
        unsigned int  count;
        unsigned int* start;
        unsigned int* end;
        unsigned int  frameVolumeDimensionality;
    };

    class CopyPixelsCommandMessage : public NddiCommandMessage {};
    class CopyPixelTilesCommandMessage : public NddiCommandMessage {};
    class FillPixelCommandMessage : public NddiCommandMessage {};
    class CopyFrameVolumeCommandMessage : public NddiCommandMessage {};
    class UpdateInputVectorCommandMessage : public NddiCommandMessage {};
    class PutCoefficientMatrixCommandMessage : public NddiCommandMessage {};
    class FillCoefficientMatrixCommandMessage : public NddiCommandMessage {};
    class FillCoefficientCommandMessage : public NddiCommandMessage {};
    class FillCoefficientTilesCommandMessage : public NddiCommandMessage {};
    class FillScalerCommandMessage : public NddiCommandMessage {};
    class FillScalerTilesCommandMessage : public NddiCommandMessage {};
    class FillScalerTileStackCommandMessage : public NddiCommandMessage {};
    class SetPixelByteSignModeCommandMessage : public NddiCommandMessage {};
    class SetFullScalerCommandMessage : public NddiCommandMessage {};
    class GetFullScalerCommandMessage : public NddiCommandMessage {
    public:
        GetFullScalerCommandMessage() : NddiCommandMessage(idGetFullScaler) {}
    };

    class Recorder {
    public:
        Recorder() {
            finished = false;
            typedef void* (*rptr)(void*);
            if (pthread_create( &streamThread, NULL, pthreadFriendlyRun, this)) {
                std::cout << "Error: Failed to start thread." << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        ~Recorder() {
            finished = true;
            pthread_join(streamThread, NULL);
        }

        void run() {
            std::ofstream os("recording.xml", std::ofstream::out);
            cereal::XMLOutputArchive oarchive(os);
            while (!finished || !streamQueue.empty()) {
                if (!streamQueue.empty()) {
                    // TODO(CDE): Protect access to streamQueue.
                    NddiCommandMessage* msg = streamQueue.front();
                    streamQueue.pop();
                    if (msg) {
                        switch (msg->id) {
                        case idInit:
                            oarchive(cereal::make_nvp("command", *(InitCommandMessage*)msg));
                            break;
                        case idDisplayWidth:
                            oarchive(cereal::make_nvp("command", *(DisplayWidthCommandMessage*)msg));
                            break;
                        case idDisplayHeight:
                            oarchive(cereal::make_nvp("command", *(DisplayHeightCommandMessage*)msg));
                            break;
                        case idNumCoefficientPlanes:
                            oarchive(cereal::make_nvp("command", *(NumCoefficientPlanesCommandMessage*)msg));
                            break;
                        case idPutPixel:
                            oarchive(cereal::make_nvp("command", *(PutPixelCommandMessage*)msg));
                            break;
                        case idCopyPixelStrip:
                            oarchive(cereal::make_nvp("command", *(CopyPixelStripCommandMessage*)msg));
                            break;
                        case idCopyPixels:
                            oarchive(cereal::make_nvp("command", *(CopyPixelsCommandMessage*)msg));
                            break;
                        case idCopyPixelTiles:
                            oarchive(cereal::make_nvp("command", *(CopyPixelTilesCommandMessage*)msg));
                            break;
                        case idFillPixel:
                            oarchive(cereal::make_nvp("command", *(FillPixelCommandMessage*)msg));
                            break;
                        case idCopyFrameVolume:
                            oarchive(cereal::make_nvp("command", *(CopyFrameVolumeCommandMessage*)msg));
                            break;
                        case idUpdateInputVector:
                            oarchive(cereal::make_nvp("command", *(UpdateInputVectorCommandMessage*)msg));
                            break;
                        case idPutCoefficientMatrix:
                            oarchive(cereal::make_nvp("command", *(PutCoefficientMatrixCommandMessage*)msg));
                            break;
                        case idFillCoefficientMatrix:
                            oarchive(cereal::make_nvp("command", *(FillCoefficientMatrixCommandMessage*)msg));
                            break;
                        case idFillCoefficient:
                            oarchive(cereal::make_nvp("command", *(FillCoefficientCommandMessage*)msg));
                            break;
                        case idFillCoefficientTiles:
                            oarchive(cereal::make_nvp("command", *(FillCoefficientTilesCommandMessage*)msg));
                            break;
                        case idFillScaler:
                            oarchive(cereal::make_nvp("command", *(FillScalerCommandMessage*)msg));
                            break;
                        case idFillScalerTiles:
                            oarchive(cereal::make_nvp("command", *(FillScalerTilesCommandMessage*)msg));
                            break;
                        case idFillScalerTileStack:
                            oarchive(cereal::make_nvp("command", *(FillScalerTileStackCommandMessage*)msg));
                            break;
                        case idSetPixelByteSignMode:
                            oarchive(cereal::make_nvp("command", *(SetPixelByteSignModeCommandMessage*)msg));
                            break;
                        case idSetFullScaler:
                            oarchive(cereal::make_nvp("command", *(SetFullScalerCommandMessage*)msg));
                            break;
                        case idGetFullScaler:
                            oarchive(cereal::make_nvp("command", *(GetFullScalerCommandMessage*)msg));
                            break;
                        case idNone:
                        default:
                            break;
                        }
                        delete(msg);
                    }
                }
                usleep(50);
            }
        }

        void record(NddiCommandMessage* msg) {
            // TODO(CDE): Protect access to streamQueue.
            streamQueue.push(msg);
        }

    private:
        bool finished;
        pthread_t streamThread;
        static void * pthreadFriendlyRun(void * This) {((Recorder*)This)->run(); return NULL;}
        std::queue<NddiCommandMessage*> streamQueue;
    };

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
        : frameVolumeDimensionality_(frameVolumeDimensionality),
          inputVectorSize_(inputVectorSize),
          numCoefficientPlanes_(numCoefficientPlanes) {
            NddiCommandMessage* msg = new InitCommandMessage(frameVolumeDimensionality, frameVolumeDimensionalSizes,
                                                             displayWidth, displayHeight,
                                                             numCoefficientPlanes, inputVectorSize);
            recorder.record(msg);
        }

        ~RecorderNddiDisplay() {}

        unsigned int DisplayWidth() {
            NddiCommandMessage* msg = new DisplayWidthCommandMessage();
            recorder.record(msg);
        }

        unsigned int DisplayHeight() {
            NddiCommandMessage* msg = new DisplayHeightCommandMessage();
            recorder.record(msg);
        }

        unsigned int NumCoefficientPlanes() {
            NddiCommandMessage* msg = new NumCoefficientPlanesCommandMessage();
            recorder.record(msg);
        }

        void PutPixel(Pixel p, unsigned int* location) {
            NddiCommandMessage* msg = new PutPixelCommandMessage(p, location, frameVolumeDimensionality_);
            recorder.record(msg);
        }

        void CopyPixelStrip(Pixel* p, unsigned int* start, unsigned int* end) {
            int dimensionToCopyAlong;
            bool dimensionFound = false;

            // Find the dimension to copy along
            for (int i = 0; !dimensionFound && (i < frameVolumeDimensionality_); i++) {
                if (start[i] != end[i]) {
                    dimensionToCopyAlong = i;
                    dimensionFound = true;
                }
            }
            int pixelsToCopy = end[dimensionToCopyAlong] - start[dimensionToCopyAlong] + 1;

            NddiCommandMessage* msg = new CopyPixelStripCommandMessage(p, pixelsToCopy,
                                                                       start, end, frameVolumeDimensionality_);
            recorder.record(msg);
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

        uint16_t GetFullScaler() {
            NddiCommandMessage* msg = new GetFullScalerCommandMessage();
            recorder.record(msg);
        }

        CostModel* GetCostModel() {}
        void Latch() {}

    private:
        unsigned int frameVolumeDimensionality_;
        unsigned int inputVectorSize_;
        unsigned int numCoefficientPlanes_;
        Recorder recorder;
    };
}

#endif // RECORDER_NDDI_DISPLAY_H
