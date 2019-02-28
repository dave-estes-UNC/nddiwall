#ifndef GRPC_NDDI_DISPLAY_H
#define GRPC_NDDI_DISPLAY_H

/**
 * \file GrpcNddiDisplay.h
 *
 * \brief This file embodies a networked implementation of a networked nDDI display for clients.
 *
 * This file embodies a networked implementation of a networked nDDI display for clients.
 */

#include <grpc++/grpc++.h>

#include "nddi/Features.h"
#include "nddi/NDimensionalDisplayInterface.h"

#include "nddiwall.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using nddiwall::NddiWall;

using namespace std;

namespace nddi {

    /**
     * \brief Implements and NDDI display where each interface is a GRPC call to the NDDI Wall Server.
     *
     * Implements and NDDI display where each interface is a GRPC call to the NDDI Wall Server.
     */
    class GrpcNddiDisplay : public NDimensionalDisplayInterface {

    public:
        /**
         * \brief Required default constructor for abstract class NDimensionalDisplayInterface.
         *
         * Required default constructor for abstract class NDimensionalDisplayInterface.
         */
        GrpcNddiDisplay();

        /**
         * \brief Minimal constructor which uses a fixed display size.
         *
         * Minimal constructor which uses a fixed display size and configures the nDDI display with the provided
         * dimensions for the Input Vector, Coefficient Planes, and Frame Volume. The implementation builds an
	 * Initialize command from the arguments and sends it to the server.
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
        GrpcNddiDisplay(vector<unsigned int> &frameVolumeDimensionalSizes,
			unsigned int numCoefficientPlanes, unsigned int inputVectorSize,
			bool fixed8x8Macroblocks = false, bool useSingleCoeffcientPlane = false);

        /**
         * \brief Full constructor which additionally allows the display size to be configured.
         *
         * Full constructor which additionally allows the display size to be configured. The implementation builds
	 * an Initialize command from the arguments and sends it to the server.
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
	GrpcNddiDisplay(vector<unsigned int> &frameVolumeDimensionalSizes,
			unsigned int displayWidth, unsigned int displayHeight,
			unsigned int numCoefficientPlanes, unsigned int inputVectorSize,
                        bool fixed8x8Macroblocks = false, bool useSingleCoeffcientPlane = false);

	/**
	 * \brief Default destructor.
	 *
	 * Default desctructor. Sends a Shutdown commands to the server.
	 */
	~GrpcNddiDisplay();

        /**
         * \brief Used to query the display width.
         *
         * Used to query the display width. Sends a DisplayWidth command to the server and then
	 * returns the result from the server to the caller.
         * @return The width of the display.
         */
        unsigned int DisplayWidth();

        /**
         * \brief Used to query the display height.
         *
         * Used to query the display height. Sends a DisplayHeight command to the server and then
	 * returns the result from the server to the caller.
         * @return The height of the display.
         */
        unsigned int DisplayHeight();

        /**
         * \brief Used to query the number of coefficient planes.
         *
         * Used to query the number of coefficient planes. Sends a NumCoefficients command to the server and then
	 * returns the result from the server to the caller.
         * @return The number of coefficient planes.
         */
        unsigned int NumCoefficientPlanes();

        /**
         * \brief Copies the provided pixel to the specified location.
         *
         * Copies the provided pixel to the specified location. Builds a PutPixel command from the arguments
	 * and sends it to the server.
         * @param p The pixel value to be copied.
         * @param location Tuple for the location within the frame volume where the pixel will be copied to.
         */
        void PutPixel(Pixel p, vector<unsigned int> &location);

        /**
         * \brief Copies the one dimensional array of pixels along a particular dimension in the frame volume.
         *
         * Copies the one dimensional array of pixels along a particular dimension in the frame volume. In a
         * two-dimensional frame volume, this can be thought of as a way to copy along a row or along a column, but
         * not both since the input pixels are only one-dimensional. Builds a CopyPixelStrip command from the arguments
	 * and sends it to the server.
         * @param p The pointer to the pixel values to be copied.
         * @param start Tuple for the first pixel in the frame volume to be filled.
         * @param end Tuple for the last pixel in the frame volume to be filled. All but one of the values in
         *            values in this last pixel should be identical to the start pixel.
         */
        void CopyPixelStrip(Pixel* p, vector<unsigned int> &start, vector<unsigned int> &end);

