#include <cmath>
#include <iostream>

#include "PixelBridgeFeatures.h"
#include "Configuration.h"
#include "DctTiler.h"

#define PI    3.14159265
#define PI_8  0.392699081

// For each scale, determines the optimal amount of planes to zero out.
//#define OPTIMAL_ZEROING

// This will turn on the simple truncation used before that simply takes the top X coefficients.
// The new scheme will instead take the most significant coefficients in a square pattern.
//#define SIMPLE_TRUNCATION

// This uses the scale to determine how often to send the coefficients at scales greater than 1x.
// e.g. The 16x scale coefficients are only sent every 16th frame. The 4x every 4th frame.
//#define SKIP_FRAMES


/*
 * Frame Volume
 *
 * Dimensions are 8 x 8 x 193 and holds the 64 basis function for the red, green, and then blue channels and then
 * one extra plane for medium gray. The pre-rendered basis functions are arranged in the frame volume
 * using zig-zag ordering, with the colors interleaved.
 *
 *      0      1      2      3      4      5      6      7      8      9     10     11
 * R(0,0) G(0,0) B(0,0) R(1,0) G(1,0) B(1,0) R(0,1) G(0,1) B(0,1) R(0,2) G(0,2) B(0,2)
 *     12     13     14     15     16     17     18     19     20     21     22     23
 * R(1,1) G(1,1) B(1,1) R(2,0) G(2,0) B(2,0) R(3,0) G(3,0) B(3,0) R(2,1) G(2,1) B(2,1)
 *   ...
 *    189    190    191
 * R(7,7) G(7,7) B(7,7)
 *
 *
 * Coefficient Plane
 *
 * The Coefficient Planes are then arranged to match, where each coefficient matrix in planes 0 - 191 correspond
 * to the same plane in the Frame Volume with the proper translation values. The last plane (192) picks the
 * extra medium medium gray plane from the frame volume.
 *
 * The scalers for each 8x8 block of coefficient matrices are set per macro block using the DCT coefficients
 * calculated for that macro block. The coefficients are of course in zig-zag order, and so an efficient fill
 * mechanism can be use to pass the non-zero coefficients down the "stack" of 8x8 coefficient planes regions.
 * The scaler for the final medium gray plane is set to full (256) as a way of adding back the 128 to each
 * channel as the final step of this NDDI-based IDCT.
 */

/*
 * Using a #define because it's used for floats and ints.
 */
#define CLAMP(x, min, max)   (x < min ? min : x >= max ? max : x)
#define CEIL(x, y)           (1 + ((x - 1) / y))
#define SQRT_125             0.353553391
#define SQRT_250             0.5


/**
 * The DctTiler is created based on the dimensions of the NDDI display that's passed in. If those
 * dimensions change, then the DctTiler should be destroyed and re-created.
 */
DctTiler::DctTiler (size_t display_width, size_t display_height,
                    size_t quality, string file)
: display_width_(display_width),
  display_height_(display_height),
  scale_(1)
{
    quiet_ = !globalConfiguration.verbose;

#ifdef USE_RAM_SAVING_COEFFICIENT_PLANE_FEATURES
    #ifdef USE_CL
    #error "OpenCL implementation doesn't support the RAM saving features."
    #endif
    saveRam_ = true;
#else
    saveRam_ = false;
#endif

    /*
     * Scale the display size members, leaving the display size input parameters
     * untouched for the tile calculations below.
     */
    if (globalConfiguration.scale > 1) {
        scale_ = globalConfiguration.scale;
        display_width_ *= scale_;
        display_height_ *= scale_;
    }
    scaled_block_width_ = BLOCK_WIDTH * scale_;
    scaled_block_height_ = BLOCK_HEIGHT * scale_;
    scaled_block_size_ = BLOCK_SIZE * scale_ * scale_;
    fv_tx_offset_ = 0;
    for (size_t s = 1; s < scale_; s <<= 1) { fv_tx_offset_ += BLOCK_WIDTH * s; }

    /*
     *  3 dimensional matching the Macroblock Width x Height x 64.
     *  If we're scaling, then the other stacks are arranged along
     *  the x direction of the frame volume.
     */
    vector<unsigned int> fvDimensions;
    fvDimensions.push_back(fv_tx_offset_ + scaled_block_width_);
    fvDimensions.push_back(scaled_block_height_);
    fvDimensions.push_back(FRAMEVOLUME_DEPTH);

    /*
     * Pre-calculate the number of tiles used for the display and initialize
     * the tile stack heights that are used to efficiently update the scalers
     * making sure to not update more than needed.
     */
    displayTilesWide_ = CEIL(display_width, BLOCK_WIDTH);
    displayTilesHigh_ = CEIL(display_height, BLOCK_HEIGHT);
    tileStackHeights_ = (uint8_t*)calloc(displayTilesWide_ * displayTilesHigh_, sizeof(uint8_t));

    if (file.length()) {
        display_ = new RecorderNddiDisplay(fvDimensions,
                display_width_, display_height_,
                (unsigned int)FRAMEVOLUME_DEPTH,
                (unsigned int)3,
                file,
                saveRam_, saveRam_);
    } else {
        if (!globalConfiguration.isSlave) {
            display_ = new GrpcNddiDisplay(fvDimensions,
                    display_width_, display_height_,
                    (unsigned int)FRAMEVOLUME_DEPTH,
                    (unsigned int)3,
                    saveRam_, saveRam_);
        } else {
            display_ = new GrpcNddiDisplay();
        }
    }

    /* Set the full scaler value and the sign mode */
    display_->SetFullScaler(MAX_DCT_COEFF);
    display_->SetPixelByteSignMode(SIGNED_MODE);

    /*
     * Initialize the zig-zag order used throughout and the calculate
     * the quantization matrix
     */
    initZigZag();
    initQuantizationMatrix(quality);

    /* Initialize Input Vector */
    vector<int> iv;
    iv.push_back(1);
    display_->UpdateInputVector(iv);

    /* Initialize Coefficient Planes */
    InitializeCoefficientPlanes();

    /* Initialize Frame Volume */
    InitializeFrameVolume();
}

