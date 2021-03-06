#ifndef SCALED_DCT_TILER_H
#define SCALED_DCT_TILER_H
/*
 *  ScaledDctTiler.h
 *  pixelbridge
 *
 *  Created by Dave Estes on 11/26/10.
 *  Copyright 2010 Dave Estes. All rights reserved.
 *
 */

#include "DctTiler.h"
#include "nddi/BaseNddiDisplay.h"

using namespace nddi;
using namespace std;

/**
 * This tiler will split provided frames into macroblocks and will perform the forward DCT
 * and create coefficients that will be used as the scalers for the coefficient planes.
 */
class ScaledDctTiler : public DctTiler {

public:
    ScaledDctTiler() {}

    /**
     * The ScaledDctTiler is created based on the dimensions of the NDDI display that's passed in. If those
     * dimensions change, then the ScaledDctTiler should be destroyed and re-created.
     *
     * @param display_width The width of the display
     * @param display_height The height of the display
     * @param quality The quality factor used for DCT.
     * @param quiet Used to squelch extra information output.
     */
    ScaledDctTiler(size_t display_width, size_t display_height, size_t quality, string file = "");

    /**
     * Returns the Display created and initialized by the tiler.
     */
    NDimensionalDisplayInterface* GetDisplay();

    /**
     * Update the scalers and then the NDDI display based on the frame that's passed in.
     *
     * @param buffer Pointer to the return frame buffer
     * @param width The width of that frame buffer
     * @param height The height of that frame buffer
     */
    void UpdateDisplay(uint8_t* buffer, size_t width, size_t height);

private:
    int16_t* DownSample(size_t factor, int16_t* buffer, size_t width, size_t height);
    int16_t* UpSample(size_t factor, int16_t* buffer, size_t width, size_t height);

protected:
    void InitializeCoefficientPlanes();
    int16_t* ConvertToSignedPixels(uint8_t* buffer, size_t width, size_t height);
    vector<uint64_t> BuildCoefficients(size_t i, size_t j, int16_t* buffer, size_t width, size_t height, bool adjustPixels);
    void SelectCoefficientsForScale(vector<uint64_t> &coefficients, size_t c);
    size_t EstimateCost(bool isTrim, vector< vector< vector<uint64_t> > > &coefficientsForScale, size_t c, size_t delta, size_t planes);
    void CalculateSnapCoefficientsToZero(vector< vector< vector<uint64_t> > > &coefficientsForScale, size_t c, size_t &delta, size_t &planes);
    void SnapCoefficientsToZero(vector< vector< vector<uint64_t> > > &coefficientsForScale, size_t c, size_t delta, size_t planes);
    void CalculateTrimCoefficients(vector< vector< vector<uint64_t> > > &coefficientsForScale, size_t c, size_t &delta, size_t &planes);
    size_t TrimCoefficients(vector<uint64_t> &coefficients, size_t i, size_t j, size_t c, size_t delta, size_t planes);
#ifdef OPTIMAL_ZEROING
    size_t EstimateCostForZeroingPlanes(vector< vector< vector<uint64_t> > > &coefficientsForScale, size_t c, size_t first, size_t last);
    void ZeroPlanes(vector< vector< vector<uint64_t> > > &coefficientsForScale, size_t c);
#endif
    void FillCoefficients(vector<uint64_t> &coefficients, size_t i, size_t j, size_t c, size_t first);
    void PrerenderCoefficients(vector<uint64_t> &coefficients, size_t i, size_t j, size_t c, int16_t* renderedBuffer, size_t width, size_t height, bool shift);
    void AdjustFrame(int16_t* buffer, int16_t* renderedBuffer, size_t width, size_t height);


private:
    size_t                                          display_width_, display_height_;
    vector< vector< vector< vector<uint64_t> > > >  cachedCoefficients_;
};
#endif // SCALED_DCT_TILER_H
