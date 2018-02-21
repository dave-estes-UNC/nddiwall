* Create skeleton server/client using a grpc example.
* Define simple initial service and messages to initialize NDDI Display.
  * Contructor
* Back in the pixelbridge project, fix costmodel to set headless based on global configuration in pixelbridge main.
* Push latest changes to NDDI (costmodel/headless and splitting of features.h)
* Fix GOMP link errors.
* Fix CL and GL warnings.
* Fix .PRECIOUS make errors.
* Add GL window support to the server so we can start seeing progress.
* Change int to uint for NDDI Interface where appropriate.
* Create a GrpcNddiDisplay for the client to use.
* Add dependencies to Makefile...er...just use cmake.
* Add NDDI Wall Pixel Bridge
* Define additional messages matching the remaining APIs.
  * unsigned int DisplayWidth()
  * unsigned int DisplayHeight()
  * unsigned int NumCoefficientPlanes()
  * void PutPixel(Pixel p, vector<unsigned int> &location)
  * void CopyPixelStrip(Pixel* p, vector<unsigned int> &start, vector<unsigned int> &end)
  * void CopyPixels(Pixel* p, vector<unsigned int> &start, vector<unsigned int> &end)
  * void CopyPixelTiles(vector<Pixel*> &p, vector<vector<unsigned int> > &starts, vector<unsigned int> &size)
  * void FillPixel(Pixel p, vector<unsigned int> &start, vector<unsigned int> &end)
  * void CopyFrameVolume(vector<unsigned int> &start, vector<unsigned int> &end, vector<unsigned int> &dest)
  * void UpdateInputVector(vector<int> &input)
  * void PutCoefficientMatrix(vector< vector<int> > &coefficientMatrix, vector<unsigned int> &location)
  * void FillCoefficientMatrix(vector< vector<int> > &coefficientMatrix, vector<unsigned int> &start, vector<unsigned int> &end)
  * void FillCoefficient(int coefficient, int row, int col, vector<unsigned int> &start, vector<unsigned int> &end)
  * void FillCoefficientTiles(vector<int> &coefficients, vector<vector<unsigned int> > &positions, vector<vector<unsigned int> > &starts, vector<unsigned int> &size)
  * void FillScaler(Scaler scaler, vector<unsigned int> &start, vector<unsigned int> &end)
  * void FillScalerTiles(vector<uint64_t> &scalers, vector<vector<unsigned int> > &starts, vector<unsigned int> &size)
  * void FillScalerTileStack(vector<uint64_t> &scalers, vector<unsigned int> &start, vector<unsigned int> &size)
  * void SetPixelByteSignMode(SignMode mode)
  * void SetFullScaler(uint16_t scaler)
  * uint16_t GetFullScaler()
* Find out if there's a way to memcpy into and out of GRPC request buffers. Look into "bytes" type.
* Add a simple latch
- Look into the rendering artifacts for fb and cache modes.
* Investigate changing vectors to straight arrays throughout the NDDI interface.
  * Then move the biggest arrays to "bytes" in the protobuffers.
- Write a NDDI recorder/player that serializes and parses these new memcpy'able arrays to and from files. Perhaps JSON?
  * Add thread safety to command queue.
  - Kill the server when finished.
  * Move to vectors instead of malloc'd arrays.
  * Implement remaining recorders serializers
  * Add playback.
    * Add input serializers.
    * Add creation and utilization of an NDDI Display.
  * Fix RecorderNddiDisplay bug with NDDI client.
  * Switch to Binary Archive
  * Get PixelBridge working with RecorderNddiDisplay.
    * Figure out a way to have recorders return full scaler, num coefficient planes, etc.
    * Convert all of the Tilers to accept a file in the constructor and to use a generic NDimensionalDisplayInterface
      display_ like FlatTiler.
    * Similarly, convert PixelBridge's simple (fb) mode to construct a recorder or grpc display.
    * Fix bug with DCT tilers. Likely one of the scaler commands.
