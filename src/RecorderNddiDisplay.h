#ifndef RECORDER_NDDI_DISPLAY_H
#define RECORDER_NDDI_DISPLAY_H

#include "GrpcNddiDisplay.h"
#include "NddiCommands.h"
#include "nddi/Features.h"
#include "nddi/NDimensionalDisplayInterface.h"

#include <fstream>
#include <pthread.h>
#include <queue>
#include <vector>

#include <cereal/archives/xml.hpp>
#include <cereal/types/vector.hpp>

namespace nddi {

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
                usleep(10);
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

        Player(char* file)
        : finished(false), file(file)  {}

        ~Player() {
            pthread_join(streamThread, NULL);
        }

        void play() {
            typedef void* (*rptr)(void*);
            if (pthread_create( &streamThread, NULL, pthreadFriendlyRun, this)) {
                std::cout << "Error: Failed to start thread." << std::endl;
                exit(EXIT_FAILURE);
            }

            GrpcNddiDisplay* display = NULL;

            while (!finished || !streamQueue.empty()) {
                if (!streamQueue.empty()) {
                    // TODO(CDE): Protect access to streamQueue.
                    NddiCommandMessage* msg = streamQueue.front();
                    streamQueue.pop();
                    if (msg) {
                        CommandID id = msg->id;
                        std::cout << "Popped a " << CommandNames[id] << std::endl;;
                        switch (id) {
                        #define GENERATE_PLAY_CASE(m) \
                        case id ## m : { \
                            ((m ## CommandMessage*)msg)->play(display); \
                        } \
                        break;
                        NDDI_COMMAND_LIST(GENERATE_PLAY_CASE)
                        case idEOT:
                        default:
                            break;
                        }
                    }
                }
                usleep(10);
            }

            if (display) { delete display; }
        }

        void run() {
            std::ifstream is(file);
            cereal::XMLInputArchive iarchive(is);

            CommandID id;
            iarchive(id);
            while (id != idEOT) {
                NddiCommandMessage* msg = NULL;
                switch (id) {
                #define GENERATE_READ_CASE(m) \
                case id ## m : { \
                    msg = new m ## CommandMessage(); \
                    iarchive(*( m ## CommandMessage *)msg); \
                } \
                break;
                NDDI_COMMAND_LIST(GENERATE_READ_CASE)
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
            player->play();
        }

        ~RecorderNddiDisplay() {
            if (recorder) { delete(recorder); }
            if (player) { delete(player); }
        }

        unsigned int DisplayWidth() {
            NddiCommandMessage* msg = new DisplayWidthCommandMessage();
            recorder->record(msg);
            return 0;
        }

        unsigned int DisplayHeight() {
            NddiCommandMessage* msg = new DisplayHeightCommandMessage();
            recorder->record(msg);
            return 0;
        }

        unsigned int NumCoefficientPlanes() {
            NddiCommandMessage* msg = new NumCoefficientPlanesCommandMessage();
            recorder->record(msg);
            return 0;
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

        void UpdateInputVector(vector<int> &input) {
            NddiCommandMessage* msg = new UpdateInputVectorCommandMessage(input);
            recorder->record(msg);
        }

        void PutCoefficientMatrix(vector< vector<int> > &coefficientMatrix, vector<unsigned int> &location) {
            NddiCommandMessage* msg = new PutCoefficientMatrixCommandMessage(coefficientMatrix, location);
            recorder->record(msg);
        }

        void FillCoefficientMatrix(vector< vector<int> > &coefficientMatrix, vector<unsigned int> &start, vector<unsigned int> &end) {
            NddiCommandMessage* msg = new FillCoefficientMatrixCommandMessage(coefficientMatrix, start, end);
            recorder->record(msg);
        }

        void FillCoefficient(int coefficient, unsigned int row, unsigned int col, vector<unsigned int> &start, vector<unsigned int> &end) {
            NddiCommandMessage* msg = new FillCoefficientCommandMessage(coefficient, row, col, start, end);
            recorder->record(msg);
        }

        void FillCoefficientTiles(vector<int> &coefficients, vector<vector<unsigned int> > &positions, vector<vector<unsigned int> > &starts, vector<unsigned int> &size) {
            NddiCommandMessage* msg = new FillCoefficientTilesCommandMessage(coefficients, positions, starts, size);
            recorder->record(msg);
        }

        void FillScaler(Scaler scaler, vector<unsigned int> &start, vector<unsigned int> &end) {
            NddiCommandMessage* msg = new FillScalerCommandMessage(scaler, start, end);
            recorder->record(msg);
        }

        void FillScalerTiles(vector<uint64_t> &scalers, vector<vector<unsigned int> > &starts, vector<unsigned int> &size) {
            NddiCommandMessage* msg = new FillScalerTilesCommandMessage(scalers, starts, size);
            recorder->record(msg);
        }

        void FillScalerTileStack(vector<uint64_t> &scalers, vector<unsigned int> &start, vector<unsigned int> &size) {
            NddiCommandMessage* msg = new FillScalerTileStackCommandMessage(scalers, start, size);
            recorder->record(msg);
        }

        void SetPixelByteSignMode(SignMode mode) {
            NddiCommandMessage* msg = new SetPixelByteSignModeCommandMessage(mode);
            recorder->record(msg);
        }

        void SetFullScaler(uint16_t scaler) {
            NddiCommandMessage* msg = new SetFullScalerCommandMessage(scaler);
            recorder->record(msg);
        }

        uint16_t GetFullScaler() {
            NddiCommandMessage* msg = new GetFullScalerCommandMessage();
            recorder->record(msg);
            return 0;
        }

        void Latch() {
            NddiCommandMessage* msg = new LatchCommandMessage();
            recorder->record(msg);
        }

        CostModel* GetCostModel() {}

    private:
        vector<unsigned int>  frameVolumeDimensionalSizes_;
        unsigned int          inputVectorSize_;
        unsigned int          numCoefficientPlanes_;
        Recorder*             recorder = NULL;
        Player*               player = NULL;
    };
}

#endif // RECORDER_NDDI_DISPLAY_H