/**
 * Returns the Display created and initialized by the tiler.
 */
NDimensionalDisplayInterface* DctTiler::GetDisplay() {
    return display_;
}

/*
 * A particular 8x8 macroblock has pixels index by x,y with the origin at the top left:
 *
 *   0,0 1,0 2,0 3,0 4,0 5,0 6,0 7,0
 *   0,1 1,1 2,1 3,1 4,1 5,1 6,1 7,1
 *   0,2 1,2 2,2 3,2 4,2 5,2 6,2 7,2
 *   0,3 1,3 2,3 3,3 4,3 5,3 6,3 7,3
 *   0,4 1,4 2,4 3,4 4,4 5,4 6,4 7,4
 *   0,5 1,5 2,5 3,5 4,5 5,5 6,5 7,5
 *   0,6 1,6 2,6 3,6 4,6 5,6 6,6 7,6
 *   0,7 1,7 2,7 3,7 4,7 5,7 6,7 7,7
 *
 * Zig-Zag order is therefore:
 *
 *                 0,0
 *               1,0 0,1
 *             0,2 1,1 2,0
 *           3,0 2,1 1,2 0,3
 *         0,4 1,3 2,2 3,1 4,0
 *       5,0 4,1 3,2 2,3 1,4 0,5
 *     0,6 1,5 2,4 3,3 4,2 5,1 6,0
 *   7,0 6,1 5,2 4,3 3,4 2,5 1,6 0,7
 *     1,7 2,6 3,5 4,4 5,3 6,2 7,1
 *       7,2 6,3 5,4 4,5 3,6 2,7
 *         3,7 4,6 5,5 6,4 7,3
 *           7,4 6,5 5,6 4,7
 *             5,7 6,6 7,5
 *               7,6 6,7
 *                 7,7
 *
 * This zig-zag ordering is then stored in the zigZag_ array. Each (x,y) pair
 * is used as a unique index into the array by multiplying the pair in a row-major
 * manner (y*8+x). Then the number of the corresponding frame volume frame is stored.
 * Specifically, each plane in the frame volume holds a rendering of a basis function
 * and each of the color channels has its own rendering. So the value stored at the
 * particular entry in the zigZag_ array is actually a logical value that must be
 * multiplied by 3 and then the proper frames for that color are chosen independently.
 */
void DctTiler::initZigZag() {
    size_t  x = 0, y = 0;
    bool    up = true;

    // Setup the zigZag_ table
    for (size_t i = 0; i < BLOCK_SIZE; i++) {
        zigZag_[y * BLOCK_WIDTH + x] = i;
        if (up) {
            if (x < (BLOCK_WIDTH - 1)) {
                x++;
                if (y > 0) {
                    y--;
                } else {
                    up = false;
                }
            } else {
                y++;
                up = false;
            }
        } else {
            if (y < (BLOCK_HEIGHT - 1)) {
                y++;
                if (x > 0) {
                    x--;
                } else {
                    up = true;
                }
            } else {
                x++;
                up = true;
            }
        }
    }
}

