#ifndef RECORDER_NDDI_DISPLAY_H
#define RECORDER_NDDI_DISPLAY_H

/**
 * \file RecorderNddiDisplay.h
 *
 * \brief This file embodies a command recorder and player for the GrpcNddiDisplay.
 *
 * This file embodies a command recorder for the GrpcNddiDisplay. This
 * allows for recording of nDDI commands and then their playback.
 */

#include "GrpcNddiDisplay.h"
#include "NddiCommands.h"
#include "nddi/Features.h"
#include "nddi/NDimensionalDisplayInterface.h"

#include <fstream>
#include <pthread.h>
#include <string.h>
#include <thread>
#include <queue>
#include <unistd.h>
#include <vector>

///\cond
//#include <cereal/archives/xml.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
///\endcond

namespace nddi {

///\cond
    /**
     * \brief Implements and NDDI display where each interface is a GRPC call to the NDDI Wall Server.
     *
     * Implements and NDDI display where each interface is a GRPC call to the NDDI Wall Server.
     */
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
                } else { std::this_thread::yield(); }
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
                } else { std::this_thread::yield(); }
            }

            // If we created a GRPC Display, then wait a second, then latch a final time and destroy it.
            if (display) {
                sleep(1);
                display->Latch(0, 0, 1, 1);
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
///\endcond

    /**
     * \brief Implements and nDDI display where each interface is a recorder of nDDI Commands.
     *
     * Implements and nDDI display where each interface is a recorder of nDDI Commands.
     * A RecorderNddiDisplay can additionally be instantiated with a recording file enabling
     * it to playback the nDDI Commands which are then sent to an nDDI display wall server.
     */
    class RecorderNddiDisplay : public NDimensionalDisplayInterface {

    public:
        /**
         * \brief Required default constructor for abstract class NDimensionalDisplayInterface.
         *
         * Required default constructor for abstract class NDimensionalDisplayInterface.
         */
        RecorderNddiDisplay();

        /**
         * \brief Minimal constructor which uses a fixed display size.
         *
         * Minimal constructor which uses a fixed display size and configures the nDDI display with the provided
         * dimensions for the Input Vector, Coefficient Planes, and Frame Volume. The implementation builds an
	 * Initialize command from the arguments and records it to a file named "recording".
         * @param frameVolumeDimensionalSizes This vector is used to configure the frame volume.
         *                                    Each element in the vector represents a dimension and that element's
         *                                    value represents the size of that dimension. e.g. a simple 4x4 2D
         *                                    frame volume will be configured with a two-element vector with 4 and 4 in it.
         * @param numCoefficientPlanes Specifies how many of the maximum coefficient planes will be used.
         * @param inputVectorSize Used to configure the size of the input vector. It must be greater than or equal to two.
         * @param fixed8x8Macroblocks Configuration option which can configure the nDDI display to have fixed macroblocks.
         *                            This is strictly used for memory optimizations so that one coefficient matrix is used
         *                            for an entire macro block instead of one per pixel location. Note: This is still per plane.
         * @param useSingleCoefficientPlane Configuration option which can configure the display implementation to use only one
         *                                  Coefficient Plane even when it's configured to used multiple planes. The configured
         *                                  number of planes are still all logically used for blending, but their coefficients may
         *                                  be different if using special values COEFFICIENT_MATRIX_X, COEFFICIENT_MATRIX_Y, or
         *                                  COEFFICIENT_MATRIX_P. This is strictly use for memory optimizations in simulation.
         */
        RecorderNddiDisplay(vector<unsigned int> &frameVolumeDimensionalSizes,
                unsigned int numCoefficientPlanes, unsigned int inputVectorSize,
                bool fixed8x8Macroblocks = false, bool useSingleCoeffcientPlane = false)
        : RecorderNddiDisplay(frameVolumeDimensionalSizes, 640, 480, numCoefficientPlanes, inputVectorSize,
                fixed8x8Macroblocks, useSingleCoeffcientPlane) {
        }

        /**
         * \brief Full constructor which additionally allows the display size to be configured.
         *
         * Full constructor which additionally allows the display size to be configured. The implementation builds
	 * an Initialize command from the arguments and  records it to a file named "recording".
         * @param frameVolumeDimensionalSizes This vector is used to configure the frame volume.
         *                                    Each element in the vector represents a dimension and that element's
         *                                    value represents the size of that dimension. e.g. a simple 4x4 2D
         *                                    frame volume will be configured with a two-element vector with 4 and 4 in it.
         * @param displayWidth Used to configure the width of the display if it is less than the display device.
         * @param displayHeight Used to configure the width of the display if it is less than the display device.
         * @param numCoefficientPlanes Specifies how many of the maximum coefficient planes will be used.
         * @param inputVectorSize Used to configure the size of the input vector. It must be greater than or equal to two.
         * @param fixed8x8Macroblocks Configuration option which can configure the nDDI display to have fixed macroblocks.
         *                            This is strictly used for memory optimizations so that one coefficient matrix is used
         *                            for an entire macro block instead of one per pixel location. Note: This is still per plane.
         * @param useSingleCoefficientPlane Configuration option which can configure the display implementation to use only one
         *                                  Coefficient Plane even when it's configured to used multiple planes. The configured
         *                                  number of planes are still all logically used for blending, but their coefficients may
         *                                  be different if using special values COEFFICIENT_MATRIX_X, COEFFICIENT_MATRIX_Y, or
         *                                  COEFFICIENT_MATRIX_P. This is strictly use for memory optimizations in simulation.
         */
        RecorderNddiDisplay(vector<unsigned int> &frameVolumeDimensionalSizes,
                unsigned int displayWidth, unsigned int displayHeight,
                unsigned int numCoefficientPlanes, unsigned int inputVectorSize,
                bool fixed8x8Macroblocks = false, bool useSingleCoeffcientPlane = false)
        : RecorderNddiDisplay(frameVolumeDimensionalSizes, displayWidth, displayHeight,
                numCoefficientPlanes, inputVectorSize,
                "recording",
                fixed8x8Macroblocks, useSingleCoeffcientPlane) {
        }
	
        /**
         * \brief Full constructor which additionally allows the display size and recording file to be configured.
         *
         * Full constructor which additionally allows the display size and recording file to be configured.
	 * The implementation builds an Initialize command from the arguments and  records it to specificed file.
         * @param frameVolumeDimensionalSizes This vector is used to configure the frame volume.
         *                                    Each element in the vector represents a dimension and that element's
         *                                    value represents the size of that dimension. e.g. a simple 4x4 2D
         *                                    frame volume will be configured with a two-element vector with 4 and 4 in it.
         * @param displayWidth Used to configure the width of the display if it is less than the display device.
         * @param displayHeight Used to configure the width of the display if it is less than the display device.
         * @param numCoefficientPlanes Specifies how many of the maximum coefficient planes will be used.
         * @param inputVectorSize Used to configure the size of the input vector. It must be greater than or equal to two.
	 * @param file Specifies the path and filename where the commands will be recorded to.
         * @param fixed8x8Macroblocks Configuration option which can configure the nDDI display to have fixed macroblocks.
         *                            This is strictly used for memory optimizations so that one coefficient matrix is used
         *                            for an entire macro block instead of one per pixel location. Note: This is still per plane.
         * @param useSingleCoefficientPlane Configuration option which can configure the display implementation to use only one
         *                                  Coefficient Plane even when it's configured to used multiple planes. The configured
         *                                  number of planes are still all logically used for blending, but their coefficients may
         *                                  be different if using special values COEFFICIENT_MATRIX_X, COEFFICIENT_MATRIX_Y, or
         *                                  COEFFICIENT_MATRIX_P. This is strictly use for memory optimizations in simulation.
         */
        RecorderNddiDisplay(vector<unsigned int> &frameVolumeDimensionalSizes,
                unsigned int displayWidth, unsigned int displayHeight,
                unsigned int numCoefficientPlanes, unsigned int inputVectorSize,
                string file,
                bool fixed8x8Macroblocks = false, bool useSingleCoeffcientPlane = false)
        : frameVolumeDimensionalSizes_(frameVolumeDimensionalSizes),
          displayWidth_(displayWidth),
          displayHeight_(displayHeight),
          inputVectorSize_(inputVectorSize),
          numCoefficientPlanes_(numCoefficientPlanes),
          fixed8x8Macroblocks_(fixed8x8Macroblocks),
          useSingleCoeffcientPlane_(useSingleCoeffcientPlane) {
            recorder = new CommandRecorder(file);
            NddiCommandMessage* msg = new InitCommandMessage(frameVolumeDimensionalSizes,
                                                             displayWidth, displayHeight,
                                                             numCoefficientPlanes, inputVectorSize,
                                                             fixed8x8Macroblocks, useSingleCoeffcientPlane);
            recorder->record(msg);
        }

        /**
         * \brief Constructor for nDDI command playback from recording file.
         *
         * Constructor for nDDI command playback from recording file. This creates a player, enabling
	 * the Play() function which will decode the commands and send them to an nDDI display wall server.
	 * @param file Specifies the path and filename where the commands are read from.
         */
        RecorderNddiDisplay(char* file) {
            player = new CommandPlayer(file);
        }

	/**
	 * \brief Default destructor.
	 *
	 * Default desctructor. Destroys the recorder and/or player.
	 */
        ~RecorderNddiDisplay() {
            if (recorder) { delete(recorder); }
            if (player) { delete(player); }
        }

        /**
         * \brief Used to query the display width.
         *
         * Used to query the display width. Records a DisplayWidth command.
         * @return The width of the display.
         */
        unsigned int DisplayWidth() {
            NddiCommandMessage* msg = new DisplayWidthCommandMessage();
            recorder->record(msg);
            return displayWidth_;
        }

        /**
         * \brief Used to query the display height.
         *
         * Used to query the display height. Records a DisplayHeight command.
         * @return The height of the display.
         */
        unsigned int DisplayHeight() {
            NddiCommandMessage* msg = new DisplayHeightCommandMessage();
            recorder->record(msg);
            return displayHeight_;;
        }

        /**
         * \brief Used to query the number of coefficient planes.
         *
         * Used to query the number of coefficient planes. Records a NumCoefficients command.
         * @return The number of coefficient planes.
         */
        unsigned int NumCoefficientPlanes() {
            NddiCommandMessage* msg = new NumCoefficientPlanesCommandMessage();
            recorder->record(msg);
            return numCoefficientPlanes_;
        }

        /**
         * \brief Copies the provided pixel to the specified location.
         *
         * Copies the provided pixel to the specified location. Builds a PutPixel command from the arguments
	 * and records it.
         * @param p The pixel value to be copied.
         * @param location Tuple for the location within the frame volume where the pixel will be copied to.
         */
        void PutPixel(Pixel p, vector<unsigned int> &location) {
            NddiCommandMessage* msg = new PutPixelCommandMessage(p, location);
            recorder->record(msg);
        }

        /**
         * \brief Copies the one dimensional array of pixels along a particular dimension in the frame volume.
         *
         * Copies the one dimensional array of pixels along a particular dimension in the frame volume. In a
         * two-dimensional frame volume, this can be thought of as a way to copy along a row or along a column, but
         * not both since the input pixels are only one-dimensional. Builds a CopyPixelStrip command from the arguments
	 * and records it.
         * @param p The pointer to the pixel values to be copied.
         * @param start Tuple for the first pixel in the frame volume to be filled.
         * @param end Tuple for the last pixel in the frame volume to be filled. All but one of the values in
         *            values in this last pixel should be identical to the start pixel.
         */
        void CopyPixelStrip(Pixel* p, vector<unsigned int> &start, vector<unsigned int> &end) {
            NddiCommandMessage* msg = new CopyPixelStripCommandMessage(p, start, end);
            recorder->record(msg);
        }

        /**
         * \brief Copies the array of pixels into the designated region of the frame volume.
         *
         * Copies the array of pixels into the designated region of the frame volume. The data must be
         * arranged in the array with strides for each dimension of the area. So to copy pixels into a
         * 2 x 2 x 2 region in the frame volume, the array must be arranged accordingly:
         * (0,0,0) (1,0,0) (0,1,0) (1,1,0) (0,0,1) (1,0,1) (0,1,1) (1,1,1)
	 * Builds a CopyPixels command from the arguments and records it.
         * @param p The pointer to the pixel values to be copied.
         * @param start Tuple for the first pixel in the frame volume to be filled.
         * @param end Tuple for the last pixel in the frame volume to be filled.
         */
        void CopyPixels(Pixel* p, vector<unsigned int> &start, vector<unsigned int> &end) {
            NddiCommandMessage* msg = new CopyPixelsCommandMessage(p, start, end);
            recorder->record(msg);
        }

        /**
         * \brief Copies the array of pixels into the designated tile regions of the frame volume.
         *
         * Copies the array of pixels into the designated tile regions of the frame volume. The data must be
         * arranged in the array with strides for each dimension of the area. Only 2D tiles are supported.
	 * Builds a CopyPixelTiles command and records it.
         * @param p The pointer to the pixel values to be copied.
         * @param starts Vector holding series of tuples for the first pixel for each destination tile in the frame volume.
         * @param size Two element tuple for the size of each tile (w, h).
         */
        void CopyPixelTiles(vector<Pixel*> &p, vector<vector<unsigned int> > &starts, vector<unsigned int> &size) {
            NddiCommandMessage* msg = new CopyPixelTilesCommandMessage(p, starts, size);
            recorder->record(msg);
        }

        /**
         * \brief Fills the frame volume with the specified pixel.
         *
         * Fills the frame volume with the specified pixel. It can fill in multiple
         * dimensions by starting at the start pixel and filling in each dimension until
         * the end pixel value is reached. Builds a FillPixel command and records it.
         * @param p The pixel value to be filled.
         * @param start Tuple for the first pixel in the frame volume to be filled.
         * @param end Tuple for the last pixel in the frame volume to be filled.
         */
        void FillPixel(Pixel p, vector<unsigned int> &start, vector<unsigned int> &end) {
            NddiCommandMessage* msg = new FillPixelCommandMessage(p, start, end);
            recorder->record(msg);
        }

        /**
         * \brief Copies pixels from one multi-dimensional region of the frame volume to another region.
         *
         * Copies pixels from one multi-dimensional region of the frame volume to another region.
	 * Builds a CopyFrameVolume command and records it.
         * @param start Tuple for the starting coordinate of the source region.
         * @param end Tuple for the ending coordinate of the source region.
         * @param dest Tuple for the first starting pixel of the destination region to be filled.
         */
        void CopyFrameVolume(vector<unsigned int> &start, vector<unsigned int> &end, vector<unsigned int> &dest) {
            NddiCommandMessage* msg = new CopyFrameVolumeCommandMessage(start, end, dest);
            recorder->record(msg);
        }

        /**
         * \brief Used to update the input vector with the extra values in the input vector.
         *
         * Used to update the input vector with the extra values in the input vector.
	 * Builds a UpdateInputVector command and records it.
         * @param input Tuple for the values to use for the update. The length of this
         *              tuple must equal the size of the actual input vector
         *              minus two, since the first two values in the input
         *              vector cannot be changed.
         */
        void UpdateInputVector(vector<int> &input) {
            NddiCommandMessage* msg = new UpdateInputVectorCommandMessage(input);
            recorder->record(msg);
        }

        /**
         * \brief Used to copy the specified coefficientMatrix into the specified location of the coefficient
         *        planes.
         *
         * Used to copy the specified coefficientMatrix into the specified location of the coefficient
         * planes. Builds a PutCoefficientMatrix command and records it.
         * @param coefficientMatrix This two-dimensional vector holds the matrix to be copied.
         *                          It's size must match the configuration of the coefficient matrices
         *                          exactly. Can use COFFICIENT_UNCHANGED for one or more elements.
         * @param location This two-element vector specifies the tuple for the location in the coefficient plane where the provided
         *                 coefficient matrix will be copied.
         */
        void PutCoefficientMatrix(vector< vector<int> > &coefficientMatrix, vector<unsigned int> &location) {
            NddiCommandMessage* msg = new PutCoefficientMatrixCommandMessage(coefficientMatrix, location);
            recorder->record(msg);
        }

        /**
         * \brief Used to copy the specified coefficientMatrix into a range of locations in the coefficient planes.
         *
         * Used to copy the specified coefficientMatrix into a range of locations in the coefficient planes.
	 * Builds a FillCoefficientMatrix command and records it.
         * @param coefficientMatrix This two-dimensional vector holds the matrix to be copied.
         *                          It's size must match the configuration of the coefficient matrices
         *                          exactly. Can use COFFICIENT_UNCHANGED for one or more elements.
         * @param start This three-element vector specifies the tuple of location in the coefficient planes where the first
         *              coefficient matrix will be copied to.
         * @param end This three-element vector specifies the tuple of the location in the coefficient planes where the last
         *            coefficient matrix will be copied to.
         */
        void FillCoefficientMatrix(vector< vector<int> > &coefficientMatrix, vector<unsigned int> &start, vector<unsigned int> &end) {
            NddiCommandMessage* msg = new FillCoefficientMatrixCommandMessage(coefficientMatrix, start, end);
            recorder->record(msg);
        }

        /**
         * \brief Used to copy the specified single coefficient value from a matrix into a range of locations in the coefficient planes.
         *
         * Used to copy the specified single coefficient value from a matrix into a range of locations in the coefficient planes.
	 * Builds a FillCoefficient command and records it.
         * @param coefficient This single value will be placed into each coefficient at the specified location in the coefficient
         *                    matrices of the specified range.
         * @param row The row of the coefficient to be updated in the coefficient matrix.
         * @param col The column of the coefficient to be updated in the coefficient matrix.
         * @param start This three-element vector specifies the tuple of location in the coefficient planes where the first
         *              coefficient matrix will be copied to.
         * @param end This three-element vector specifies the tuple of location in the coefficient planes where the last
         *            coefficient matrix will be copied to.
         */
        void FillCoefficient(int coefficient, unsigned int row, unsigned int col, vector<unsigned int> &start, vector<unsigned int> &end) {
            NddiCommandMessage* msg = new FillCoefficientCommandMessage(coefficient, row, col, start, end);
            recorder->record(msg);
        }

        /**
         * \brief For each coefficient, positions, and start; copies the coefficient to the position
         *        in the in each coefficient matrix in the 2D tile specified by the start and size.
         *
         * For each coefficient, positions, and start; copies the coefficient to the position
         * in the in each coefficient matrix in the 2D tile specified by the start and size.
	 * Builds a FillCoefficientTiles command and records it.
         * @param coefficients The buffer of coefficients.
         * @param positions Tuple for the position (row, col) to place the coefficient within the coefficient matrix.
         * @param starts Tuple for the location (x, y) of the start of the tile in the coefficient planes.
         * @param size Tuple for the size (w, h) of the tile.
         */
        void FillCoefficientTiles(vector<int> &coefficients, vector<vector<unsigned int> > &positions, vector<vector<unsigned int> > &starts, vector<unsigned int> &size) {
            NddiCommandMessage* msg = new FillCoefficientTilesCommandMessage(coefficients, positions, starts, size);
            recorder->record(msg);
        }

        /**
         * \brief Used to copy the specified scaler to a range of locations in the coefficient planes.
         *
         * Used to copy the specified scaler to a range of locations in the coefficient planes.
	 * Builds a FillScaler command and records it.
         * @param scaler This single scaler will be copied to each location in the range of coefficient planes.
         * @param start This three-element vector specifies tuple for the start of the range in the coefficient planes
         *              where the scaler will be copied to.
         * @param end This three-element vector specifies the tuple for end of the range in the coefficient planes where
         *            the scalers will be copied to.
         */
        void FillScaler(Scaler scaler, vector<unsigned int> &start, vector<unsigned int> &end) {
            NddiCommandMessage* msg = new FillScalerCommandMessage(scaler, start, end);
            recorder->record(msg);
        }

        /**
         * \brief Used to copy the specified scalers to a series of 2D ranges of locations (tiles) in the coefficient planes.
         *
         * Used to copy the specified scalers to a series of 2D ranges of locations (tiles) in the coefficient planes.
         * This is accomplished with a set of scalers, an equal number of tile starts, and one tile size.
	 * Builds a FillScalerTiles command and records it.
         * @param scalers Each scaler in this list will be filled to its own tile, which is a 2D range of coefficient matrices.
         * @param starts Vector of tuples for the start locations (x, y, z) in the coefficient planes for each tile to be filled.
         * @param size Tuple for the size (w, h) of the tile.
         */
        void FillScalerTiles(vector<uint64_t> &scalers, vector<vector<unsigned int> > &starts, vector<unsigned int> &size) {
            NddiCommandMessage* msg = new FillScalerTilesCommandMessage(scalers, starts, size);
            recorder->record(msg);
        }

        /**
         * \brief Used to copy the specified scalers to a stack of 2D ranges of locations (tiles) in the coefficient planes.
         *
         * Used to copy the specified scalers to a stack of 2D ranges of locations (tiles) in the coefficient planes.
         * This is accomplished with with a set of scalers, a single tile stack location, and one tile size.
         * The stack includes the top-most tile on the coefficient plane indicated by the tile start as well as
         * the tiles for the planes under (higher plane coordinates) the start tile. The height of the stack to be
         * filled is determined by the number of scalers provided. Builds a FillScalerTileStack command and records it.
         * @param scalers Each scaler in this list will be filled to its own tile (2D range of coefficient matrices) in the stack.
         * @param start Tuple for the location (x, y) of the start of the tile stack in the coefficient planes.
         * @param size Tuple for the size (w, h) of the tile.
         */
        void FillScalerTileStack(vector<uint64_t> &scalers, vector<unsigned int> &start, vector<unsigned int> &size) {
            NddiCommandMessage* msg = new FillScalerTileStackCommandMessage(scalers, start, size);
            recorder->record(msg);
        }

        /**
         * \brief Allows the bytes of pixel values to be interpretted as signed values when scaling, accumulating, and clamping
         * in the pixel blending pipeline.
         *
         * Allows the bytes of pixel values to be interpretted as signed values when scaling, accumulating, and clamping
         * in the pixel blending pipeline. When the SignMode is set to the default UNSIGNED_MODE, each 8-bit color channel for
         * pixels will be treated as an unsigned value. Setting this configuration option to SIGNED_MODE treats those channels
         * as 8-bit signed values. Builds a SetPixelByteSignMode command and records it.
         * @param mode Can be UNSIGNED_MODE or SIGNED_MODE.
         */
        void SetPixelByteSignMode(SignMode mode) {
            NddiCommandMessage* msg = new SetPixelByteSignModeCommandMessage(mode);
            recorder->record(msg);
        }

        /**
         * \brief Used to set the scaler value which is interpretted as fully on or 100%.
         *
         * Used to set the scaler value which is interpretted as fully on or 100%. The default is
         * 256, which implies that any scaler sent by the client is an
         * integer fraction of 256, but in fact a scaler can be larger than 256,
         * leading to planes that contribute 2.5x or even -3x for instance. Builds a SetFullScaler
	 * command and records it.
         * @param scaler The value to be interpretted as fully on or 100%.
         */
        void SetFullScaler(uint16_t scaler) {
            fullScaler_ = scaler;
            NddiCommandMessage* msg = new SetFullScalerCommandMessage(scaler);
            recorder->record(msg);
        }

        /**
         * \brief Used to get the current full scaler value.
         *
         * Used to get the current full scaler value. Records a GetFullScaler command to the server
	 * and returns the local cached value.
         * @return The current fully on scaler value.
         */
        uint16_t GetFullScaler() {
            NddiCommandMessage* msg = new GetFullScalerCommandMessage();
            recorder->record(msg);
            return fullScaler_;
        }

        /**
         * \brief Records a ClearCostModel command.
         *
	 * Records a ClearCostModel command.
         */
        void ClearCostModel() {
            NddiCommandMessage* msg = new ClearCostModelCommandMessage();
            recorder->record(msg);
        }

        /**
         * \brief Builds and records a Latch command for the given subregion.
         *
	 * Builds and records a Latch command for the given subregion.
	 * @param sub_x The x coordinate of the start of the subregion.
	 * @param sub_y The y coordinate of the start of the subregion.
	 * @param sub_w The width of the subregion
 	 * @param sub_h The height of the subregion
         */
        void Latch(uint32_t sub_x, uint32_t sub_y, uint32_t sub_w, uint32_t sub_h) {
            NddiCommandMessage* msg = new LatchCommandMessage(sub_x, sub_y, sub_w, sub_h);
            recorder->record(msg);
        }

        /**
         * \brief Records a Shutdown command.
         *
	 * Records a Shutdown command.
         */
        void Shutdown() {
            NddiCommandMessage* msg = new ShutdownCommandMessage();
            recorder->record(msg);
        }

	/**
	 * \brief Not implemented since the GrpcNddiDisplay doesn't support such an nDDI command.
	 *
	 * Not implemented since the GrpcNddiDisplay doesn't support such an nDDI command.
	 */
        CostModel* GetCostModel() {}

	/**
	 * \brief Used to playback recorded nDDI commands.
	 *
	 * Used to playback recorded nDDI commands. When the RecorderNddiDisplay is instantiated
	 * as a player, this interface is used to start the playback. The player reads the
	 * recorded commands from the file provided to the constructor. This call returns when
	 * the last command is played.
	 */
        void Play() {
            player->play();
        }

    private:
        vector<unsigned int>  frameVolumeDimensionalSizes_;
        unsigned int          displayWidth_;
        unsigned int          displayHeight_;
        unsigned int          inputVectorSize_;
        bool                  fixed8x8Macroblocks_;
        bool                  useSingleCoeffcientPlane_;
        unsigned int          numCoefficientPlanes_;
        uint16_t              fullScaler_{0xff};
        CommandRecorder*      recorder = NULL;
        CommandPlayer*        player = NULL;
    };
}

#endif // RECORDER_NDDI_DISPLAY_H
