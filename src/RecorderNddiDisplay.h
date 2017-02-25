#ifndef RECORDER_NDDI_DISPLAY_H
#define RECORDER_NDDI_DISPLAY_H

#include "GrpcNddiDisplay.h"
#include "NddiCommands.h"
#include "nddi/Features.h"
#include "nddi/NDimensionalDisplayInterface.h"

#include <fstream>
#include <pthread.h>
#include <string.h>
#include <queue>
#include <unistd.h>
#include <vector>

//#include <cereal/archives/xml.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>

namespace nddi {

    class CommandRecorder {
    public:
        CommandRecorder(string file)
        : finished(false),
          file(file) {
            streamMutex = PTHREAD_MUTEX_INITIALIZER;
            typedef void* (*rptr)(void*);
            if (pthread_create( &streamThread, NULL, pthreadFriendlyRun, this)) {
                std::cout << "Error: Failed to start thread." << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        ~CommandRecorder() {
            finished = true;
            pthread_join(streamThread, NULL);
        }

        void run() {
            std::ofstream os(file, std::ofstream::out);
            //cereal::XMLOutputArchive oarchive(os);
            cereal::BinaryOutputArchive oarchive(os);

            while (!finished || !streamQueue.empty()) {
                if (!streamQueue.empty()) {
                    pthread_mutex_lock(&streamMutex);
                    NddiCommandMessage* msg = streamQueue.front();
                    streamQueue.pop();
                    pthread_mutex_unlock(&streamMutex);

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
                } else { usleep(10); }
            }
            oarchive(cereal::make_nvp("id", idEOT));
        }

        void record(NddiCommandMessage* msg) {
            pthread_mutex_lock(&streamMutex);
            streamQueue.push(msg);
            pthread_mutex_unlock(&streamMutex);
        }

    private:
        bool finished;
        string file;
        pthread_mutex_t streamMutex;
        pthread_t streamThread;
        static void * pthreadFriendlyRun(void * This) {((CommandRecorder*)This)->run(); return NULL;}
        std::queue<NddiCommandMessage*> streamQueue;
    };

    class CommandPlayer {
    public:
        CommandPlayer() : finished(true), file("recording") {
            streamMutex = PTHREAD_MUTEX_INITIALIZER;
        }

        CommandPlayer(string file)
        : finished(false),
          file(file) {
            streamMutex = PTHREAD_MUTEX_INITIALIZER;
        }

        ~CommandPlayer() {
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
                    pthread_mutex_lock(&streamMutex);
                    NddiCommandMessage* msg = streamQueue.front();
                    streamQueue.pop();
                    pthread_mutex_unlock(&streamMutex);

                    if (msg) {
                        CommandID id = msg->id;
                        //std::cout << "Popped a " << CommandNames[id] << std::endl;;
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
                } else { usleep(10); }
            }

            // If we created a GRCP Display, then wait a second, then latch a final time and destroy it.
            if (display) {
                sleep(1);
                display->Latch();
                delete display;
            }
        }

        void run() {
            std::ifstream is(file);
            //cereal::XMLInputArchive iarchive(is);
            cereal::BinaryInputArchive iarchive(is);

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
                    pthread_mutex_lock(&streamMutex);
                    streamQueue.push(msg);
                    pthread_mutex_unlock(&streamMutex);
                }

                iarchive(id);
            }
            finished = true;
        }

    private:
        bool finished;
        string file;
        pthread_mutex_t streamMutex;
        pthread_t streamThread;
        static void * pthreadFriendlyRun(void * This) {((CommandPlayer*)This)->run(); return NULL;}
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
        : RecorderNddiDisplay(frameVolumeDimensionalSizes, displayWidth, displayHeight,
                numCoefficientPlanes, inputVectorSize, "recording") {
        }
        RecorderNddiDisplay(vector<unsigned int> &frameVolumeDimensionalSizes,
                unsigned int displayWidth, unsigned int displayHeight,
                unsigned int numCoefficientPlanes, unsigned int inputVectorSize,
                string file)
        : frameVolumeDimensionalSizes_(frameVolumeDimensionalSizes),
          displayWidth_(displayWidth),
          displayHeight_(displayHeight),
          inputVectorSize_(inputVectorSize),
          numCoefficientPlanes_(numCoefficientPlanes) {
            recorder = new CommandRecorder(file);
            NddiCommandMessage* msg = new InitCommandMessage(frameVolumeDimensionalSizes,
                                                             displayWidth, displayHeight,
                                                             numCoefficientPlanes, inputVectorSize);
            recorder->record(msg);
        }

        RecorderNddiDisplay(char* file) {
            player = new CommandPlayer(file);
            player->play();
        }

        ~RecorderNddiDisplay() {
            if (recorder) { delete(recorder); }
            if (player) { delete(player); }
        }

        unsigned int DisplayWidth() {
            NddiCommandMessage* msg = new DisplayWidthCommandMessage();
            recorder->record(msg);
            return displayWidth_;
        }

        unsigned int DisplayHeight() {
            NddiCommandMessage* msg = new DisplayHeightCommandMessage();
            recorder->record(msg);
            return displayHeight_;;
        }

        unsigned int NumCoefficientPlanes() {
            NddiCommandMessage* msg = new NumCoefficientPlanesCommandMessage();
            recorder->record(msg);
            return numCoefficientPlanes_;
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
            fullScaler_ = scaler;
            NddiCommandMessage* msg = new SetFullScalerCommandMessage(scaler);
            recorder->record(msg);
        }

        uint16_t GetFullScaler() {
            NddiCommandMessage* msg = new GetFullScalerCommandMessage();
            recorder->record(msg);
            return fullScaler_;
        }

        void Latch() {
            NddiCommandMessage* msg = new LatchCommandMessage();
            recorder->record(msg);
        }

        CostModel* GetCostModel() {}

    private:
        vector<unsigned int>  frameVolumeDimensionalSizes_;
        unsigned int          displayWidth_;
        unsigned int          displayHeight_;
        unsigned int          inputVectorSize_;
        unsigned int          numCoefficientPlanes_;
        uint16_t              fullScaler_{0xff};
        CommandRecorder*      recorder = NULL;
        CommandPlayer*        player = NULL;
    };
}

#endif // RECORDER_NDDI_DISPLAY_H