/*
 * Uses simple algorithm from M. Nelson, "The Data Compression Book," San Mateo, CA, M&T Books, 1992.
 */
void DctTiler::initQuantizationMatrix(size_t quality) {
    for (size_t v = 0; v < BLOCK_HEIGHT; v++) {
        for (size_t u = 0; u < BLOCK_WIDTH; u++) {
            quantizationMatrix_[v * BLOCK_WIDTH + u] = 1 + (1 + u + v) * quality;
        }
    }
}

/**
 * Initializes the Coefficient Planes for this tiler. The coefficient matrices
 * for each plane will pick a plane from the frame volume.
 */
void DctTiler::InitializeCoefficientPlanes() {

    Scaler s;

    // Setup the coefficient matrix to complete 3x3 identity initially
    vector< vector<int> > coeffs;
    coeffs.resize(3);
    coeffs[0].push_back(1); coeffs[0].push_back(0); coeffs[0].push_back(0);
    coeffs[1].push_back(0); coeffs[1].push_back(1); coeffs[1].push_back(0);
    coeffs[2].push_back(0); coeffs[2].push_back(0); coeffs[2].push_back(0);

    // Setup start and end points to (0,0,0) initially
    vector<unsigned int> start, end;
    start.push_back(0); start.push_back(0); start.push_back(0);
    end.push_back(0); end.push_back(0); end.push_back(0);

    // Break the display into macroblocks and initialize each cube of coefficients to pick out the proper block from the frame volume
    for (int j = 0; j < displayTilesHigh_; j++) {
        for (int i = 0; i < displayTilesWide_; i++) {
            coeffs[2][0] = -i * scaled_block_width_ + fv_tx_offset_;
            coeffs[2][1] = -j * scaled_block_height_;
            start[0] = i * scaled_block_width_; start[1] = j * scaled_block_height_; start[2] = 0;
            end[0] = (i + 1) * scaled_block_width_ - 1; end[1] = (j + 1) * scaled_block_height_ - 1; end[2] = FRAMEVOLUME_DEPTH - 1;
            end[2] = FRAMEVOLUME_DEPTH - 1;
            if (end[0] >= display_width_) { end[0] = display_width_ - 1; }
            if (end[1] >= display_height_) { end[1] = display_height_ - 1; }
            if (globalConfiguration.isSlave) {
                start[0] += globalConfiguration.sub_x;
                start[1] += globalConfiguration.sub_y;
                end[0] += globalConfiguration.sub_x;
                end[1] += globalConfiguration.sub_y;
                coeffs[2][0] -= globalConfiguration.sub_x;
                coeffs[2][1] -= globalConfiguration.sub_y;
            }
            display_->FillCoefficientMatrix(coeffs, start, end);
        }
    }
    // Finish up by setting the proper k for every plane
    start[0] = 0; start[1] = 0;
    end[0] = display_width_ - 1; end[1] = display_height_ - 1;
    if (globalConfiguration.isSlave) {
        start[0] += globalConfiguration.sub_x;
        start[1] += globalConfiguration.sub_y;
        end[0] += globalConfiguration.sub_x;
        end[1] += globalConfiguration.sub_y;
    }
    for (int k = 0; k < FRAMEVOLUME_DEPTH; k++) {
        start[2] = k; end[2] = k;
        if (saveRam_) {
            display_->FillCoefficient(COEFFICIENT_MATRIX_P, 2, 2, start, end);
        } else {
            display_->FillCoefficient(k, 2, 2, start, end);
        }
    }

    // Fill each scaler in every plane with 0
    start[0] = 0; start[1] = 0; start[2] = 0;
    end[0] = display_width_ - 1;
    end[1] = display_height_ - 1;
    end[2] = display_->NumCoefficientPlanes() - 1;
    if (globalConfiguration.isSlave) {
        start[0] += globalConfiguration.sub_x;
        start[1] += globalConfiguration.sub_y;
        end[0] += globalConfiguration.sub_x;
        end[1] += globalConfiguration.sub_y;
    }
    s.packed = 0;
    display_->FillScaler(s, start, end);

    // Fill the scalers for the medium gray plane to full on
    start[2] = display_->NumCoefficientPlanes() - 1;
    s.r = s.g = s.b = display_->GetFullScaler();
    display_->FillScaler(s, start, end);
}


/**
 * Initializes the Frame Volume for this tiler by pre-rendering each
 * of the 64 basis functions into 8x8 planes in the Frame Volume. They're
 * rendered for each color channel and stored in those groups of three in
 * zig-zag order. If scaling, then still only 64 basis functions are rendered,
 * but they'll be stretched into scaled planes. e.g. 16x16, 32x32, etc.
 */
