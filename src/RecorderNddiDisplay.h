#ifndef RECORDER_NDDI_DISPLAY_H
#define RECORDER_NDDI_DISPLAY_H

#include "nddi/Features.h"
#include "nddi/NDimensionalDisplayInterface.h"

#include <fstream>
#include <pthread.h>
#include <queue>
#include <vector>

#include <cereal/archives/xml.hpp>
#include <cereal/types/vector.hpp>

#define NDDI_COMMAND_LIST(m) \
  m(Init) \
  m(DisplayWidth) \
  m(DisplayHeight) \
  m(NumCoefficientPlanes) \
  m(PutPixel) \
  m(CopyPixelStrip) \
  m(CopyPixels) \
  m(CopyPixelTiles) \
  m(FillPixel) \
  m(CopyFrameVolume) \
  m(UpdateInputVector) \
  m(PutCoefficientMatrix) \
  m(FillCoefficientMatrix) \
  m(FillCoefficient) \
  m(FillCoefficientTiles) \
  m(FillScaler) \
  m(FillScalerTiles) \
  m(FillScalerTileStack) \
  m(SetPixelByteSignMode) \
  m(SetFullScaler) \
  m(GetFullScaler)

namespace nddi {

    enum CommandID : unsigned int {
        idEOT,
        #define GENERATE_ENUM(m) id ## m ,
        NDDI_COMMAND_LIST(GENERATE_ENUM)
    };

    class NddiCommandMessage {
    public:
        NddiCommandMessage(CommandID id)
        : id(id) {
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
                           unsigned int inputVectorSize)
        : frameVolumeDimensionalSizes(frameVolumeDimensionalSizes),
          displayWidth(displayWidth),
          displayHeight(displayHeight),
          numCoefficientPlanes(numCoefficientPlanes),
          inputVectorSize(inputVectorSize),
          NddiCommandMessage(idInit) {}

        template <class Archive>
        void serialize(Archive& ar) {
          ar(CEREAL_NVP(frameVolumeDimensionalSizes));
          ar(CEREAL_NVP(displayWidth), CEREAL_NVP(displayHeight),
             CEREAL_NVP(numCoefficientPlanes), CEREAL_NVP(inputVectorSize));
        }

    private:
        unsigned int  frameVolumeDimensionality;
        vector<unsigned int> frameVolumeDimensionalSizes;
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
        PutPixelCommandMessage() : NddiCommandMessage(idPutPixel) {}

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
        CopyPixelStripCommandMessage() : NddiCommandMessage(idCopyPixelStrip) {}

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

    class CopyPixelsCommandMessage : public NddiCommandMessage {
    public:
        CopyPixelsCommandMessage() : NddiCommandMessage(idCopyPixels) {}

        CopyPixelsCommandMessage(Pixel* p, unsigned int count,
                                 unsigned int* start, unsigned int* end,
                                 unsigned int frameVolumeDimensionality)
        : NddiCommandMessage(idCopyPixels),
          count(count),
          frameVolumeDimensionality(frameVolumeDimensionality) {
            this->p = (Pixel*)malloc(sizeof(Pixel) * count);
            memcpy(this->p, p, sizeof(Pixel) * count);
            this->start = (unsigned int*)malloc(sizeof(unsigned int) * frameVolumeDimensionality);
            memcpy(this->start, start, sizeof(unsigned int) * frameVolumeDimensionality);
            this->end = (unsigned int*)malloc(sizeof(unsigned int) * frameVolumeDimensionality);
            memcpy(this->end, end, sizeof(unsigned int) * frameVolumeDimensionality);
        }

        ~CopyPixelsCommandMessage() {
            if (p) { free((void*)p); }
            if (start) { free((void*)start); }
            if (end) { free((void*)end); }
        }

