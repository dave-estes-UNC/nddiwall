#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>
#include <sys/time.h>

#ifdef USE_GL
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#endif

#include <grpc++/grpc++.h>

// Only including PixelBridgeFeatures.h for warnings about configuration.
#include "PixelBridgeFeatures.h"
#include "Configuration.h"

#include "nddi/Features.h"
#ifdef USE_GL
#include "nddi/GlNddiDisplay.h"
#else
#include "nddi/SimpleNddiDisplay.h"
#endif

#include "nddiwall.grpc.pb.h"

#ifdef DEBUG
#define DEBUG_MSG(str) do { std::cout << str; } while( false )
#else
#define DEBUG_MSG(str) do { } while ( false )
#endif

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using nddiwall::InitializeRequest;
using nddiwall::StatusReply;
using nddiwall::DisplayWidthRequest;
using nddiwall::DisplayHeightRequest;
using nddiwall::DisplayWidthReply;
using nddiwall::DisplayHeightReply;
using nddiwall::NumCoefficientPlanesRequest;
using nddiwall::NumCoefficientPlanesReply;
using nddiwall::PutPixelRequest;
using nddiwall::FillPixelRequest;
using nddiwall::CopyFrameVolumeRequest;
using nddiwall::CopyPixelStripRequest;
using nddiwall::CopyPixelsRequest;
using nddiwall::CopyPixelTilesRequest;
using nddiwall::PutCoefficientMatrixRequest;
using nddiwall::FillCoefficientMatrixRequest;
using nddiwall::FillCoefficientRequest;
using nddiwall::FillCoefficientTilesRequest;
using nddiwall::FillScalerRequest;
using nddiwall::FillScalerTilesRequest;
using nddiwall::FillScalerTileStackRequest;
using nddiwall::SetPixelByteSignModeRequest;
using nddiwall::GetFullScalerRequest;
using nddiwall::GetFullScalerReply;
using nddiwall::SetFullScalerRequest;
using nddiwall::UpdateInputVectorRequest;
using nddiwall::ClearCostModelRequest;
using nddiwall::LatchRequest;
using nddiwall::ShutdownRequest;
using nddiwall::NddiWall;

/*
 * Globals
 */
#ifdef USE_GL
GlNddiDisplay* myDisplay;
#else
SimpleNddiDisplay* myDisplay;
#endif
pthread_t serverThread;
pthread_mutex_t renderMutex;
pthread_cond_t renderCondition;
std::unique_ptr<Server> server;
bool alive;
int totalUpdates = 0;
timeval startTime, endTime; // Used for timing data
uint32_t sub_x, sub_y, sub_w, sub_h;


// Logic and data behind the server's behavior.
class NddiServiceImpl final : public NddiWall::Service {

  Status Initialize(ServerContext* context, const InitializeRequest* request,
                    StatusReply* reply) override {
    DEBUG_MSG("Server got a request to initialize an NDDI Display." << std::endl);
    if (!myDisplay) {
        inputVectorSize_ = request->inputvectorsize();
        frameVolumeDimensionality_ = request->framevolumedimensionalsizes_size();

        DEBUG_MSG("  - Frame Volume: ");
        vector<unsigned int> fvDimensions;
        for (int i = 0; i < request->framevolumedimensionalsizes_size(); i++) {
            fvDimensions.push_back(request->framevolumedimensionalsizes(i));
            if (i) { DEBUG_MSG("x"); }
            DEBUG_MSG(request->framevolumedimensionalsizes(i));
        }
        DEBUG_MSG(std::endl);
        DEBUG_MSG("  - Display: " << request->displaywidth() << "x" << request->displayheight() << std::endl);
        DEBUG_MSG("  - Coefficient Planes: " << request->numcoefficientplanes() << std::endl);
        DEBUG_MSG("  - Input Vector Size: " << request->inputvectorsize() << std::endl);
        DEBUG_MSG("  - Fixed 8x8 Macroblocks: " << request->fixed8x8macroblocks() << std::endl);
        DEBUG_MSG("  - Use Single Coeffcient Plane: " << request->usesinglecoeffcientplane() << std::endl);

        // Initialize the NDDI display
#ifdef USE_GL
        myDisplay = new GlNddiDisplay(fvDimensions,                    // framevolume dimensional sizes
                                      request->displaywidth(),         // display size
                                      request->displayheight(),
                                      request->numcoefficientplanes(), // number of coefficient planes on the display
                                      request->inputvectorsize(),      // input vector size (x, y, t)
                                      false,                           // Is not headless
                                      request->fixed8x8macroblocks(),  // Use fixed macroblocks
                                      request->usesinglecoeffcientplane()); // Use only one coefficient plane for coefficeints
#else
        myDisplay = new SimpleNddiDisplay(fvDimensions,                    // framevolume dimensional sizes
                                          request->displaywidth(),         // display size
                                          request->displayheight(),
                                          request->numcoefficientplanes(), // number of coefficient planes on the display
                                          request->inputvectorsize(),      // input vector size (x, y, t)
                                          false,                           // Is not headless
                                          request->fixed8x8macroblocks(),  // Use fixed macroblocks
                                          request->usesinglecoeffcientplane()); // Use only one coefficient plane for coefficeints
#endif

        reply->set_status(reply->OK);
    } else {
        reply->set_status(reply->NOT_OK);
    }
    return Status::OK;
  }

