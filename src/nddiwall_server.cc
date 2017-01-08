/*
 *
 * Copyright 2015, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <grpc++/grpc++.h>

#include "nddi/Features.h"
#include "nddi/GlNddiDisplay.h"

#include "nddiwall.grpc.pb.h"

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
using nddiwall::FillCoefficientMatrixRequest;
using nddiwall::FillScalerRequest;
using nddiwall::GetFullScalerRequest;
using nddiwall::GetFullScalerReply;
using nddiwall::SetFullScalerRequest;
using nddiwall::NddiWall;

/*
 * Globals
 */
GlNddiDisplay* myDisplay;
pthread_t serverThread;

// Logic and data behind the server's behavior.
class NddiServiceImpl final : public NddiWall::Service {

  Status Initialize(ServerContext* context, const InitializeRequest* request,
                    StatusReply* reply) override {
    std::cout << "Server got a request to initialize an NDDI Display." << std::endl;
    if (!myDisplay) {
        inputVectorSize_ = request->inputvectorsize();
        frameVolumeDimensionality_ = request->framevolumedimensionalsizes_size();

        std::cout << "  - Frame Volume: ";
        vector<unsigned int> fvDimensions;
        for (int i = 0; i < request->framevolumedimensionalsizes_size(); i++) {
            fvDimensions.push_back(request->framevolumedimensionalsizes(i));
            if (i) { std::cout << "x"; }
            std::cout << request->framevolumedimensionalsizes(i);
        }
        std::cout << std::endl;
        std::cout << "  - Display: " << request->displaywidth() << "x" << request->displayheight() << std::endl;
        std::cout << "  - Coefficient Planes: " << request->numcoefficientplanes() << std::endl;
        std::cout << "  - Input Vector Size: " << request->inputvectorsize() << std::endl;

        // Initialize the NDDI display
        myDisplay = new GlNddiDisplay(fvDimensions,                    // framevolume dimensional sizes
                                      request->displaywidth(),         // display size
                                      request->displayheight(),
                                      request->numcoefficientplanes(), // number of coefficient planes on the display
                                      request->inputvectorsize());     // input vector size (x, y, t)

        reply->set_status(reply->OK);
    } else {
        reply->set_status(reply->NOT_OK);
    }
    return Status::OK;
  }

  Status DisplayWidth(ServerContext* context, const DisplayWidthRequest* request,
                      DisplayWidthReply* reply) override {
      std::cout << "Server got a request for the NDDI Display width." << std::endl;
      if (myDisplay) {
          reply->set_width(myDisplay->DisplayWidth());
      } else {
          reply->set_width(0);
      }
      return Status::OK;
  }

  Status DisplayHeight(ServerContext* context, const DisplayHeightRequest* request,
                       DisplayHeightReply* reply) override {
      std::cout << "Server got a request for the NDDI Display height." << std::endl;
      if (myDisplay) {
          reply->set_height(myDisplay->DisplayHeight());
      } else {
          reply->set_height(0);
      }
      return Status::OK;
  }

  Status NumCoefficientPlanes(ServerContext* context, const NumCoefficientPlanesRequest* request,
                              NumCoefficientPlanesReply* reply) override {
      std::cout << "Server got a request for the NDDI Display number of coefficient planes." << std::endl;
      if (myDisplay) {
          reply->set_planes(myDisplay->NumCoefficientPlanes());
      } else {
          reply->set_planes(0);
      }
      return Status::OK;
  }

  Status PutPixel(ServerContext* context, const PutPixelRequest* request,
                  StatusReply* reply) override {
      std::cout << "Server got a request to PutPixel." << std::endl;
      if (myDisplay) {
          std::cout << "  - Location: (";
          vector<unsigned int> location;
          for (int i = 0; i < request->location_size(); i++) {
              location.push_back(request->location(i));
              if (i) { std::cout << ","; }
              std::cout << request->location(i);
          }
          std::cout << ")" << std::endl;

          Pixel p;
          p.packed = request->pixel();
          std::cout << "  - Pixel: " << (uint32_t)p.r << " " << (uint32_t)p.g << " " << (uint32_t)p.b << " " << (uint32_t)p.a << std::endl;

          myDisplay->PutPixel(p, location);

          reply->set_status(reply->OK);
      } else {
          reply->set_status(reply->NOT_OK);
      }
      return Status::OK;
  }

  Status FillPixel(ServerContext* context, const FillPixelRequest* request,
                  StatusReply* reply) override {
      std::cout << "Server got a request to FillPixel." << std::endl;
      if (myDisplay) {
          std::cout << "  - Start: (";
          vector<unsigned int> start;
          for (int i = 0; i < request->start_size(); i++) {
              start.push_back(request->start(i));
              if (i) { std::cout << ","; }
              std::cout << request->start(i);
          }
          std::cout << ")" << std::endl;

          std::cout << "  - End: (";
          vector<unsigned int> end;
          for (int i = 0; i < request->end_size(); i++) {
              end.push_back(request->end(i));
              if (i) { std::cout << ","; }
              std::cout << request->end(i);
          }
          std::cout << ")" << std::endl;

          Pixel p;
          p.packed = request->pixel();
          std::cout << "  - Pixel: " << (uint32_t)p.r << " " << (uint32_t)p.g << " " << (uint32_t)p.b << " " << (uint32_t)p.a << std::endl;

          myDisplay->FillPixel(p, start, end);

          reply->set_status(reply->OK);
      } else {
          reply->set_status(reply->NOT_OK);
      }
      return Status::OK;
  }

