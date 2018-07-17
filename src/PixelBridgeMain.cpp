#include <iostream>
#include <sys/time.h>
#include <cmath>
#include <assert.h>
#include <queue>
#include <pthread.h>

#include <opencv/cv.h>

#include "PixelBridgeFeatures.h"
#include "Configuration.h"

#include "GrpcNddiDisplay.h"
#include "RecorderNddiDisplay.h"

#include "CachedTiler.h"
#include "DctTiler.h"
#include "ScaledDctTiler.h"
#include "MultiDctTiler.h"
#include "ItTiler.h"
#include "FlatTiler.h"
#include "FfmpegPlayer.h"
#include "RandomPlayer.h"
#include "Rewinder.h"


typedef enum {
    NOT_REWINDING,  // Normal playback, not rewinding
    PLAY_BACKWARDS, // Rewinding frame by frame backwards
    PLAY_FORWARD    // Finished rewinding, playing forward from memory
} rewind_play_t;

// General Globals
size_t displayWidth = 40, displayHeight = 32;
const char* fileName = NULL;
Configuration globalConfiguration = Configuration();

// Helper Objects
Player*  myPlayer;
NDimensionalDisplayInterface* myDisplay;
Tiler* myTiler;
Rewinder* myRewinder = NULL;

// Decoder thread
pthread_t           decoderThread;

// Statistical Instrumentation
int totalUpdates = 0;
timeval startTime, endTime; // Used for timing data

// Stores the current and previously decoded frame
uint8_t* videoBuffer = NULL;
uint8_t* lastBuffer = NULL;

#ifdef USE_ASYNC_DECODER
// Buffers decoded frame
queue<uint8_t*> bufferQueue;

// Synchronization variables
pthread_mutex_t      bufferQueueMutex;
#endif

bool finished = false;

void setupDisplay() {

    Scaler s;

    // Cached-Tiled
    if (globalConfiguration.tiler == CACHE) {

        // Setup Cached Tiler which initializies Coefficient Plane
        myTiler = new CachedTiler(displayWidth, displayHeight,
                globalConfiguration.tileWidth, globalConfiguration.tileHeight,
                globalConfiguration.maxTiles, globalConfiguration.sigBits,
                globalConfiguration.recordFile);

        // Grab the display
        myDisplay = myTiler->GetDisplay();

    // DCT-Tiled
    } else if (globalConfiguration.tiler == DCT) {

        // Setup DCT Tiler and initializes Coefficient Plane and Frame Volume
#if (defined USE_SCALED_DCT) && (defined USE_MULTI_DCT)
#error Cannot use both USE_SCALED_DCT and USE_MULTI_DCT. Pick one or the other.
#endif
#if defined USE_SCALED_DCT
        myTiler = new ScaledDctTiler(displayWidth, displayHeight,
                                         globalConfiguration.quality,
                                         globalConfiguration.recordFile);
#elif (defined USE_MULTI_DCT)
        myTiler = new MultiDctTiler(displayWidth, displayHeight,
                                         globalConfiguration.quality,
                                         globalConfiguration.recordFile);
#else
        myTiler = new DctTiler(displayWidth, displayHeight,
                               globalConfiguration.quality,
                               globalConfiguration.recordFile);
#endif

        // Grab the display
        myDisplay = myTiler->GetDisplay();

    // IT-Tiled
    } else if (globalConfiguration.tiler == IT) {

        // Setup IT Tiler and initializes Coefficient Plane and Frame Volume
        myTiler = new ItTiler(displayWidth, displayHeight,
                              globalConfiguration.quality,
                              globalConfiguration.recordFile);

        // Grab the display
        myDisplay = myTiler->GetDisplay();

    // Flat-Tiled
    } else if (globalConfiguration.tiler == FLAT) {

        // Set up Flat Tiler and initialize Coefficient Planes
        myTiler = new FlatTiler(displayWidth, displayHeight,
                                globalConfiguration.tileWidth,
                                globalConfiguration.tileHeight,
                                globalConfiguration.sigBits,
                                globalConfiguration.recordFile);

        // Grab the display
        myDisplay = myTiler->GetDisplay();

    // Simple Framebuffer
    } else {

        assert(!globalConfiguration.isSlave && "Slave support not implemented for this mode!");
        assert(globalConfiguration.scale == 1 && "Scaling not supported for this mode!");

        // 2 dimensional matching the Video Width x Height
        vector<unsigned int> fvDimensions;
        fvDimensions.push_back(displayWidth);
        fvDimensions.push_back(displayHeight);

        if (globalConfiguration.recordFile.length()) {
            myDisplay = new RecorderNddiDisplay(fvDimensions,
                    displayWidth, displayHeight,
                    (unsigned int)1,
                    (unsigned int)2,
                    globalConfiguration.recordFile);
        } else {
            if (!globalConfiguration.isSlave) {
                myDisplay = new GrpcNddiDisplay(fvDimensions,
                        displayWidth, displayHeight,
                        (unsigned int)1,
                        (unsigned int)2);
            } else {
                myDisplay = new GrpcNddiDisplay();
            }
        }

        // Initialize Frame Volume
        nddi::Pixel p;
        p.r = p.g = p.b = p.a = 0xff;
        vector<unsigned int> start, end;
        start.push_back(0); start.push_back(0);
        end.push_back(displayWidth - 1); end.push_back(displayHeight - 1);
        myDisplay->FillPixel(p, start, end);

        // Initialize Coefficient Plane
        vector< vector<int> > coeffs;
        coeffs.resize(2);
        coeffs[0].push_back(1); coeffs[0].push_back(0);
        coeffs[1].push_back(0); coeffs[1].push_back(1);

        start.clear(); end.clear();

        start.push_back(0); start.push_back(0); start.push_back(0);
        end.push_back(displayWidth - 1); end.push_back(displayHeight - 1); end.push_back(0);

        myDisplay->FillCoefficientMatrix(coeffs, start, end);

        // Turn off all planes and then set the 0 plane to full on.
        end[2] = myDisplay->NumCoefficientPlanes() - 1;
        s.packed = 0;
        myDisplay->FillScaler(s, start, end);
        end[2] = 0;
        s.r = s.g = s.b = myDisplay->GetFullScaler();
        myDisplay->FillScaler(s, start, end);

    }

#ifdef CLEAR_COST_MODEL_AFTER_SETUP
    if (globalConfiguration.recordFile.length()) {
         ((RecorderNddiDisplay*)myDisplay)->ClearCostModel();
    } else {
        ((GrpcNddiDisplay*)myDisplay)->ClearCostModel();
    }
#endif

}