  Status DisplayWidth(ServerContext* context, const DisplayWidthRequest* request,
                      DisplayWidthReply* reply) override {
      DEBUG_MSG("Server got a request for the NDDI Display width." << std::endl);
      if (myDisplay) {
          reply->set_width(myDisplay->DisplayWidth());
      } else {
          reply->set_width(0);
      }
      return Status::OK;
  }

  Status DisplayHeight(ServerContext* context, const DisplayHeightRequest* request,
                       DisplayHeightReply* reply) override {
      DEBUG_MSG("Server got a request for the NDDI Display height." << std::endl);
      if (myDisplay) {
          reply->set_height(myDisplay->DisplayHeight());
      } else {
          reply->set_height(0);
      }
      return Status::OK;
  }

  Status NumCoefficientPlanes(ServerContext* context, const NumCoefficientPlanesRequest* request,
                              NumCoefficientPlanesReply* reply) override {
      DEBUG_MSG("Server got a request for the NDDI Display number of coefficient planes." << std::endl);
      if (myDisplay) {
          reply->set_planes(myDisplay->NumCoefficientPlanes());
      } else {
          reply->set_planes(0);
      }
      return Status::OK;
  }

  Status PutPixel(ServerContext* context, const PutPixelRequest* request,
                  StatusReply* reply) override {
      DEBUG_MSG("Server got a request to PutPixel." << std::endl);
      if (myDisplay) {
          DEBUG_MSG("  - Location: (");
          vector<unsigned int> location;
          for (int i = 0; i < request->location_size(); i++) {
              location.push_back(request->location(i));
              if (i) { DEBUG_MSG(","); }
              DEBUG_MSG(request->location(i));
          }
          DEBUG_MSG(")" << std::endl);

          Pixel p;
          p.packed = request->pixel();
          DEBUG_MSG("  - Pixel: " << (uint32_t)p.r << " " << (uint32_t)p.g << " " << (uint32_t)p.b << " " << (uint32_t)p.a << std::endl);

          myDisplay->PutPixel(p, location);

          reply->set_status(reply->OK);
      } else {
          reply->set_status(reply->NOT_OK);
      }
      return Status::OK;
  }

  Status FillPixel(ServerContext* context, const FillPixelRequest* request,
                  StatusReply* reply) override {
      DEBUG_MSG("Server got a request to FillPixel." << std::endl);
      if (myDisplay) {
          DEBUG_MSG("  - Start: (");
          vector<unsigned int> start;
          for (int i = 0; i < request->start_size(); i++) {
              start.push_back(request->start(i));
              if (i) { DEBUG_MSG(","); }
              DEBUG_MSG(request->start(i));
          }
          DEBUG_MSG(")" << std::endl);

          DEBUG_MSG("  - End: (");
          vector<unsigned int> end;
          for (int i = 0; i < request->end_size(); i++) {
              end.push_back(request->end(i));
              if (i) { DEBUG_MSG(","); }
              DEBUG_MSG(request->end(i));
          }
          DEBUG_MSG(")" << std::endl);

          Pixel p;
          p.packed = request->pixel();
          DEBUG_MSG("  - Pixel: " << (uint32_t)p.r << " " << (uint32_t)p.g << " " << (uint32_t)p.b << " " << (uint32_t)p.a << std::endl);

          myDisplay->FillPixel(p, start, end);

          reply->set_status(reply->OK);
      } else {
          reply->set_status(reply->NOT_OK);
      }
      return Status::OK;
  }

  Status CopyFrameVolume(ServerContext* context, const CopyFrameVolumeRequest* request,
                         StatusReply* reply) override {
      DEBUG_MSG("Server got a request to CopyFrameVolume." << std::endl);
      if (myDisplay) {
          DEBUG_MSG("  - Start: (");
          vector<unsigned int> start;
          for (int i = 0; i < request->start_size(); i++) {
              start.push_back(request->start(i));
              if (i) { DEBUG_MSG(","); }
              DEBUG_MSG(request->start(i));
          }
          DEBUG_MSG(")" << std::endl);

          DEBUG_MSG("  - End: (");
          vector<unsigned int> end;
          for (int i = 0; i < request->end_size(); i++) {
              end.push_back(request->end(i));
              if (i) { DEBUG_MSG(","); }
              DEBUG_MSG(request->end(i));
          }
          DEBUG_MSG(")" << std::endl);

          DEBUG_MSG("  - Dest: (");
          vector<unsigned int> dest;
          for (int i = 0; i < request->dest_size(); i++) {
              dest.push_back(request->dest(i));
              if (i) { DEBUG_MSG(","); }
              DEBUG_MSG(request->dest(i));
          }
          DEBUG_MSG(")" << std::endl);

          myDisplay->CopyFrameVolume(start, end, dest);

          reply->set_status(reply->OK);
      } else {
          reply->set_status(reply->NOT_OK);
      }
      return Status::OK;
  }