void DctTiler::InitializeFrameVolume() {

    basisFunctions_ = (Pixel *)calloc(scaled_block_size_ * FRAMEVOLUME_DEPTH, sizeof(Pixel));

    // Pre-render each basis function
#ifdef USE_OMP
#pragma omp parallel for
#endif
    for (int j = 0; j < BASIS_BLOCKS_TALL; j++) {
        for (int i = 0; i < BASIS_BLOCKS_WIDE; i++) {

            // Don't process the final basis block.
            if (i == BASIS_BLOCKS_WIDE - 1 && j == BASIS_BLOCKS_TALL) continue;

            size_t p = zigZag_[j * BASIS_BLOCKS_WIDE + i] * scaled_block_size_;

            for (int y = 0; y < BLOCK_HEIGHT; y++) {
                for (int x = 0; x < BLOCK_WIDTH; x++) {
                    double m = 0.0;
                    bool neg = false;

                    for (int v = 0; v < BLOCK_HEIGHT; v++) {
                        for (int u = 0; u < BLOCK_WIDTH; u++) {
                            double px = 1.0;
                            px *= (u == 0) ? SQRT_125 : SQRT_250;   // alpha(u)
                            px *= (v == 0) ? SQRT_125 : SQRT_250;   // alpha(v)
                            px *= ((u == i) && (v == j)) ? double(MAX_DCT_COEFF) : 0.0;  // DCT coefficient (maximum on or off)
                            px *= cos(PI_8 * ((double)x + 0.5) * (double)u);         // cos with x, u
                            px *= cos(PI_8 * ((double)y + 0.5) * (double)v);         // cos with y, v
                            m += px;
                        }
                    }

                    // Set the neg flag if the magnitude is negative and then remove the sign from the magnitude
                    if (m < 0.0f) {
                        neg = true;
                        m *= -1.0;
                    }

                    // Clamp to 127 and cast to byte and convert to two's complement if necessary
                    unsigned int c = (unsigned char)CLAMP(m, 0.0, 127.0);
                    if (neg) {
                        c = ~c + 1;
                    }

                    for (size_t sy = 0; sy < scale_; sy++) {
                        for (size_t sx = 0; sx < scale_; sx++) {
                            // Set the color channels with the magnitude clamped to 127
                            basisFunctions_[p + sx + sy * scaled_block_width_].r = c;
                            basisFunctions_[p + sx + sy * scaled_block_width_].g = c;
                            basisFunctions_[p + sx + sy * scaled_block_width_].b = c;
                            basisFunctions_[p + sx + sy * scaled_block_width_].a = 0xff;
                        }
                    }
                    // Move to the next pixel
                    p += scale_;
                }
                p += scaled_block_width_ * (scale_ - 1);
            }
        }
    }

    // Then render the gray block as the actual last basis block
    size_t p = zigZag_[BASIS_BLOCKS_WIDE * BASIS_BLOCKS_TALL - 1] * scaled_block_size_;
    for (int y = 0; y < scaled_block_height_; y++) {
        for (int x = 0; x < scaled_block_width_; x++) {
            unsigned int c = 0x7f;
            basisFunctions_[p].r = c;
            basisFunctions_[p].g = c;
            basisFunctions_[p].b = c;
            basisFunctions_[p].a = 0xff;
            p++;
        }
    }

    // Update the frame volume with the basis function renderings and gray block in bulk.
    vector<unsigned int> start, end;
    start.push_back(fv_tx_offset_); start.push_back(0); start.push_back(0);
    end.push_back(fv_tx_offset_ + scaled_block_width_ - 1); end.push_back(scaled_block_height_ - 1); end.push_back(FRAMEVOLUME_DEPTH - 1);
    display_->CopyPixels(basisFunctions_, start, end);
}

/**
 * Update the display by calculating the DCT coefficients for each macroblock
 * and updating the coefficient plane scalers.
 *
 * @param buffer Pointer to an RGB buffer
 * @param width The width of the RGB buffer
 * @param height The height of the RGB buffer
 */