        /**
         * \brief Copies the array of pixels into the designated region of the frame volume.
         *
         * Copies the array of pixels into the designated region of the frame volume. The data must be
         * arranged in the array with strides for each dimension of the area. So to copy pixels into a
         * 2 x 2 x 2 region in the frame volume, the array must be arranged accordingly:
         * (0,0,0) (1,0,0) (0,1,0) (1,1,0) (0,0,1) (1,0,1) (0,1,1) (1,1,1)
	 * Builds a CopyPixels command from the arguments and sends it to the server.
         * @param p The pointer to the pixel values to be copied.
         * @param start Tuple for the first pixel in the frame volume to be filled.
         * @param end Tuple for the last pixel in the frame volume to be filled.
         */
        void CopyPixels(Pixel* p, vector<unsigned int> &start, vector<unsigned int> &end);

        /**
         * \brief Copies the array of pixels into the designated tile regions of the frame volume.
         *
         * Copies the array of pixels into the designated tile regions of the frame volume. The data must be
         * arranged in the array with strides for each dimension of the area. Only 2D tiles are supported.
	 * Builds a CopyPixelTiles command and sends it to the server.
         * @param p The pointer to the pixel values to be copied.
         * @param starts Vector holding series of tuples for the first pixel for each destination tile in the frame volume.
         * @param size Two element tuple for the size of each tile (w, h).
         */
        void CopyPixelTiles(vector<Pixel*> &p, vector<vector<unsigned int> > &starts, vector<unsigned int> &size);

        /**
         * \brief Fills the frame volume with the specified pixel.
         *
         * Fills the frame volume with the specified pixel. It can fill in multiple
         * dimensions by starting at the start pixel and filling in each dimension until
         * the end pixel value is reached. Builds a FillPixel command and sends it to the server.
         * @param p The pixel value to be filled.
         * @param start Tuple for the first pixel in the frame volume to be filled.
         * @param end Tuple for the last pixel in the frame volume to be filled.
         */
        void FillPixel(Pixel p, vector<unsigned int> &start, vector<unsigned int> &end);

        /**
         * \brief Copies pixels from one multi-dimensional region of the frame volume to another region.
         *
         * Copies pixels from one multi-dimensional region of the frame volume to another region.
	 * Builds a CopyFrameVolume command and sends it to the server.
         * @param start Tuple for the starting coordinate of the source region.
         * @param end Tuple for the ending coordinate of the source region.
         * @param dest Tuple for the first starting pixel of the destination region to be filled.
         */
        void CopyFrameVolume(vector<unsigned int> &start, vector<unsigned int> &end, vector<unsigned int> &dest);

        /**
         * \brief Used to update the input vector with the extra values in the input vector.
         *
         * Used to update the input vector with the extra values in the input vector.
	 * Builds a UpdateInputVector command and sends it to the server.
         * @param input Tuple for the values to use for the update. The length of this
         *              tuple must equal the size of the actual input vector
         *              minus two, since the first two values in the input
         *              vector cannot be changed.
         */
        void UpdateInputVector(vector<int> &input);

        /**
         * \brief Used to copy the specified coefficientMatrix into the specified location of the coefficient
         *        planes.
         *
         * Used to copy the specified coefficientMatrix into the specified location of the coefficient
         * planes. Builds a PutCoefficientMatrix command and sends it to the server.
         * @param coefficientMatrix This two-dimensional vector holds the matrix to be copied.
         *                          It's size must match the configuration of the coefficient matrices
         *                          exactly. Can use COFFICIENT_UNCHANGED for one or more elements.
         * @param location This two-element vector specifies the tuple for the location in the coefficient plane where the provided
         *                 coefficient matrix will be copied.
         */
        void PutCoefficientMatrix(vector< vector<int> > &coefficientMatrix, vector<unsigned int> &location);