  Status CopyPixelStrip(ServerContext* context, const CopyPixelStripRequest* request,
                      StatusReply* reply) override {
      DEBUG_MSG("Server got a request to CopyPixelStrip." << std::endl);
      if (myDisplay) {
          size_t count = request->pixels().length() / sizeof(Pixel);
          DEBUG_MSG("  - Pixels: " << count << std::endl);
          Pixel p[count];
          memcpy(p, request->pixels().data(), count * sizeof(Pixel));

          DEBUG_MSG("  - Start: (");
          vector<unsigned int> start;
          for (int i = 0; i < request->start_size(); i++) {
              start.push_back(request->start(i));
              if (i) { DEBUG_MSG(","); }
              DEBUG_MSG(request->start(i));
          }
          DEBUG_MSG(")" << std::endl);

          DEBUG_MSG("  - End: (");
          vector<unsigned int> end;
          for (int i = 0; i < request->end_size(); i++) {
              end.push_back(request->end(i));
              if (i) { DEBUG_MSG(","); }
              DEBUG_MSG(request->end(i));
          }
          DEBUG_MSG(")" << std::endl);

          myDisplay->CopyPixelStrip(p, start, end);

          reply->set_status(reply->OK);
      } else {
          reply->set_status(reply->NOT_OK);
      }
      return Status::OK;
  }

  Status CopyPixels(ServerContext* context, const CopyPixelsRequest* request,
                    StatusReply* reply) override {
      DEBUG_MSG("Server got a request to CopyPixels." << std::endl);
      if (myDisplay) {
          size_t count = request->pixels().length() / sizeof(Pixel);
          DEBUG_MSG("  - Pixels: " << count << std::endl);
          Pixel p[count];
          memcpy(p, request->pixels().data(), count * sizeof(Pixel));

          DEBUG_MSG("  - Start: (");
          vector<unsigned int> start;
          for (int i = 0; i < request->start_size(); i++) {
              start.push_back(request->start(i));
              if (i) { DEBUG_MSG(","); }
              DEBUG_MSG(request->start(i));
          }
          DEBUG_MSG(")" << std::endl);

          DEBUG_MSG("  - End: (");
          vector<unsigned int> end;
          for (int i = 0; i < request->end_size(); i++) {
              end.push_back(request->end(i));
              if (i) { DEBUG_MSG(","); }
              DEBUG_MSG(request->end(i));
          }
          DEBUG_MSG(")" << std::endl);

          myDisplay->CopyPixels(p, start, end);

          reply->set_status(reply->OK);
      } else {
          reply->set_status(reply->NOT_OK);
      }
      return Status::OK;
  }

  Status CopyPixelTiles(ServerContext* context, const CopyPixelTilesRequest* request,
                        StatusReply* reply) override {
      DEBUG_MSG("Server got a request to CopyPixelTiles." << std::endl);
      if (myDisplay) {
          size_t count = request->pixels().length() / sizeof(Pixel);
          DEBUG_MSG("  - Pixels: " << count << std::endl);
          Pixel p[count];
          memcpy(p, request->pixels().data(), count * sizeof(Pixel));
          size_t tile_size = request->size(0) * request->size(1);
          size_t tile_count = request->starts_size() / frameVolumeDimensionality_;
          vector<Pixel*> ps(tile_count, 0);
          for (int i = 0; i < tile_count; i++) {
              ps[i] = p + (i * tile_size);
          }

          DEBUG_MSG("  - Starts: " << request->starts_size() << std::endl);
          vector< vector<unsigned int> > starts;
          for (int i = 0; i < tile_count; i++) {
              vector<unsigned int> start;
              for (int j = 0; j < frameVolumeDimensionality_; j++) {
                  start.push_back(request->starts(i * frameVolumeDimensionality_ + j));
              }
              starts.push_back(start);
          }

          DEBUG_MSG("  - Size: (");
          vector<unsigned int> size;
          size.push_back(request->size(0));
          size.push_back(request->size(1));
          DEBUG_MSG(request->size(0) << "," << request->size(1) << ")" << std::endl);

          myDisplay->CopyPixelTiles(ps, starts, size);

          reply->set_status(reply->OK);
      } else {
          reply->set_status(reply->NOT_OK);
      }
      return Status::OK;
  }