void updateDisplay(uint8_t* buffer, size_t width, size_t height) {

    // CACHE, DCT, IT, or FLAT
    if ( (globalConfiguration.tiler == CACHE) || (globalConfiguration.tiler == DCT) || (globalConfiguration.tiler == IT) || (globalConfiguration.tiler == FLAT) ) {
        // Update the display with the Tiler
        myTiler->UpdateDisplay(buffer, width, height);
    // SIMPLE
    } else {
        // Update the display like a simple Frame Buffer
        size_t bufferPos = 0;

        // Array of pixels used for framebuffer mode.
        Pixel* frameBuffer = (Pixel*)malloc(sizeof(Pixel) * displayWidth * displayHeight);

        // Transform the buffer into pixels
        for (int j = 0; j < displayHeight; j++) {
#ifdef USE_SMALLER_WINDOW
            // Set the bufferPos to the beginning of the next row
            bufferPos = j * width * 3;
#endif
            for (int i = 0; i < displayWidth; i++) {
                frameBuffer[j * displayWidth + i].r = buffer[bufferPos++];
                frameBuffer[j * displayWidth + i].g = buffer[bufferPos++];
                frameBuffer[j * displayWidth + i].b = buffer[bufferPos++];
                frameBuffer[j * displayWidth + i].a = 0xff;
            }
        }

        // Update the frame volume
        vector<unsigned int> start, end, dest;

        // Just send the pixels to the single plane
        start.push_back(0); start.push_back(0);
        end.push_back(displayWidth - 1); end.push_back(displayHeight - 1);
        myDisplay->CopyPixels(frameBuffer, start, end);

        // Free the temporary frame buffer
        free(frameBuffer);
    }
    if (globalConfiguration.recordFile.length()) {
        ((RecorderNddiDisplay*)myDisplay)->Latch(globalConfiguration.sub_x,
                                                 globalConfiguration.sub_y,
                                                 globalConfiguration.sub_w,
                                                 globalConfiguration.sub_h);
    } else {
        ((GrpcNddiDisplay*)myDisplay)->Latch(globalConfiguration.sub_x,
                                             globalConfiguration.sub_y,
                                             globalConfiguration.sub_w,
                                             globalConfiguration.sub_h);
    }
    totalUpdates++;

}