        /**
         * \brief Used to copy the specified coefficientMatrix into a range of locations in the coefficient planes.
         *
         * Used to copy the specified coefficientMatrix into a range of locations in the coefficient planes.
	 * Builds a FillCoefficientMatrix command and sends it to the server.
         * @param coefficientMatrix This two-dimensional vector holds the matrix to be copied.
         *                          It's size must match the configuration of the coefficient matrices
         *                          exactly. Can use COFFICIENT_UNCHANGED for one or more elements.
         * @param start This three-element vector specifies the tuple of location in the coefficient planes where the first
         *              coefficient matrix will be copied to.
         * @param end This three-element vector specifies the tuple of the location in the coefficient planes where the last
         *            coefficient matrix will be copied to.
         */
        void FillCoefficientMatrix(vector< vector<int> > &coefficientMatrix, vector<unsigned int> &start, vector<unsigned int> &end);

        /**
         * \brief Used to copy the specified single coefficient value from a matrix into a range of locations in the coefficient planes.
         *
         * Used to copy the specified single coefficient value from a matrix into a range of locations in the coefficient planes.
	 * Builds a FillCoefficient command and sends it to the server.
         * @param coefficient This single value will be placed into each coefficient at the specified location in the coefficient
         *                    matrices of the specified range.
         * @param row The row of the coefficient to be updated in the coefficient matrix.
         * @param col The column of the coefficient to be updated in the coefficient matrix.
         * @param start This three-element vector specifies the tuple of location in the coefficient planes where the first
         *              coefficient matrix will be copied to.
         * @param end This three-element vector specifies the tuple of location in the coefficient planes where the last
         *            coefficient matrix will be copied to.
         */
        void FillCoefficient(int coefficient, unsigned int row, unsigned int col, vector<unsigned int> &start, vector<unsigned int> &end);

        /**
         * \brief For each coefficient, positions, and start; copies the coefficient to the position
         *        in the in each coefficient matrix in the 2D tile specified by the start and size.
         *
         * For each coefficient, positions, and start; copies the coefficient to the position
         * in the in each coefficient matrix in the 2D tile specified by the start and size.
	 * Builds a FillCoefficientTiles command and sends it to the server.
         * @param coefficients The buffer of coefficients.
         * @param positions Tuple for the position (row, col) to place the coefficient within the coefficient matrix.
         * @param starts Tuple for the location (x, y) of the start of the tile in the coefficient planes.
         * @param size Tuple for the size (w, h) of the tile.
         */
        void FillCoefficientTiles(vector<int> &coefficients, vector<vector<unsigned int> > &positions, vector<vector<unsigned int> > &starts, vector<unsigned int> &size);

        /**
         * \brief Used to copy the specified scaler to a range of locations in the coefficient planes.
         *
         * Used to copy the specified scaler to a range of locations in the coefficient planes.
	 * Builds a FillScaler command and sends it to the server.
         * @param scaler This single scaler will be copied to each location in the range of coefficient planes.
         * @param start This three-element vector specifies tuple for the start of the range in the coefficient planes
         *              where the scaler will be copied to.
         * @param end This three-element vector specifies the tuple for end of the range in the coefficient planes where
         *            the scalers will be copied to.
         */
        void FillScaler(Scaler scaler, vector<unsigned int> &start, vector<unsigned int> &end);

        /**
         * \brief Used to copy the specified scalers to a series of 2D ranges of locations (tiles) in the coefficient planes.
         *
         * Used to copy the specified scalers to a series of 2D ranges of locations (tiles) in the coefficient planes.
         * This is accomplished with a set of scalers, an equal number of tile starts, and one tile size.
	 * Builds a FillScalerTiles command and sends it to the server.
         * @param scalers Each scaler in this list will be filled to its own tile, which is a 2D range of coefficient matrices.
         * @param starts Vector of tuples for the start locations (x, y, z) in the coefficient planes for each tile to be filled.
         * @param size Tuple for the size (w, h) of the tile.
         */
        void FillScalerTiles(vector<uint64_t> &scalers, vector<vector<unsigned int> > &starts, vector<unsigned int> &size);