  Status PutCoefficientMatrix(ServerContext* context, const PutCoefficientMatrixRequest* request,
                              StatusReply* reply) override {
      DEBUG_MSG("Server got a request to PutCoefficientMatrix." << std::endl);
      if (myDisplay) {
          DEBUG_MSG("  - Coefficient Matrix (row <-> col):" << std::endl);
          vector< vector<int> > coefficientMatrix;
          assert(request->coefficientmatrix_size() == inputVectorSize_ * frameVolumeDimensionality_);
          coefficientMatrix.resize(frameVolumeDimensionality_);
          for (int j = 0; j < frameVolumeDimensionality_; j++) {
              DEBUG_MSG("    ");
              for (int i = 0; i < inputVectorSize_; i++) {
                  coefficientMatrix[j].push_back(request->coefficientmatrix(j * frameVolumeDimensionality_ + i));
                  DEBUG_MSG(request->coefficientmatrix(j * frameVolumeDimensionality_ + i) << " ");
              }
              DEBUG_MSG(std::endl);
          }

          DEBUG_MSG("  - Location: (");
          vector<unsigned int> location;
          for (int i = 0; i < request->location_size(); i++) {
              location.push_back(request->location(i));
              if (i) { DEBUG_MSG(","); }
              DEBUG_MSG(request->location(i));
          }
          DEBUG_MSG(")" << std::endl);

          myDisplay->PutCoefficientMatrix(coefficientMatrix, location);

          reply->set_status(reply->OK);
      } else {
          reply->set_status(reply->NOT_OK);
      }
      return Status::OK;
  }

  Status FillCoefficientMatrix(ServerContext* context, const FillCoefficientMatrixRequest* request,
                               StatusReply* reply) override {
      DEBUG_MSG("Server got a request to FillCoefficientMatrix." << std::endl);
      if (myDisplay) {
          DEBUG_MSG("  - Coefficient Matrix (row <-> col):" << std::endl);
          vector< vector<int> > coefficientMatrix;
          assert(request->coefficientmatrix_size() == inputVectorSize_ * frameVolumeDimensionality_);
          coefficientMatrix.resize(frameVolumeDimensionality_);
          for (int j = 0; j < frameVolumeDimensionality_; j++) {
              DEBUG_MSG("    ");
              for (int i = 0; i < inputVectorSize_; i++) {
                  coefficientMatrix[j].push_back(request->coefficientmatrix(j * frameVolumeDimensionality_ + i));
                  DEBUG_MSG(request->coefficientmatrix(j * frameVolumeDimensionality_ + i) << " ");
              }
              DEBUG_MSG(std::endl);
          }

          DEBUG_MSG("  - Start: (");
          vector<unsigned int> start;
          for (int i = 0; i < request->start_size(); i++) {
              start.push_back(request->start(i));
              if (i) { DEBUG_MSG(","); }
              DEBUG_MSG(request->start(i));
          }
          DEBUG_MSG(")" << std::endl);

          DEBUG_MSG("  - End: (");
          vector<unsigned int> end;
          for (int i = 0; i < request->end_size(); i++) {
              end.push_back(request->end(i));
              if (i) { DEBUG_MSG(","); }
              DEBUG_MSG(request->end(i));
          }
          DEBUG_MSG(")" << std::endl);

          myDisplay->FillCoefficientMatrix(coefficientMatrix, start, end);

          reply->set_status(reply->OK);
      } else {
          reply->set_status(reply->NOT_OK);
      }
      return Status::OK;
  }

  Status FillCoefficient(ServerContext* context, const FillCoefficientRequest* request,
                         StatusReply* reply) override {
      DEBUG_MSG("Server got a request to FillCoefficient." << std::endl);
      if (myDisplay) {
          DEBUG_MSG("  - Start: (");
          vector<unsigned int> start;
          for (int i = 0; i < request->start_size(); i++) {
              start.push_back(request->start(i));
              if (i) { DEBUG_MSG(","); }
              DEBUG_MSG(request->start(i));
          }
          DEBUG_MSG(")" << std::endl);

          DEBUG_MSG("  - End: (");
          vector<unsigned int> end;
          for (int i = 0; i < request->end_size(); i++) {
              end.push_back(request->end(i));
              if (i) { DEBUG_MSG(","); }
              DEBUG_MSG(request->end(i));
          }
          DEBUG_MSG(")" << std::endl);

          int coefficient = request->coefficient();
          DEBUG_MSG("  - Coefficient: " << coefficient << std::endl);
          int col = request->col();
          DEBUG_MSG("  - Col: " << col << std::endl);
          int row = request->row();
          DEBUG_MSG("  - Row: " << row << std::endl);

          myDisplay->FillCoefficient(coefficient, row, col, start, end);

          reply->set_status(reply->OK);
      } else {
          reply->set_status(reply->NOT_OK);
      }
      return Status::OK;
  }

