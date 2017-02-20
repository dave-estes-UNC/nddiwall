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

        PutPixelCommandMessage(Pixel p, vector<unsigned int> &location)
        : NddiCommandMessage(idPutPixel),
          p(p),
          location(location) {
        }

        ~PutPixelCommandMessage() {
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
            pixelsToCopy = end[dimensionToCopyAlong] - start[dimensionToCopyAlong] + 1;

            this->p = (Pixel*)malloc(sizeof(Pixel) * pixelsToCopy);
            memcpy(this->p, p, sizeof(Pixel) * pixelsToCopy);
        }

        ~CopyPixelStripCommandMessage() {
            if (p) { free((void*)p); }
        }

        template <class Archive>
        void serialize(Archive& ar) {
          ar.saveBinaryValue(p, sizeof(Pixel) * pixelsToCopy, "p");
          ar(CEREAL_NVP(start), CEREAL_NVP(end));
        }

    private:
        Pixel*               p;
        unsigned int         pixelsToCopy;
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
            pixelsToCopy = 1;
            for (int i = 0; i < start.size(); i++) {
                pixelsToCopy *= end[i] - start[i] + 1;
            }

            this->p = (Pixel*)malloc(sizeof(Pixel) * pixelsToCopy);
            memcpy(this->p, p, sizeof(Pixel) * pixelsToCopy);
        }

        ~CopyPixelsCommandMessage() {
            if (p) { free((void*)p); }
        }

        template <class Archive>
        void serialize(Archive& ar) {
          ar.saveBinaryValue(p, sizeof(Pixel) * pixelsToCopy, "p");
          ar(CEREAL_NVP(start), CEREAL_NVP(end));
        }

    private:
        Pixel*               p;
        unsigned int         pixelsToCopy;
        vector<unsigned int> start;
        vector<unsigned int> end;
    };

    class CopyPixelTilesCommandMessage : public NddiCommandMessage {
    public:
        CopyPixelTilesCommandMessage() : NddiCommandMessage(idCopyPixelTiles) {}

        CopyPixelTilesCommandMessage(vector<Pixel*> &p, vector<vector<unsigned int> > &starts, vector<unsigned int> &size)
        : NddiCommandMessage(idCopyPixelTiles),
          p(p),
          starts(starts),
          size(size) {
            assert(false && "Not yet tested.");
            pixelsPerTile = 1;
            for (int i = 0; i < size.size(); i++) {
                pixelsPerTile *= size[i];
            }
            for (int i = 0; i < p.size(); i++) {
                Pixel* tmp = (Pixel*)malloc(sizeof(Pixel) * pixelsPerTile);
                memcpy((void*)tmp, (void*)p[i], sizeof(Pixel) * pixelsPerTile);
                p[i] = tmp;
            }
        }

        ~CopyPixelTilesCommandMessage() {
            for (int i = 0; i < p.size(); i++) {
                if (p[i]) { free((void*) p[i]); }
            }
        }

        template <class Archive>
        void serialize(Archive& ar) {
            for (int i = 0; i < p.size(); i++) {
                ar.saveBinaryValue(p[i], sizeof(Pixel) * pixelsPerTile, "p");
            }
            ar(CEREAL_NVP(starts), CEREAL_NVP(size));
        }

    private:
        vector<Pixel*> p;
        unsigned int pixelsPerTile;
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

        template <class Archive>
        void serialize(Archive& ar) {
          ar.saveBinaryValue(&p, sizeof(Pixel), "p");
          ar(CEREAL_NVP(start), CEREAL_NVP(end));
        }

    private:
        Pixel         p;
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
        RecorderNddiDisplay(vector<unsigned int> &frameVolumeDimensionalSizes,
                            unsigned int numCoefficientPlanes, unsigned int inputVectorSize)
        : RecorderNddiDisplay(frameVolumeDimensionalSizes, 640, 480, numCoefficientPlanes, inputVectorSize) {
        }

        RecorderNddiDisplay(vector<unsigned int> &frameVolumeDimensionalSizes,
                unsigned int displayWidth, unsigned int displayHeight,
                unsigned int numCoefficientPlanes, unsigned int inputVectorSize)
        : frameVolumeDimensionalSizes_(frameVolumeDimensionalSizes),
          inputVectorSize_(inputVectorSize),
          numCoefficientPlanes_(numCoefficientPlanes) {
            recorder = new Recorder();
            NddiCommandMessage* msg = new InitCommandMessage(frameVolumeDimensionalSizes,
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

        void PutPixel(Pixel p, vector<unsigned int> &location) {
            NddiCommandMessage* msg = new PutPixelCommandMessage(p, location);
            recorder->record(msg);
        }

        void CopyPixelStrip(Pixel* p, vector<unsigned int> &start, vector<unsigned int> &end) {
            NddiCommandMessage* msg = new CopyPixelStripCommandMessage(p, start, end);
            recorder->record(msg);
        }

        void CopyPixels(Pixel* p, vector<unsigned int> &start, vector<unsigned int> &end) {
            NddiCommandMessage* msg = new CopyPixelsCommandMessage(p, start, end);
            recorder->record(msg);
        }

        void CopyPixelTiles(vector<Pixel*> &p, vector<vector<unsigned int> > &starts, vector<unsigned int> &size) {
            NddiCommandMessage* msg = new CopyPixelTilesCommandMessage(p, starts, size);
            recorder->record(msg);
        }

        void FillPixel(Pixel p, vector<unsigned int> &start, vector<unsigned int> &end) {
            NddiCommandMessage* msg = new FillPixelCommandMessage(p, start, end);
            recorder->record(msg);
        }

        void CopyFrameVolume(vector<unsigned int> &start, vector<unsigned int> &end, vector<unsigned int> &dest) {
            NddiCommandMessage* msg = new CopyFrameVolumeCommandMessage(start, end, dest);
            recorder->record(msg);
        }
        void UpdateInputVector(vector<int> &input) {}
        void PutCoefficientMatrix(vector< vector<int> > &coefficientMatrix, vector<unsigned int> &location) {}
        void FillCoefficientMatrix(vector< vector<int> > &coefficientMatrix, vector<unsigned int> &start, vector<unsigned int> &end) {}
        void FillCoefficient(int coefficient, unsigned int row, unsigned int col, vector<unsigned int> &start, vector<unsigned int> &end) {}
        void FillCoefficientTiles(vector<int> &coefficients, vector<vector<unsigned int> > &positions, vector<vector<unsigned int> > &starts, vector<unsigned int> &size) {}
        void FillScaler(Scaler scaler, vector<unsigned int> &start, vector<unsigned int> &end) {}
        void FillScalerTiles(vector<uint64_t> &scalers, vector<vector<unsigned int> > &starts, vector<unsigned int> &size) {}
        void FillScalerTileStack(vector<uint64_t> &scalers, vector<unsigned int> &start, vector<unsigned int> &size) {}
        void SetPixelByteSignMode(SignMode mode) {}
        void SetFullScaler(uint16_t scaler) {}

        uint16_t GetFullScaler() {
            NddiCommandMessage* msg = new GetFullScalerCommandMessage();
            recorder->record(msg);
        }

        CostModel* GetCostModel() {}
        void Latch() {}

    private:
        vector<unsigned int>  frameVolumeDimensionalSizes_;
        unsigned int          inputVectorSize_;
        unsigned int          numCoefficientPlanes_;
        Recorder*             recorder = NULL;
        Player*               player = NULL;
    };
}

#endif // RECORDER_NDDI_DISPLAY_H