p Add a proper sync mechanism.
* Define experiments
  * Video Conference (7689x4320)
    * 4x Across Top: 1080p participant streams
    * Middle Left: 4k Slide Show
    * Middle Right: 1080p presenter stream scaled 2x
    * 4x Across Bottom: 1080p paricipant streams
  * Stadium Jumbotron (7680x4860)
    * Main Frame: 4k Soccer footage scaled 2x (7680x4320)
    * Top Right Frame: 1080p Video commercial (1920x1080)
    * Middle Right Frame: Game Statistics (1920x2160)
    * Bottom Right Frame: 1080p Video second game (1920x1080)
    * Bottom Frame: Ticker (7680x540)
  * City Billboard
    * Three 1080p displays for top of building
    * One 4k main vertical display
    * One quarter-height 4k vertical display
    * One 4k side horizontal display
* Coefficient Plans RAM savings
  x Create a new memory-thrify implementation of coefficient planes.
  x Do not preallocate coefficient and scalar memory.
  x Instead allocate CoefficientMatrices and Scalars on-demand. CoefficientPlanes will have a pointer for every physical
    pixel to the matrix and the scalar whenever they're initialized.
  x Fill commands will allocate a single matrix or scalar and will update the pointers to point to it.
  x Any command to set a coefficient or scalar will trigger an efficient reverse-lookup to see how many pixel locations
    use it. If the command is a fill which includes all of those locations, then the single piece of shared memory is updated.
    Otherwise, the shared memory is duplicated into a new allocation for the command's range, and then the values are updated.
  x The reverse-lookup is done be maintaining a set of bounding volumes arranged from the largest to the smallest.
  * Implement support for fixed 8x8 macroblocks with one coefficient matrix and one scaler per macroblock.
  * Implement single coefficient plane emulation of 64 plane using special coefficients for current X, Y and P.
    (coefficient matrices only)
  * Add RAM savings features support to the nddiwallserver. Will include expanding the Initialization message.
  * Turn on the support for the RAM savings features via PixelBridgeFeatures.h.
  * Build in the support to the version of DctTiler in the nddiwall project. Note, a simple copy of DctTiler.* might suffice.
    * Fix crash in nddiwall_server
- Frame Volume RAM savings
  - If dimensionality jumps up, then we'll use a dynamic allocator that just allocates planes of RAM when an area
    of the Frame Volume is initialized.
* Modify nddiwall_server to spit out stats like the original pixelbridge application did. Includes CSV support.
  * Get the statistics sorted out for nddi server, making sure they match the standalone pixelbridge app when using RAM
    savings and when not.
    * Figure out what to do with the extra bulk transmission changes that we need to log from DctTiler.
      There are some TODOs marked on #if 0 lines.
    * Copy over the outputStats() implementation from legacy PixelBridge to nddiwall_server.
- Work out multi-client scheme
  * Add the build targets for nddiwall_master_client and nddiwall_slave_client using nddiwall_test_client and nddiwall_pixelbridge_client.
  * Create a master client which initializes the display to the full size. When it exits, it will send the shutdown command.
  * Make any necessary changes to support multiple GRPC clients connecting to the server.
  * Modify the PixelBridge to support operation as a slave client. In this mode, it won't send an Init Command or a Shutdown Command.
    This mode is triggered with a command line option which also specifies its region of the display. This region is strictly used as
    by the client itself primarily in form of x and y offsets for the nddi commands. (e.g. When filling frame volume or coefficient plane
    data, these offsets will be added to the start/end/position arguments.
  * Modify PixelBridge to shift its coordinates for all of the NDDI commands using the subregion.
  - Expand the latch command to optionally take a region of the display to latch. This will extend down to the
    NDDI implementation (Likely just GlNddiDisplay) such that it only computes the latched region and displays that portion updated
    alongside the previous framebuffer. Furthermore, the CostModel will reflect only that region of pixels for the Pixel Mapping Charge.
  * Modify RecorderNddiDisplay to also support slave mode. Make sure recording playback of multiple clients works.
  - NOTE: The scheme above will not take synchronization into account. Instead each client will produce a given number of frames for
          the particular use case. Once each client has finished that number of frames, the master client will exit, triggering the
          display of the client stats.