  Status FillCoefficientTiles(ServerContext* context, const FillCoefficientTilesRequest* request,
                        StatusReply* reply) override {
      DEBUG_MSG("Server got a request to FillCoefficientTiles." << std::endl);
      if (myDisplay) {
          size_t tile_count = request->coefficients_size();
          DEBUG_MSG("  - Coefficients: " << request->coefficients_size() << std::endl);
          vector<int> coeffs;
          for (int i = 0; i < request->coefficients_size(); i++) {
              coeffs.push_back(request->coefficients(i));
          }

          DEBUG_MSG("  - Positions: " << request->positions_size() << std::endl);
          vector< vector<unsigned int> > positions;
          for (int i = 0; i < tile_count; i++) {
              vector<unsigned int> pos;
              pos.push_back(request->positions(2 * i + 0));
              pos.push_back(request->positions(2 * i + 0));
              positions.push_back(pos);
          }

          DEBUG_MSG("  - Starts: " << request->starts_size() << std::endl);
          vector< vector<unsigned int> > starts;
          for (int i = 0; i < tile_count; i++) {
              vector<unsigned int> start;
              for (int j = 0; j < frameVolumeDimensionality_; j++) {
                  start.push_back(request->starts(i * frameVolumeDimensionality_ + j));
              }
              starts.push_back(start);
          }

          DEBUG_MSG("  - Size: (");
          vector<unsigned int> size;
          size.push_back(request->size(0));
          size.push_back(request->size(1));
          DEBUG_MSG(request->size(0) << "," << request->size(1) << ")" << std::endl);

          myDisplay->FillCoefficientTiles(coeffs, positions, starts, size);

          reply->set_status(reply->OK);
      } else {
          reply->set_status(reply->NOT_OK);
      }
      return Status::OK;
  }

  Status FillScaler(ServerContext* context, const FillScalerRequest* request,
                  StatusReply* reply) override {
      DEBUG_MSG("Server got a request to FillScaler." << std::endl);
      if (myDisplay) {
          DEBUG_MSG("  - Start: (");
          vector<unsigned int> start;
          for (int i = 0; i < request->start_size(); i++) {
              start.push_back(request->start(i));
              if (i) { DEBUG_MSG(","); }
              DEBUG_MSG(request->start(i));
          }
          DEBUG_MSG(")" << std::endl);

          DEBUG_MSG("  - End: (");
          vector<unsigned int> end;
          for (int i = 0; i < request->end_size(); i++) {
              end.push_back(request->end(i));
              if (i) { DEBUG_MSG(","); }
              DEBUG_MSG(request->end(i));
          }
          DEBUG_MSG(")" << std::endl);

          Scaler s;
          s.packed = request->scaler();
          DEBUG_MSG("  - Scaler: " << s.r << " " << s.g << " " << s.r << " " << s.a << std::endl);

          myDisplay->FillScaler(s, start, end);

          reply->set_status(reply->OK);
      } else {
          reply->set_status(reply->NOT_OK);
      }
      return Status::OK;
  }

  Status FillScalerTiles(ServerContext* context, const FillScalerTilesRequest* request,
                         StatusReply* reply) override {
      DEBUG_MSG("Server got a request to FillScalerTiles." << std::endl);
      if (myDisplay) {
          size_t tile_count = request->scalers_size();
          DEBUG_MSG("  - Scalers: " << request->scalers_size() << std::endl);
          vector<uint64_t> scalers;
          for (int i = 0; i < request->scalers_size(); i++) {
              scalers.push_back(request->scalers(i));
          }

          DEBUG_MSG("  - Starts: " << request->starts_size() << std::endl);
          vector < vector<unsigned int> > starts;
          for (int i = 0; i < tile_count; i++) {
              vector<unsigned int> start;
              for (int j = 0; j < frameVolumeDimensionality_; j++) {
                  start.push_back(
                          request->starts(
                                  i * frameVolumeDimensionality_ + j));
              }
              starts.push_back(start);
          }

          DEBUG_MSG("  - Size: (");
          vector<unsigned int> size;
          size.push_back(request->size(0));
          size.push_back(request->size(1));
          DEBUG_MSG(
                  request->size(0) << "," << request->size(1) << ")" << std::endl);

          myDisplay->FillScalerTiles(scalers, starts, size);

          reply->set_status(reply->OK);
      } else {
          reply->set_status(reply->NOT_OK);
      }
      return Status::OK;
  }

  Status FillScalerTileStack(ServerContext* context, const FillScalerTileStackRequest* request,
                             StatusReply* reply) override {
      DEBUG_MSG("Server got a request to FillScalerTileStack." << std::endl);
      if (myDisplay) {
          vector<uint64_t> scalers;
          DEBUG_MSG("  - Scalers: " << request->scalers_size() << std::endl);
          for (int i = 0; i < request->scalers_size(); i++) {
              scalers.push_back(request->scalers(i));
          }

          DEBUG_MSG("  - Start: (");
          vector<unsigned int> start;
          for (int i = 0; i < request->start_size(); i++) {
              start.push_back(request->start(i));
              if (i) { DEBUG_MSG(","); }
              DEBUG_MSG(request->start(i));
          }
          DEBUG_MSG(")" << std::endl);

          DEBUG_MSG("  - Size: (");
          vector<unsigned int> size;
          for (int i = 0; i < request->size_size(); i++) {
              size.push_back(request->size(i));
              if (i) { DEBUG_MSG(","); }
              DEBUG_MSG(request->size(i));
          }
          DEBUG_MSG(")" << std::endl);

          myDisplay->FillScalerTileStack(scalers, start, size);

          reply->set_status(reply->OK);
      } else {
          reply->set_status(reply->NOT_OK);
      }
      return Status::OK;
  }