        template <class Archive>
        void serialize(Archive& ar) {
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

    class CopyPixelTilesCommandMessage : public NddiCommandMessage {
    public:
        CopyPixelTilesCommandMessage() : NddiCommandMessage(idCopyPixelTiles) {}
    };

    class FillPixelCommandMessage : public NddiCommandMessage {
    public:
        FillPixelCommandMessage() : NddiCommandMessage(idFillPixel) {}

        FillPixelCommandMessage(Pixel p,
                                unsigned int* start, unsigned int* end,
                                unsigned int frameVolumeDimensionality)
        : NddiCommandMessage(idFillPixel),
          p(p),
          frameVolumeDimensionality(frameVolumeDimensionality) {
            this->start = (unsigned int*)malloc(sizeof(unsigned int) * frameVolumeDimensionality);
            memcpy(this->start, start, sizeof(unsigned int) * frameVolumeDimensionality);
            this->end = (unsigned int*)malloc(sizeof(unsigned int) * frameVolumeDimensionality);
            memcpy(this->end, end, sizeof(unsigned int) * frameVolumeDimensionality);
        }

        ~FillPixelCommandMessage() {
            if (start) { free((void*)start); }
            if (end) { free((void*)end); }
        }

        template <class Archive>
        void serialize(Archive& ar) {
          ar.saveBinaryValue(&p, sizeof(Pixel), "p");
          ar.saveBinaryValue(start, sizeof(unsigned int) * frameVolumeDimensionality, "start");
          ar.saveBinaryValue(end, sizeof(unsigned int) * frameVolumeDimensionality, "end");
        }

    private:
        Pixel         p;
        unsigned int* start;
        unsigned int* end;
        unsigned int  frameVolumeDimensionality;
    };

    class CopyFrameVolumeCommandMessage : public NddiCommandMessage {
    public:
        CopyFrameVolumeCommandMessage() : NddiCommandMessage(idCopyFrameVolume) {}
    };

    class UpdateInputVectorCommandMessage : public NddiCommandMessage {
    public:
        UpdateInputVectorCommandMessage() : NddiCommandMessage(idUpdateInputVector) {}
    };

    class PutCoefficientMatrixCommandMessage : public NddiCommandMessage {
    public:
        PutCoefficientMatrixCommandMessage() : NddiCommandMessage(idPutCoefficientMatrix) {}
    };

    class FillCoefficientMatrixCommandMessage : public NddiCommandMessage {
    public:
        FillCoefficientMatrixCommandMessage() : NddiCommandMessage(idFillCoefficientMatrix) {}
    };

    class FillCoefficientCommandMessage : public NddiCommandMessage {
    public:
        FillCoefficientCommandMessage() : NddiCommandMessage(idFillCoefficient) {}
    };

    class FillCoefficientTilesCommandMessage : public NddiCommandMessage {
    public:
        FillCoefficientTilesCommandMessage() : NddiCommandMessage(idFillCoefficientTiles) {}
    };

    class FillScalerCommandMessage : public NddiCommandMessage {
    public:
        FillScalerCommandMessage() : NddiCommandMessage(idFillScaler) {}
    };

    class FillScalerTilesCommandMessage : public NddiCommandMessage {
    public:
        FillScalerTilesCommandMessage() : NddiCommandMessage(idFillScalerTiles) {}
    };

    class FillScalerTileStackCommandMessage : public NddiCommandMessage {
    public:
        FillScalerTileStackCommandMessage() : NddiCommandMessage(idFillScalerTileStack) {}
    };

    class SetPixelByteSignModeCommandMessage : public NddiCommandMessage {
    public:
        SetPixelByteSignModeCommandMessage() : NddiCommandMessage(idSetPixelByteSignMode) {}
    };

    class SetFullScalerCommandMessage : public NddiCommandMessage {
    public:
        SetFullScalerCommandMessage() : NddiCommandMessage(idSetFullScaler) {}
    };

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
                        CommandID id = msg->id;
                        switch (id) {
                        #define GENERATE_REC_CASE(m) \
                        case id ## m : \
                            oarchive(CEREAL_NVP(id), cereal::make_nvp("command", *( m ## CommandMessage*)msg)); \
                            break;
                        NDDI_COMMAND_LIST(GENERATE_REC_CASE)
                        case idEOT:
                        default:
                            break;
                        }
                        delete(msg);
                    }
                }
                usleep(50);
            }
            oarchive(cereal::make_nvp("id", idEOT));
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

    class Player {
    public:
        Player() : finished(true) {}

        Player(char* file) : finished(false), file(file)  {
            typedef void* (*rptr)(void*);
            if (pthread_create( &streamThread, NULL, pthreadFriendlyRun, this)) {
                std::cout << "Error: Failed to start thread." << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        ~Player() {
            pthread_join(streamThread, NULL);
        }

        void run() {
            std::ifstream is(file);
            cereal::XMLInputArchive iarchive(is);

            CommandID id;
            iarchive(id);
            while (id != idEOT) {
                std::cout << id << std::endl;
                NddiCommandMessage* msg = NULL;
                switch (id) {
                #define GENERATE_PLAY_CASE(m) \
                case id ## m : { \
                    msg = new m ## CommandMessage(); \
                    iarchive(*msg); \
                } \
                break;
                NDDI_COMMAND_LIST(GENERATE_PLAY_CASE)
                case idEOT:
                default:
                    break;
                }

                if (msg) {
                    // TODO(CDE): Protect access to streamQueue.
                    streamQueue.push(msg);
                }

                iarchive(id);
            }
            finished = true;
        }

    private:
        bool finished;
        char* file;
        pthread_t streamThread;
        static void * pthreadFriendlyRun(void * This) {((Player*)This)->run(); return NULL;}
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
            recorder = new Recorder();

            vector<unsigned int> fds;
            for (auto i = 0; i < frameVolumeDimensionality; i++) {
                fds.push_back(frameVolumeDimensionalSizes[i]);
            }
            NddiCommandMessage* msg = new InitCommandMessage(fds,
                                                             displayWidth, displayHeight,
                                                             numCoefficientPlanes, inputVectorSize);
            recorder->record(msg);
        }

        RecorderNddiDisplay(char* file) {
            player = new Player(file);
        }

        ~RecorderNddiDisplay() {
            if (recorder) { delete(recorder); }
            if (player) { delete(player); }
        }

        unsigned int DisplayWidth() {
            NddiCommandMessage* msg = new DisplayWidthCommandMessage();
            recorder->record(msg);
        }

        unsigned int DisplayHeight() {
            NddiCommandMessage* msg = new DisplayHeightCommandMessage();
            recorder->record(msg);
        }

        unsigned int NumCoefficientPlanes() {
            NddiCommandMessage* msg = new NumCoefficientPlanesCommandMessage();
            recorder->record(msg);
        }

        void PutPixel(Pixel p, unsigned int* location) {
            NddiCommandMessage* msg = new PutPixelCommandMessage(p, location, frameVolumeDimensionality_);
            recorder->record(msg);
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
            recorder->record(msg);
        }

        void CopyPixels(Pixel* p, unsigned int* start, unsigned int* end) {
            int pixelsToCopy = 1;
            for (int i = 0; i < frameVolumeDimensionality_; i++) {
                pixelsToCopy *= end[i] - start[i] + 1;
            }
            NddiCommandMessage* msg = new CopyPixelsCommandMessage(p, pixelsToCopy,
                                                                   start, end, frameVolumeDimensionality_);
            recorder->record(msg);
        }

        void CopyPixelTiles(Pixel** p, unsigned int* starts, unsigned int* size, size_t count) {}

        void FillPixel(Pixel p, unsigned int* start, unsigned int* end) {
            NddiCommandMessage* msg = new FillPixelCommandMessage(p, start, end, frameVolumeDimensionality_);
            recorder->record(msg);
        }

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
            recorder->record(msg);
        }

        CostModel* GetCostModel() {}
        void Latch() {}

    private:
        unsigned int frameVolumeDimensionality_;
        unsigned int inputVectorSize_;
        unsigned int numCoefficientPlanes_;
        Recorder*    recorder = NULL;
        Player*      player = NULL;
    };
}

#endif // RECORDER_NDDI_DISPLAY_H