void DctTiler::UpdateDisplay(uint8_t* buffer, size_t width, size_t height)
{
    vector<unsigned int> size(2, 0);
    Scaler s;

    assert(width * scale_ >= display_width_);
    assert(height * scale_ >= display_height_);

    size[0] = scaled_block_width_;
    size[1] = scaled_block_height_;

    /*
     * Produces the de-quantized coefficients for the input buffer using the following steps:
     *
     * 1. Shift by subtracting 128
     * 2. Take the 2D DCT
     * 3. Quantize
     * 4. De-quantize
     */
#ifdef USE_OMP
#pragma omp parallel for ordered
#endif
    for (size_t j = 0; j < displayTilesHigh_; j++) {
        for (size_t i = 0; i < displayTilesWide_; i++) {

            vector<unsigned int> start(3, 0);

            /* The coefficients are stored in this array in zig-zag order */
            vector<uint64_t> coefficients(BLOCK_SIZE, 0);

            for (size_t v = 0; v < BLOCK_HEIGHT; v++) {
                for (size_t u = 0; u < BLOCK_WIDTH; u++) {

                    double c_r = 0.0, c_g = 0.0, c_b = 0.0;

                    /* Calculate G for each u, v using g(x,y) shifted (1. and 2.) */
                    for (size_t y = 0; y < BLOCK_HEIGHT; y++) {
                        size_t bufPos = ((j * BLOCK_HEIGHT + y) * width + i * BLOCK_WIDTH + 0) * 3;
                        for (size_t x = 0; x < BLOCK_WIDTH; x++) {

                            double p = 1.0;

                            p *= (u == 0) ? SQRT_125 : SQRT_250;                                 // alpha(u)
                            p *= (v == 0) ? SQRT_125 : SQRT_250;                                 // alpha(v)
                            p *= cos(PI_8 * ((double)x + 0.5) * (double)u);                      // cos with x, u
                            p *= cos(PI_8 * ((double)y + 0.5) * (double)v);                      // cos with y, v
                            /* Fetch each channel, multiply by product p and then shift */
                            if ( ((i * BLOCK_WIDTH + x) < display_width_)
                                    && ((j * BLOCK_HEIGHT + y) < display_height_) ) {
                                c_r += p * ((double)buffer[bufPos] - 128.0); bufPos++;           // red: g(x,y) - 128
                                c_g += p * ((double)buffer[bufPos] - 128.0); bufPos++;           // green: g(x,y) - 128
                                c_b += p * ((double)buffer[bufPos] - 128.0); bufPos++;           // blue: g(x,y) - 128
                            } else {
                                bufPos += 3;
                            }
                        }
                    }

                    int g_r, g_g, g_b;
                    size_t matPos = v * BLOCK_WIDTH + u;

                    /* Quantize G(u,v) (3.) */
                    g_r = (int)int(c_r / double(quantizationMatrix_[matPos]) + 0.5);
                    g_g = (int)int(c_g / double(quantizationMatrix_[matPos]) + 0.5);
                    g_b = (int)int(c_b / double(quantizationMatrix_[matPos]) + 0.5);

                    /* De-quantized G(u,v) (4.) */
                    g_r *= (int)quantizationMatrix_[matPos];
                    g_g *= (int)quantizationMatrix_[matPos];
                    g_b *= (int)quantizationMatrix_[matPos];

                    /* Set the coefficient in zig-zag order. */
                    size_t p = zigZag_[matPos];

                    /* Skip the last block, because we've used it for the medium gray block */
                    if (p == BLOCK_SIZE - 1) continue;

                    /* Build the scaler from the three coefficients */
                    s.packed = 0;
                    s.r = g_r;
                    s.g = g_g;
                    s.b = g_b;
                    coefficients[p] = s.packed;
                }
            }

            /*
             * Determine the minimum height of the stack that needs to be sent and
             * update the coefficients vector size. The minimum stack height is
             * determined by the non-zero planes in this update, but it might be
             * slightly larger if we need to overwrite the larger non-zero planes from
             * the last update. So the stack height is the largest of the old stack height
             * and the current stack height.
             */
            size_t h = BLOCK_SIZE - 2;
            while (h > 0 && coefficients[h] == 0) {
                 h--;
            }
            if (h < tileStackHeights_[j * displayTilesWide_ + i]) {
                coefficients.resize(tileStackHeights_[j * displayTilesWide_ + i] + 1);
            } else {
                coefficients.resize(h + 1);
            }
            tileStackHeights_[j * displayTilesWide_ + i] = h;

            /* Send the NDDI command to update this macroblock's coefficients, one plane at a time. */
            start[0] = i * scaled_block_width_;
            start[1] = j * scaled_block_height_;
            if (globalConfiguration.isSlave) {
                start[0] += globalConfiguration.sub_x;
                start[1] += globalConfiguration.sub_y;
            }
#pragma omp critical
            display_->FillScalerTileStack(coefficients, start, size);
        }
    }
}