  Status SetPixelByteSignMode(ServerContext* context, const SetPixelByteSignModeRequest* request,
                              StatusReply* reply) override {
      DEBUG_MSG("Server got a request to set the sign mode." << std::endl);
      DEBUG_MSG("  - Sign Mode: " <<  request->mode() << std::endl);
      if (myDisplay) {
          myDisplay->SetPixelByteSignMode((SignMode)request->mode());
          reply->set_status(reply->OK);
      } else {
          reply->set_status(reply->NOT_OK);
      }
      return Status::OK;
  }

  Status GetFullScaler(ServerContext* context, const GetFullScalerRequest* request,
                       GetFullScalerReply* reply) override {
      DEBUG_MSG("Server got a request for the maximum scaler." << std::endl);
      if (myDisplay) {
          reply->set_fullscaler(myDisplay->GetFullScaler());
      } else {
          reply->set_fullscaler(0);
      }
      return Status::OK;
  }

  Status SetFullScaler(ServerContext* context, const SetFullScalerRequest* request,
                       StatusReply* reply) override {
      DEBUG_MSG("Server got a request to set the maximum scaler." << std::endl);
      DEBUG_MSG("  - Full Scaler: " <<  request->fullscaler() << std::endl);
      if (myDisplay) {
          myDisplay->SetFullScaler((uint16_t)request->fullscaler());
          reply->set_status(reply->OK);
      } else {
          reply->set_status(reply->NOT_OK);
      }
      return Status::OK;
  }

  Status UpdateInputVector(ServerContext* context, const UpdateInputVectorRequest* request,
                           StatusReply* reply) override {
      DEBUG_MSG("Server got a request to update the Input Vector." << std::endl);
      if (myDisplay) {
          DEBUG_MSG("  - Input: ");
          vector<int> input;
          for (int i = 0; i < request->input_size(); i++) {
              input.push_back(request->input(i));
              if (i) { DEBUG_MSG(" "); }
              DEBUG_MSG(request->input(i));
          }
          DEBUG_MSG(std::endl);

          myDisplay->UpdateInputVector(input);
          reply->set_status(reply->OK);
      } else {
          reply->set_status(reply->NOT_OK);
      }
      return Status::OK;
  }

  Status ClearCostModel(ServerContext* context, const ClearCostModelRequest* request,
                        StatusReply* reply) override {
      DEBUG_MSG("Server got a request to clear the cost model." << std::endl);
      if (myDisplay) {
          myDisplay->GetCostModel()->clearCosts();
          reply->set_status(reply->OK);
      } else {
          reply->set_status(reply->NOT_OK);
      }
      return Status::OK;
  }

  Status Latch(ServerContext* context, const LatchRequest* request,
               StatusReply* reply) override {
      DEBUG_MSG("Server got a request to latch." << std::endl);

      sub_x = request->sub_x();
      sub_y = request->sub_y();
      sub_w = request->sub_w();
      sub_h = request->sub_h();

      pthread_mutex_lock(&renderMutex);
      pthread_cond_signal(&renderCondition);
      pthread_mutex_unlock(&renderMutex);

      reply->set_status(reply->OK);
      return Status::OK;
  }

  Status Shutdown(ServerContext* context, const ShutdownRequest* request,
                   StatusReply* reply) override {
      DEBUG_MSG("Server got a request to shutdown." << std::endl);
      alive = false;
      pthread_mutex_lock(&renderMutex);
      pthread_cond_signal(&renderCondition);
      pthread_mutex_unlock(&renderMutex);
      reply->set_status(reply->OK);
      return Status::OK;
  }

  unsigned int inputVectorSize_, frameVolumeDimensionality_;

};