long calculateSE(uint8_t* videoIn, Pixel* frameOut) {
    long errR = 0, errG = 0, errB = 0;
    size_t i, bufferPos = 0;
    long SE = 0;

    for (i = 0; i < displayWidth * displayHeight; i++) {
        errR = videoIn[bufferPos++] - frameOut[i].r;
        errG = videoIn[bufferPos++] - frameOut[i].g;
        errB = videoIn[bufferPos++] - frameOut[i].b;
        SE += errR * errR + errG * errG + errB * errB;
    }

    return SE;
}


void renderFrame() {

    static int framesDecoded = 0;
    static int framesRendered = 0;
    static int currentFrame = 0;
    long SE = 0;

    static rewind_play_t rewindState = NOT_REWINDING;

    if (globalConfiguration.maxFrames && (totalUpdates >= globalConfiguration.maxFrames)) {
        finished = true;
    } else {
        // If we're not in a rewind backwards or forward state...
        if (rewindState == NOT_REWINDING) {
#ifdef USE_ASYNC_DECODER
            // Get a decoded frame
            // TODO(CDE): Don't poll. Use signals instead.
            while (bufferQueue.size() == 0) {
                usleep(5);
             }
            if ((videoBuffer = bufferQueue.front()) != NULL) {
                pthread_mutex_lock(&bufferQueueMutex);
                bufferQueue.pop();
                pthread_mutex_unlock(&bufferQueueMutex);
#else
            // Decode a frame
            if ((videoBuffer = myPlayer->decodeFrame()) != NULL) {
#endif
                framesDecoded++;

                // If we're past the designated start frame, then update the NDDI display
                if (framesDecoded >= globalConfiguration.startFrame) {
                    // If configured to rewind, then check to see if this decoded frame should be stored.
                    if ( myRewinder &&
                        (currentFrame >= (globalConfiguration.rewindStartFrame - globalConfiguration.rewindFrames)) &&
                        (currentFrame < globalConfiguration.rewindStartFrame) ) {
                        myRewinder->CopyFrame(videoBuffer, currentFrame - (globalConfiguration.rewindStartFrame - globalConfiguration.rewindFrames));
                    }

                    // Update NDDI Display
                    updateDisplay(videoBuffer,
                                  myPlayer->width(), myPlayer->height());

                    framesRendered++;

                    // If this is the final frame stored, then set the rewindState to PLAY_BACKWARDS
                    if (myRewinder && currentFrame == globalConfiguration.rewindStartFrame) {
                        rewindState = PLAY_BACKWARDS;
                        currentFrame--;
                    } else {
                        currentFrame++;
                    }
                }
            }

        // Else if we're in the rewind placy backwards state...
        } else if (rewindState == PLAY_BACKWARDS) {
            // Update NDDI Display
            updateDisplay(myRewinder->GetFrame(currentFrame - (globalConfiguration.rewindStartFrame - globalConfiguration.rewindFrames)),
                          myPlayer->width(), myPlayer->height());

            framesRendered++;

            // If this is the final frame stored, then set the rewindState to PLAY_BACKWARDS
            if (currentFrame == (globalConfiguration.rewindStartFrame - globalConfiguration.rewindFrames)) {
                rewindState = PLAY_FORWARD;
                currentFrame++;
            } else {
                currentFrame--;
            }
        // Else if we're in the rewind play forward state...
        } else if (rewindState == PLAY_FORWARD) {
            // Update NDDI Display
            updateDisplay(myRewinder->GetFrame(currentFrame - (globalConfiguration.rewindStartFrame - globalConfiguration.rewindFrames)),
                          myPlayer->width(), myPlayer->height());

            framesRendered++;

            // Increment the currentFrame
            currentFrame++;

            // If this is the final frame stored, then set the rewindState to PLAY_BACKWARDS
            if (currentFrame == globalConfiguration.rewindStartFrame) {
                rewindState = NOT_REWINDING;
                delete(myRewinder);
            }

        }

    }

}


/**
 * Operates much like renderFrame(), except it just counts changed pixels instead of
 * rendering the contents. Used exclusively for the COUNT configuration which corresponds
 * to Perfect Pixel Latching AKA the Ideal Pixel Bridge Mode.
 */
void countChangedPixels() {

    static int framesDecoded = 0;
    static int framesCounted = 0;
    static int pixelsDiff = 0;

    if (globalConfiguration.maxFrames && (totalUpdates >= globalConfiguration.maxFrames)) {
        finished = true;
    } else {
        int pixel_count = myPlayer->width() * myPlayer->height();
#ifdef USE_ASYNC_DECODER
        // Get a decoded frame
        // TODO(CDE): Don't poll. Use signals instead.
        while (bufferQueue.size() == 0) {
            usleep(5);
        }
        if ((videoBuffer = bufferQueue.front()) != NULL) {

            pthread_mutex_lock(&bufferQueueMutex);
            bufferQueue.pop();
            pthread_mutex_unlock(&bufferQueueMutex);
#else
            // Decode a frame
            if ((videoBuffer = myPlayer->decodeFrame()) != NULL) {
#endif
            framesDecoded++;

            if (framesDecoded > globalConfiguration.startFrame) {
                // If we're in a rewind period, then we'll be incrementing diffs as well as frame count by 3 instead of 1
                int inc;
                if ( (framesDecoded > (globalConfiguration.rewindStartFrame - globalConfiguration.rewindFrames)) && (framesDecoded <= globalConfiguration.rewindStartFrame) ) {
                    inc = 3;
                } else {
                    inc = 1;
                }

                // If we haven't previously decoded a frame
                if (lastBuffer == NULL) {
                    // Allocate the lastBuffer and copy the decode frame into it.
                    lastBuffer = (uint8_t*)malloc(VIDEO_PIXEL_SIZE * pixel_count);
                    memcpy(lastBuffer, videoBuffer, VIDEO_PIXEL_SIZE * pixel_count);

                    // Otherwise compare the newly decode frame to the previous frame, pixel by pixel
                } else {
                    // Count the changed pixels
                    for (int p = 0; p < pixel_count; p++) {
                        int offset = p * 3;
                        if ((lastBuffer[offset + 0] != videoBuffer[offset + 0]) ||
                            (lastBuffer[offset + 1] != videoBuffer[offset + 1]) ||
                            (lastBuffer[offset + 2] != videoBuffer[offset + 2])) {
                            pixelsDiff+= inc;
                        }
                    }

                    // Then copy to the lastBuffer
                    memcpy(lastBuffer, videoBuffer, VIDEO_PIXEL_SIZE * pixel_count);
                }

                // Update the totalUpdates
                totalUpdates += inc;
                framesCounted += inc;
            }
        }
    }
    if (finished) {
        cout << "CountCSV" << endl;
        cout << "FramesDecoded,FramesCounted,PixelsDifferent" << endl;
        cout << framesDecoded << "," << framesCounted << "," << pixelsDiff << endl;
    }
}

/**
 * Used to compute the optical flow between two frames and to report.
 */
void computeFlow() {
    static int framesDecoded = 0;
    static std::vector<cv::Point2f> features, featuresLast;
#ifdef NORMALIZE_FLOW_FOR_RESOLUTION
    static size_t diagonal = sqrt(displayWidth * displayWidth + displayHeight * displayHeight);
#endif

    if (globalConfiguration.maxFrames && (totalUpdates >= globalConfiguration.maxFrames)) {
        finished = true;
    } else {
        int pixel_count = myPlayer->width() * myPlayer->height();
#ifdef USE_ASYNC_DECODER
        // Get a decoded frame
        // TODO(CDE): Don't poll. Use signals instead.
        while (bufferQueue.size() == 0) {
            usleep(5);
        }
        if ((videoBuffer = bufferQueue.front()) != NULL) {

            pthread_mutex_lock(&bufferQueueMutex);
            bufferQueue.pop();
            pthread_mutex_unlock(&bufferQueueMutex);
#else
            // Decode a frame
            if ((videoBuffer = myPlayer->decodeFrame()) != NULL) {
#endif
            framesDecoded++;

            if (framesDecoded > globalConfiguration.startFrame) {
                // If we're in a rewind period, then we'll be incrementing diffs as well as frame count by 3 instead of 1
                int inc;
                if ( (framesDecoded > (globalConfiguration.rewindStartFrame - globalConfiguration.rewindFrames)) && (framesDecoded <= globalConfiguration.rewindStartFrame) ) {
                    inc = 3;
                } else {
                    inc = 1;
                }

                // If we haven't previously decoded a frame
                if (lastBuffer == NULL) {
#if 0 // Normally you pick points once, and they're tracked throughout. However, they dwindle over time
      // so I'm opting to just find them every frame below instead of here.
                    // Convert the image buffers to OpenCV Mats
                    cv::Mat imgCurrent(displayHeight, displayWidth , CV_8UC3, videoBuffer);

                    // Then convert to grayscale
                    cv::Mat grayCurrent;
                    cv::cvtColor(imgCurrent, grayCurrent, CV_RGB2GRAY);

                    // Then find the features to track
                    cv::goodFeaturesToTrack(grayCurrent, featuresLast,
                                            500,  // the maximum number of features
                                            0.01, // quality level (top 1%)
                                            10);  // min distance between two features
#endif

                    // Allocate the lastBuffer and copy the decode frame into it.
                    lastBuffer = (uint8_t*)malloc(VIDEO_PIXEL_SIZE * pixel_count);
                    memcpy(lastBuffer, videoBuffer, VIDEO_PIXEL_SIZE * pixel_count);

                // Otherwise compute flow between the newly decoded frame to the previous frame
                } else {

                    // Convert the image buffers to OpenCV Mats
                    cv::Mat imgCurrent(displayHeight, displayWidth , CV_8UC3, videoBuffer);
                    cv::Mat imgLast(displayHeight, displayWidth , CV_8UC3, lastBuffer);

                    // Then convert to grayscale
                    cv::Mat grayCurrent, grayLast;
                    cv::cvtColor(imgCurrent, grayCurrent, CV_RGB2GRAY);
                    cv::cvtColor(imgLast, grayLast, CV_RGB2GRAY);

                    // Then find the features to track
                    cv::goodFeaturesToTrack(grayLast, featuresLast,
                                            500,  // the maximum number of features
                                            0.01, // quality level (top 1%)
                                            10);  // min distance between two features

                    vector<uchar> status;
                    vector<float> err;
                    cv::calcOpticalFlowPyrLK(grayLast, grayCurrent,
                                             featuresLast, // input point positions in previous image
                                             features,     // output point positions for the current
                                             status,       // tracking success
                                             err);         // tracking error

                    float dist;
                    size_t count = 0;
                    for (size_t i = 0; i < featuresLast.size(); i++) {
                        if (status[i]) {
                            dist += sqrt((features[i].x - featuresLast[i].x) * (features[i].x - featuresLast[i].x) +
                                         (features[i].y - featuresLast[i].y) * (features[i].y - featuresLast[i].y));
                            count++;
                        }
                    }
                    //cout << count << endl;
                    featuresLast.resize(count);
#ifdef NORMALIZE_FLOW_FOR_RESOLUTION
                    dist = dist / ((float)count * diagonal);
#else
                    dist = dist / (float)count;
#endif

                    cout << "FlowCSV," << framesDecoded - 1 << "," << framesDecoded << "," << dist << endl;

                    // Then copy to the lastBuffer
                    memcpy(lastBuffer, videoBuffer, VIDEO_PIXEL_SIZE * pixel_count);
                }

                // Update the totalUpdates
                totalUpdates += inc;
            }
        }
    }
}


static bool doing_it = false;


void showUsage() {
    cout << "pixelbridge [--mode <fb|flat|cache|dct|count|flow>] [--ts <n> <n>] [--tc <n>] [--bits <1-8>]" << endl <<
            "            [--dctscales x:y[,x:y...]] [--dctdelta <n>] [--dctplanes <n>] [--dctbudget <n>] [--dctsnap] [--dcttrim] [--quality <0/1-100>]" << endl <<
            "            [--start <n>] [--frames <n>] [--rewind <n> <n>] [--verbose] [--csv | -- record <record-filename>] <filename>" << endl <<
            "            [--subregion <x> <y> <width> <height>] [--scale <n>]" << endl;
    cout << endl;
    cout << "  --mode  Configure NDDI as a framebuffer (fb), as a flat tile array (flat), as a cached tile (cache), using DCT (dct), or using IT (it).\n" <<
            "          Optional the mode can be set to count the number of pixels changed (count) or determine optical flow (flow)." << endl;
    cout << "  --ts  Sets the tile size to the width and height provided." << endl;
    cout << "  --tc  Sets the maximum number of tiles in the cache." << endl;
    cout << "  --bits  Sets the number of significant bits per channel when computing checksums." << endl;
    cout << "  --dctscales  For dct mode, this will set a series of scales in the form of comma-separated two tuples holding the scale and then\n" <<
            "               the edge length of a square that determines the number of planes used (i.e. 2 -> 2 x 2 = 4 planes." << endl;
    cout << "  --dctdelta  For dct mode, this is the delta between coefficients that is considered a match which will not be updated.\n" <<
            "              Setting to zero when using a budget will use an optimal setting." << endl;
    cout << "  --dctplanes  For dct mode, this is the number of planes either zeroed or trimmed.\n" <<
            "               Setting to zero when using a budget will use an optimal setting." << endl;
    cout << "  --dctbudget  For dct mode, this is budget in bytes for the transmission of each frame." << endl;
    cout << "  --dctsnap  For dct mode, this turns on snap to zero using either planes or delta." << endl;
    cout << "  --dcttrim  For dct mode, this turns on lossy trimming using either planes or delta." << endl;
    cout << "  --quality  Sets the quality for DCT [1-100] or IT [0-100] mode." << endl;
    cout << "  --start  Will start with this frame, ignoring any decoded frames prior to it." << endl;
    cout << "  --frames  Sets the number of maximum frames that are decoded." << endl;
    cout << "  --rewind  Sets a start point and a number of frames to play in reverse. Once finished, normal playback continues." << endl;
    cout << "  --verbose  Outputs frame-by-frame statistics." << endl;
    cout << "  --csv  Outputs CSV data." << endl;
    cout << "  --record  Records the NDDI commands to the file specified." << endl;
    cout << "  --subregion  Used to indicate which subregion of the display this client renders to when it's configured as one of several slaves." << endl;
    cout << "  --scale  The output is scaled by <n> in both directions. n can be 1, 2, 4, 8,..." << endl;
}


bool parseArgs(int argc, char *argv[]) {
    argc--;
    argv++;

    while (argc) {
        if (strcmp(*argv, "--mode") == 0) {
            argc--;
            argv++;
            if (strcmp(*argv, "fb") == 0) {
                globalConfiguration.tiler = SIMPLE;
            } else if (strcmp(*argv, "flat") == 0) {
                globalConfiguration.tiler = FLAT;
            } else if (strcmp(*argv, "cache") == 0) {
                globalConfiguration.tiler = CACHE;
            } else if (strcmp(*argv, "dct") == 0) {
                globalConfiguration.tiler = DCT;
            } else if (strcmp(*argv, "it") == 0) {
                globalConfiguration.tiler = IT;
            } else if (strcmp(*argv, "count") == 0) {
                globalConfiguration.tiler = COUNT;
            } else if (strcmp(*argv, "flow") == 0) {
                globalConfiguration.tiler = FLOW;
            } else {
                showUsage();
                return false;
            }
            argc--;
            argv++;
        } else if (strcmp(*argv, "--dctscales") == 0) {
            if (globalConfiguration.tiler != DCT) {
                showUsage();
                return false;
            }
            argc--;
            argv++;
            // Clear the default dct scales
            globalConfiguration.clearDctScales();
            // Then pull out each tuple x:y
            char *p = strtok(*argv, ",");
            size_t first = 0;
            while (p != NULL) {
                // Scan the scale and count
                int scale, edge;
                sscanf(p, "%d:%d", &scale, &edge);
                if (edge < 0 || edge > 8) {
                    showUsage();
                    return false;
                }
                // Add the dct scale
                globalConfiguration.addDctScale(scale, first, edge);
                first += edge * edge;
                // Move to next tuple
                p = strtok(NULL, ",");
            }
            argc--;
            argv++;
        } else if (strcmp(*argv, "--dctdelta") == 0) {
            argc--;
            argv++;
            if (globalConfiguration.tiler != DCT) {
                showUsage();
                return false;
            }
            globalConfiguration.dctDelta = atoi(*argv);
            argc--;
            argv++;
        } else if (strcmp(*argv, "--dctplanes") == 0) {
            argc--;
            argv++;
            if (globalConfiguration.tiler != DCT) {
                showUsage();
                return false;
            }
            globalConfiguration.dctPlanes = atoi(*argv);
            argc--;
            argv++;
        } else if (strcmp(*argv, "--dctbudget") == 0) {
            argc--;
            argv++;
            if (globalConfiguration.tiler != DCT) {
                showUsage();
                return false;
            }
            globalConfiguration.dctBudget = atoi(*argv);
            argc--;
            argv++;
        } else if (strcmp(*argv, "--dctsnap") == 0) {
            argc--;
            argv++;
            if (globalConfiguration.tiler != DCT) {
                showUsage();
                return false;
            }
            globalConfiguration.dctSnap = true;
        } else if (strcmp(*argv, "--dcttrim") == 0) {
            argc--;
            argv++;
            if (globalConfiguration.tiler != DCT) {
                showUsage();
                return false;
            }
            globalConfiguration.dctTrim = true;
        } else if (strcmp(*argv, "--ts") == 0) {
            globalConfiguration.tileWidth = atoi(argv[1]);
            globalConfiguration.tileHeight = atoi(argv[2]);
            if ((globalConfiguration.tileWidth == 0) || (globalConfiguration.tileHeight == 0)) {
                showUsage();
                return false;
            }
            argc -= 3;
            argv += 3;
        } else if (strcmp(*argv, "--tc") == 0) {
            globalConfiguration.maxTiles = atoi(argv[1]);
            if (globalConfiguration.maxTiles == 0) {
                showUsage();
                return false;
            }
            argc -= 2;
            argv += 2;
        } else if (strcmp(*argv, "--bits") == 0) {
            globalConfiguration.sigBits = atoi(argv[1]);
            if ( (globalConfiguration.sigBits == 0) || (globalConfiguration.sigBits > 8) ) {
                showUsage();
                return false;
            }
            argc -= 2;
            argv += 2;
        } else if (strcmp(*argv, "--quality") == 0) {
            globalConfiguration.quality = atoi(argv[1]);
            if ( (globalConfiguration.quality == 0) || (globalConfiguration.quality > 100) ) {
                showUsage();
                return false;
            }
            argc -= 2;
            argv += 2;
        } else if (strcmp(*argv, "--start") == 0) {
            globalConfiguration.startFrame = atoi(argv[1]);
            if (globalConfiguration.startFrame == 0) {
                showUsage();
                return false;
            }
            argc -= 2;
            argv += 2;
        } else if (strcmp(*argv, "--frames") == 0) {
            globalConfiguration.maxFrames = atoi(argv[1]);
            if (globalConfiguration.maxFrames == 0) {
                showUsage();
                return false;
            }
            argc -= 2;
            argv += 2;
        } else if (strcmp(*argv, "--rewind") == 0) {
            globalConfiguration.rewindStartFrame = atoi(argv[1]);
            globalConfiguration.rewindFrames = atoi(argv[2]);
            if ((globalConfiguration.rewindStartFrame == 0) || (globalConfiguration.rewindFrames == 0) || (globalConfiguration.rewindFrames > globalConfiguration.rewindStartFrame)) {
                showUsage();
                return false;
            }
            argc -= 3;
            argv += 3;
        } else if (strcmp(*argv, "--verbose") == 0) {
            globalConfiguration.verbose = true;
            argc--;
            argv++;
        } else if (strcmp(*argv, "--csv") == 0) {
            globalConfiguration.csv = true;
            argc--;
            argv++;
        } else if (strcmp(*argv, "--record") == 0) {
            globalConfiguration.recordFile = argv[1];
            argc -= 2;
            argv += 2;
        } else if (strcmp(*argv, "--subregion") == 0) {
            globalConfiguration.isSlave = true;
            globalConfiguration.sub_x = atoi(argv[1]);
            globalConfiguration.sub_y = atoi(argv[2]);
            globalConfiguration.sub_w = atoi(argv[3]);
            globalConfiguration.sub_h = atoi(argv[4]);
            argc -= 5;
            argv += 5;
        } else if (strcmp(*argv, "--scale") == 0) {
            globalConfiguration.scale = atoi(argv[1]);
            argc -= 2;
            argv += 2;
        } else {
            fileName = *argv;
            argc--;
            argv++;
        }
    }

    if (!fileName) {
        showUsage();
        return false;
    } else {
        return true;
    }
}


#ifdef USE_ASYNC_DECODER
void* decoderRun(void* none) {
    uint8_t* buf;

    while ((buf = myPlayer->decodeFrame()) != NULL) {
        // TODO(CDE): Don't poll. Use signals instead.
        while (bufferQueue.size() >= 20) {
            usleep(50);
        }
        pthread_mutex_lock(&bufferQueueMutex);
        bufferQueue.push(buf);
        pthread_mutex_unlock(&bufferQueueMutex);
    }
    // Push a final NULL to trigger the end of stream
    bufferQueue.push(NULL);

    return NULL;
}
#endif


int main(int argc, char *argv[]) {

    // Parse command line arguments
    if (!parseArgs(argc, argv)) {
        return -1;
    }

    // Initialize ffmpeg and set dimensions
#ifndef USE_RANDOM_PLAYER
    myPlayer = (Player*)new FfmpegPlayer(fileName);
#else
    myPlayer = (Player*)new RandomPlayer();
#endif
#ifndef USE_SMALLER_WINDOW
    displayWidth = myPlayer->width();
    displayHeight = myPlayer->height();
#endif

    if (!globalConfiguration.isSlave) {
        globalConfiguration.sub_x = 0;
        globalConfiguration.sub_y = 0;
        globalConfiguration.sub_w = displayWidth * globalConfiguration.scale;
        globalConfiguration.sub_h = displayHeight * globalConfiguration.scale;
    }

    if (globalConfiguration.rewindStartFrame > 0) {
        myRewinder = new Rewinder(globalConfiguration.rewindStartFrame - globalConfiguration.rewindFrames, displayWidth, displayHeight);
    }
    if ( (displayWidth == 0) || (displayHeight == 0) ) {
        cerr << "Error: Could not get video dimensions." << endl;
        return -1;
    }

#ifdef USE_ASYNC_DECODER
    // Start decoded thread
    pthread_create(&decoderThread, NULL, decoderRun, NULL);
#endif

    // Setup NDDI Display
    if (globalConfiguration.tiler > COUNT) {

        // Force tilesize to be 8x8 for DCT since we're using 8x8 macroblocks
        if (globalConfiguration.tiler == DCT) {
            globalConfiguration.tileWidth = globalConfiguration.tileHeight = 8;
        // Else if the tile size wasn't specified, then dynamically calculate it
        } else if ((globalConfiguration.tileWidth == 0) || (globalConfiguration.tileHeight == 0)) {
            // Calculate the tile as a square who's edge is 1/40 of the longest display dimension
            size_t edge = ((displayWidth > displayHeight) ? displayWidth : displayHeight) / 40;
            // But don't go smaller than 8x8 when automatically calculating the tile size
            if (edge < 8)
                edge = 8;
            globalConfiguration.tileWidth = globalConfiguration.tileHeight = edge;
        }

        // Setup the Nddi Display and Tiler if required
        setupDisplay();
    }

    // Take the start time stamp
    gettimeofday(&startTime, NULL);

    // Run main loop
    while (!finished) {
        if (globalConfiguration.tiler > COUNT) {
            renderFrame();
        } else if (globalConfiguration.tiler == COUNT){
            countChangedPixels();
        } else if (globalConfiguration.tiler == FLOW){
            computeFlow();
        }
    }

    if (myPlayer) { delete myPlayer; }
    if (myDisplay) {
        if (globalConfiguration.recordFile.length()) {
            if (!globalConfiguration.isSlave) {
                ((RecorderNddiDisplay*)myDisplay)->Shutdown();
            }
            delete (RecorderNddiDisplay*)myDisplay;
        } else {
            if (!globalConfiguration.isSlave) {
                ((GrpcNddiDisplay*)myDisplay)->Shutdown();
            }
            delete (GrpcNddiDisplay*)myDisplay;
        }
    }
    if (myTiler) { delete myTiler; }
    if (myRewinder) { delete myRewinder; }

    return 0;
}