  Status FillCoefficientMatrix(ServerContext* context, const FillCoefficientMatrixRequest* request,
                               StatusReply* reply) override {
      std::cout << "Server got a request to FillCoefficientMatrix." << std::endl;
      if (myDisplay) {
          std::cout << "  - Coefficient Matrix:" << std::endl;
          vector< vector<int> > coefficientMatrix;
          assert(request->coefficientmatrix_size() == inputVectorSize_ * frameVolumeDimensionality_);
          coefficientMatrix.resize(frameVolumeDimensionality_);
          for (int j = 0; j < frameVolumeDimensionality_; j++) {
              std::cout << "    ";
              for (int i = 0; i < inputVectorSize_; i++) {
                  coefficientMatrix[j].push_back(request->coefficientmatrix(j * frameVolumeDimensionality_ + i));
                  std::cout << request->coefficientmatrix(j * frameVolumeDimensionality_ + i) << " ";
              }
              std::cout << std::endl;
          }

          std::cout << "  - Start: (";
          vector<unsigned int> start;
          for (int i = 0; i < request->start_size(); i++) {
              start.push_back(request->start(i));
              if (i) { std::cout << ","; }
              std::cout << request->start(i);
          }
          std::cout << ")" << std::endl;

          std::cout << "  - End: (";
          vector<unsigned int> end;
          for (int i = 0; i < request->end_size(); i++) {
              end.push_back(request->end(i));
              if (i) { std::cout << ","; }
              std::cout << request->end(i);
          }
          std::cout << ")" << std::endl;

          myDisplay->FillCoefficientMatrix(coefficientMatrix, start, end);

          reply->set_status(reply->OK);
      } else {
          reply->set_status(reply->NOT_OK);
      }
      return Status::OK;
  }

  Status FillScaler(ServerContext* context, const FillScalerRequest* request,
                  StatusReply* reply) override {
      std::cout << "Server got a request to FillScaler." << std::endl;
      if (myDisplay) {
          std::cout << "  - Start: (";
          vector<unsigned int> start;
          for (int i = 0; i < request->start_size(); i++) {
              start.push_back(request->start(i));
              if (i) { std::cout << ","; }
              std::cout << request->start(i);
          }
          std::cout << ")" << std::endl;

          std::cout << "  - End: (";
          vector<unsigned int> end;
          for (int i = 0; i < request->end_size(); i++) {
              end.push_back(request->end(i));
              if (i) { std::cout << ","; }
              std::cout << request->end(i);
          }
          std::cout << ")" << std::endl;

          Scaler s;
          s.packed = request->scaler();
          std::cout << "  - Scaler: " << s.r << " " << s.g << " " << s.r << " " << s.a << std::endl;

          myDisplay->FillScaler(s, start, end);

          reply->set_status(reply->OK);
      } else {
          reply->set_status(reply->NOT_OK);
      }
      return Status::OK;
  }

  Status GetFullScaler(ServerContext* context, const GetFullScalerRequest* request,
                       GetFullScalerReply* reply) override {
      std::cout << "Server got a request for the maximum scaler." << std::endl;
      if (myDisplay) {
          reply->set_fullscaler(myDisplay->GetFullScaler());
      } else {
          reply->set_fullscaler(0);
      }
      return Status::OK;
  }

  Status SetFullScaler(ServerContext* context, const SetFullScalerRequest* request,
                       StatusReply* reply) override {
      std::cout << "Server got a request to set the maximum scaler." << std::endl;
      std::cout << "  - Full Scaler: " <<  request->fullscaler() << std::endl;
      if (myDisplay) {
          myDisplay->SetFullScaler((uint16_t)request->fullscaler());
          reply->set_status(reply->OK);
      } else {
          reply->set_status(reply->NOT_OK);
      }
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
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

void renderFrame() {
    // Draw
    glutPostRedisplay();
}

void draw( void ) {

    if (!myDisplay)
        return;

    // Grab the frame buffer from the NDDI display
    GLuint texture = myDisplay->GetFrameBufferTex();

// TODO(CDE): Temporarily putting this here until GlNddiDisplay and ClNddiDisplay
//            are using the exact same kind of GL textures
#ifndef NO_CL
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
            pthread_cancel(serverThread);
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

int main(int argc, char** argv) {

  //RunServer();
  pthread_create(&serverThread, NULL, runServer, NULL);

  // Wait until the server initializes the NDDI display for a client.
  while (!myDisplay)
    usleep(200);

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

  // Run main loop
  glutMainLoop();

  return 0;
}