void* runServer(void *) {
  std::string server_address("0.0.0.0:50051");
  NddiServiceImpl service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  server = builder.BuildAndStart();
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

void outputStats() {

    CostModel * costModel = myDisplay->GetCostModel();

    //
    // Print a detailed, readable report if we're not running configHeadless
    //
    cout << endl;

    // General
    //
    cout << "General Information" << endl;
    cout << "  Dimensions: " << myDisplay->DisplayWidth() << " x " << myDisplay->DisplayHeight() << endl;
    cout << "  Coefficient Planes: " << (myDisplay ? myDisplay->NumCoefficientPlanes() : 0) << endl;
    cout << "  Frames Rendered: " << totalUpdates << endl;
    cout << endl;

    // Transmission
    //
    cout << "Transmission Statistics:" << endl;
    // Get total transmission cost
    long totalCost = costModel->getLinkBytesTransmitted();
    cout << "  Total Pixel Data Updated (bytes): " << totalUpdates * myDisplay->DisplayWidth() * myDisplay->DisplayHeight() * BYTES_PER_PIXEL <<
    " Total NDDI Cost (bytes): " << totalCost <<
    " Ratio: " << (double)totalCost / (double)totalUpdates / (double)myDisplay->DisplayWidth() / (double)myDisplay->DisplayHeight() / BYTES_PER_PIXEL << endl;
    cout << endl;


    // Memory
    //
    cout << "Memory Statistics:" << endl;
    cout << "  Input Vector" << endl;
    cout << "    - Num Reads: " << costModel->getReadAccessCount(INPUT_VECTOR_COMPONENT) <<  " - Bytes Read: " << costModel->getBytesRead(INPUT_VECTOR_COMPONENT) << endl;
    cout << "    - Num Writes: " << costModel->getWriteAccessCount(INPUT_VECTOR_COMPONENT) << " - Bytes Written: " << costModel->getBytesWritten(INPUT_VECTOR_COMPONENT) << endl;
    cout << "  Coefficient Plane" << endl;
    cout << "    - Num Reads: " << costModel->getReadAccessCount(COEFFICIENT_PLANE_COMPONENT) <<  " - Bytes Read: " << costModel->getBytesRead(COEFFICIENT_PLANE_COMPONENT) << endl;
    cout << "    - Num Writes: " << costModel->getWriteAccessCount(COEFFICIENT_PLANE_COMPONENT) << " - Bytes Written: " << costModel->getBytesWritten(COEFFICIENT_PLANE_COMPONENT) << endl;
    cout << "  Frame Volume" << endl;
    cout << "    - Num Reads: " << costModel->getReadAccessCount(FRAME_VOLUME_COMPONENT) <<  " - Bytes Read: " << costModel->getBytesRead(FRAME_VOLUME_COMPONENT) << endl;
    cout << "    - Num Writes: " << costModel->getWriteAccessCount(FRAME_VOLUME_COMPONENT) << " - Bytes Written: " << costModel->getBytesWritten(FRAME_VOLUME_COMPONENT) << endl;
    cout << endl;


    // Pixel
    //
    cout << "Pixel Statistics:" << endl;
    cout << "  Pixel Mappings: " << costModel->getPixelsMapped() << endl;
    cout << "  Pixel Blends: " << costModel->getPixelsBlended() << endl;
    cout << endl;

    // Performance
    //
    gettimeofday(&endTime, NULL);
    cout << "Performance Statistics:" << endl;
    cout << "  Average FPS: " << (double)totalUpdates / ((double)(endTime.tv_sec * 1000000
                                                                  + endTime.tv_usec
                                                                  - startTime.tv_sec * 1000000
                                                                  - startTime.tv_usec) / 1000000.0f) << endl;
    cout << endl;

    // CSV
    //

    // Pretty print a heading to stdout, but for headless just spit it to stderr for reference
    cout << "CSV Headings:" << endl;
    cout << "Frames,Commands Sent,Bytes Transmitted,IV Num Reads,IV Bytes Read,IV Num Writes,IV Bytes Written,CP Num Reads,CP Bytes Read,CP Num Writes,CP Bytes Written,FV Num Reads,FV Bytes Read,FV Num Writes,FV Bytes Written,FV Time,Pixels Mapped,Pixels Blended" << endl;

    cout
    << totalUpdates << " , "
    << costModel->getLinkCommandsSent() << " , "
    << costModel->getLinkBytesTransmitted() << " , "
    << costModel->getReadAccessCount(INPUT_VECTOR_COMPONENT) << " , "
    << costModel->getBytesRead(INPUT_VECTOR_COMPONENT) << " , "
    << costModel->getWriteAccessCount(INPUT_VECTOR_COMPONENT) << " , "
    << costModel->getBytesWritten(INPUT_VECTOR_COMPONENT) << " , "
    << costModel->getReadAccessCount(COEFFICIENT_PLANE_COMPONENT) << " , "
    << costModel->getBytesRead(COEFFICIENT_PLANE_COMPONENT) << " , "
    << costModel->getWriteAccessCount(COEFFICIENT_PLANE_COMPONENT) << " , "
    << costModel->getBytesWritten(COEFFICIENT_PLANE_COMPONENT) << " , "
    << costModel->getReadAccessCount(FRAME_VOLUME_COMPONENT) << " , "
    << costModel->getBytesRead(FRAME_VOLUME_COMPONENT) << " , "
    << costModel->getWriteAccessCount(FRAME_VOLUME_COMPONENT) << " , "
    << costModel->getBytesWritten(FRAME_VOLUME_COMPONENT) << " , "
    << costModel->getTime(FRAME_VOLUME_COMPONENT) << " , "
    << costModel->getPixelsMapped() << " , "
    << costModel->getPixelsBlended() << " , "
    << endl;

    cerr << endl;

    // Warnings about Features Configuration
#if defined(SUPRESS_EXCESS_RENDERING) || defined(SKIP_COMPUTE_WHEN_SCALER_ZERO) || !defined(USE_CL) || !defined(USE_GL) || defined(CLEAR_COST_MODEL_AFTER_SETUP)
    cerr << endl << "CONFIGURATION WARNINGS:" << endl;
#ifdef SUPRESS_EXCESS_RENDERING
    cerr << "  - Was compiled with SUPRESS_EXCESS_RENDERING, and so the numbers may be off. Reconfig with \"cmake -DHACKS=off ...\"." << endl;
#endif
#if (defined SKIP_COMPUTE_WHEN_SCALER_ZERO) && !(defined USE_OMP)
    cerr << "  - Was compiled with SKIP_COMPUTE_WHEN_SCALER_ZERO, and so the numbers may be off when running without USE_OMP." << endl <<
            "    When using OpenMP, the number will be fine regardless because they're register in bulk later. Reconfig" << endl <<
            "    with \"cmake -DHACKS=off ...\"." << endl;
#endif
#ifndef USE_OMP
    cerr << "  - Was compiled without OpenMP." << endl;
#endif
#ifndef USE_CL
    cerr << "  - Was compiled without OpenCL." << endl;
#endif
#ifndef USE_GL
    cerr << "  - Was compiled without OpenGL, and so rendering was only simulated Reconfig with \"cmake -DUSE_GL=on ...\"" << endl <<
            "    if you need accurage numbers for the rendering operations." << endl;
#endif
#ifdef CLEAR_COST_MODEL_AFTER_SETUP
    cerr << "  - Was compiled with CLEAR_COST_MODEL_AFTER_SETUP, affecting the true cost for PixelBridge experiments." << endl;
#endif
#endif
}

void renderFrame() {

    if (alive) {
        pthread_mutex_lock(&renderMutex);
        pthread_cond_wait(&renderCondition, &renderMutex);
#ifdef USE_GL
        glutPostRedisplay();
#else
        if (myDisplay) { myDisplay->SimulateRender(sub_x, sub_y, sub_w, sub_h); }
#endif
        totalUpdates++;
        pthread_mutex_unlock(&renderMutex);
    } else {
        server->completion_queue()->Shutdown();
        server->Shutdown();
        pthread_join(serverThread, NULL);
        outputStats();
        delete myDisplay;
        exit(0);
    }
}

#ifdef USE_GL
void draw( void ) {

    if (!myDisplay)
        return;

    // Grab the frame buffer from the NDDI display
    GLuint texture = myDisplay->GetFrameBufferTex(sub_x, sub_y, sub_w, sub_h);

// TODO(CDE): Temporarily putting this here until GlNddiDisplay and ClNddiDisplay
//            are using the exact same kind of GL textures
#ifdef USE_CL
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT );

    // Draw the texture with ARB Rectangles
    glEnable(GL_TEXTURE_RECTANGLE_ARB);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texture );

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-1.0, 1.0);
    glTexCoord2f(0, myDisplay->DisplayHeight()); glVertex2f(-1.0, -1.0);
    glTexCoord2f(myDisplay->DisplayWidth(), myDisplay->DisplayHeight()); glVertex2f(1.0, -1.0);
    glTexCoord2f(myDisplay->DisplayWidth(), 0); glVertex2f(1.0, 1.0);
    glEnd();

    glDisable(GL_TEXTURE_RECTANGLE_ARB);