        /**
         * \brief Used to copy the specified scalers to a stack of 2D ranges of locations (tiles) in the coefficient planes.
         *
         * Used to copy the specified scalers to a stack of 2D ranges of locations (tiles) in the coefficient planes.
         * This is accomplished with with a set of scalers, a single tile stack location, and one tile size.
         * The stack includes the top-most tile on the coefficient plane indicated by the tile start as well as
         * the tiles for the planes under (higher plane coordinates) the start tile. The height of the stack to be
         * filled is determined by the number of scalers provided. Builds a FillScalerTileStack command and sends it to the server.
         * @param scalers Each scaler in this list will be filled to its own tile (2D range of coefficient matrices) in the stack.
         * @param start Tuple for the location (x, y) of the start of the tile stack in the coefficient planes.
         * @param size Tuple for the size (w, h) of the tile.
         */
        void FillScalerTileStack(vector<uint64_t> &scalers, vector<unsigned int> &start, vector<unsigned int> &size);

        /**
         * \brief Allows the bytes of pixel values to be interpretted as signed values when scaling, accumulating, and clamping
         * in the pixel blending pipeline.
         *
         * Allows the bytes of pixel values to be interpretted as signed values when scaling, accumulating, and clamping
         * in the pixel blending pipeline. When the SignMode is set to the default UNSIGNED_MODE, each 8-bit color channel for
         * pixels will be treated as an unsigned value. Setting this configuration option to SIGNED_MODE treats those channels
         * as 8-bit signed values. Builds a SetPixelByteSignMode command and sends it to the server.
         * @param mode Can be UNSIGNED_MODE or SIGNED_MODE.
         */
        void SetPixelByteSignMode(SignMode mode);

        /**
         * \brief Used to set the scaler value which is interpretted as fully on or 100%.
         *
         * Used to set the scaler value which is interpretted as fully on or 100%. The default is
         * 256, which implies that any scaler sent by the client is an
         * integer fraction of 256, but in fact a scaler can be larger than 256,
         * leading to planes that contribute 2.5x or even -3x for instance. Builds a SetFullScaler
	 * command and sends it to the server.
         * @param scaler The value to be interpretted as fully on or 100%.
         */
        void SetFullScaler(uint16_t scaler);

        /**
         * \brief Used to get the current full scaler value.
         *
         * Used to get the current full scaler value. Sends a GetFullScaler command to the server
	 * and returns the response from the server.
         * @return The current fully on scaler value.
         */
        uint16_t GetFullScaler();

	/**
	 * \brief Returns null as there is not support for retrieving the CostModel from the server.
	 *
	 * Returns null as there is not support for retrieving the CostModel from the server. Instead,
	 * the server reports the CostModel data when it exits.
	 */
        CostModel* GetCostModel();
	
        /**
         * \brief Sends the ClearCostModel command to the server.
         *
	 * Sends the ClearCostModel command to the server.
         */
        void ClearCostModel();

        /**
         * \brief Sends the Latch command to the server for the given subregion.
         *
	 * Sends the Latch command to the server for the given subregion.
	 * @param sub_x The x coordinate of the start of the subregion.
	 * @param sub_y The y coordinate of the start of the subregion.
	 * @param sub_w The width of the subregion
 	 * @param sub_h The height of the subregion
        */
        void Latch(uint32_t sub_x, uint32_t sub_y, uint32_t sub_w, uint32_t sub_h);

        /**
         * \brief Sends the Shutdown command to the server.
         *
	 * Sends the Shutdown command to the server.
         */
        void Shutdown();

    private:
        unique_ptr<NddiWall::Stub> stub_;

    };

}

#endif // GRPC_NDDI_DISPLAY_H