#else
    glBindTexture( GL_TEXTURE_2D, texture );

    glBegin( GL_QUADS );
    glTexCoord2d(0.0,0.0); glVertex2d(-1.0,1.0);
    glTexCoord2d(1.0,0.0); glVertex2d(1.0,1.0);
    glTexCoord2d(1.0,1.0); glVertex2d(1.0,-1.0);
    glTexCoord2d(0.0,1.0); glVertex2d(-1.0,-1.0);
    glEnd();

    glBindTexture( GL_TEXTURE_2D, 0 );
#endif

    // Update the window
    glutSwapBuffers();
}

void keyboard( unsigned char key, int x, int y ) {

    switch (key) {
        case 27: // Esc
            alive = false;
            server->completion_queue()->Shutdown();
            server->Shutdown();
            pthread_join(serverThread, NULL);
            delete myDisplay;
            exit(0);
            break;
        default:
            break;
    }

    glutPostRedisplay();
}

static bool doing_it = false;

void mouse( int button, int state, int x, int y ) {

    if (button == GLUT_LEFT && state == GLUT_DOWN) {
        doing_it = true;
    } else {
        doing_it = false;
    }
    glutPostRedisplay();
}

void motion( int x, int y ) {

    if (doing_it) {
        glutPostRedisplay();
    }
}
#endif

int main(int argc, char** argv) {

  alive = true;

  pthread_create(&serverThread, NULL, runServer, NULL);

  // Wait until the server initializes the NDDI display for a client.
  while (!myDisplay)
    usleep(200);

#ifdef USE_GL
  // Initialize GLUT
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
  glutInitWindowSize(myDisplay->DisplayWidth(), myDisplay->DisplayHeight());

  glutCreateWindow("NDDI Display Wall");

  glutDisplayFunc(draw);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);

  glEnable(GL_TEXTURE_2D);

  glutIdleFunc(renderFrame);

  // Take the start time stamp
  gettimeofday(&startTime, NULL);

  // Run main loop
  glutMainLoop();
#else
  // Take the start time stamp
  gettimeofday(&startTime, NULL);
  while (true) {
      renderFrame();
  }
#endif

  return 0;
}
