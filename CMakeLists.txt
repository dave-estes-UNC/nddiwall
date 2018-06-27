cmake_minimum_required(VERSION 3.2)
project(nddiwall)

find_package(Protobuf REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(OpenCV REQUIRED)
find_package(OpenMP REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS} -ffast-math -std=c++11 -DGL_GLEXT_PROTOTYPES")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -DDEBUG")
if (NOT NO_HACKS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSUPRESS_EXCESS_RENDERING -DSKIP_COMPUTE_WHEN_SCALER_ZERO")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DSUPRESS_EXCESS_RENDERING -DSKIP_COMPUTE_WHEN_SCALER_ZERO")
endif(NOT NO_HACKS)

include_directories(/usr/local/include ${OPENGL_INCLUDE_DIR} ${GLUT_INCLUDE_DIRS} /opt/AMDAPP/include/ ${OpenCV_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/src/cereal/include)
link_directories(/usr/local/lib ${OPENGL_LIBRARIES} /opt/AMDAPP/lib/x86_64 /opt/AMDAPP/lib/x86)

set(PROTO_PATH "${CMAKE_SOURCE_DIR}/protos")
set(NDDIWALL_PROTO "${PROTO_PATH}/nddiwall.proto")
set(GENERATED_PROTOBUF_PATH "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_PROTOBUF_PATH})

set(NDDIWALL_PB_CPP_FILE "${GENERATED_PROTOBUF_PATH}/nddiwall.pb.cc")
set(NDDIWALL_PB_H_FILE "${GENERATED_PROTOBUF_PATH}/nddiwall.pb.h")
set(NDDIWALL_GRPC_PB_CPP_FILE "${GENERATED_PROTOBUF_PATH}/nddiwall.grpc.pb.cc")
set(NDDIWALL_GRPC_PB_H_FILE "${GENERATED_PROTOBUF_PATH}/nddiwall.grpc.pb.h")
add_custom_command(
    OUTPUT "${NDDIWALL_PB_H_FILE}"
           "${NDDIWALL_PB_CPP_FILE}"
           "${NDDIWALL_GRPC_PB_H_FILE}"
           "${NDDIWALL_GRPC_PB_CPP_FILE}"
    COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
    ARGS "--proto_path=${PROTO_PATH}"
         "--cpp_out=${GENERATED_PROTOBUF_PATH}"
         "${NDDIWALL_PROTO}"
    COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
    ARGS "--proto_path=${PROTO_PATH}"
         "--grpc_out=${GENERATED_PROTOBUF_PATH}"
         "--plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin"
         "${NDDIWALL_PROTO}"
    )
set(GENERATED_PROTOBUF_FILES ${NDDIWALL_PB_H_FILE} ${NDDIWALL_PB_CPP_FILE}
    ${NDDIWALL_GRPC_PB_H_FILE} ${NDDIWALL_GRPC_PB_CPP_FILE})

include_directories(${GENERATED_PROTOBUF_PATH})
link_libraries(grpc++_unsecure grpc gpr ${PROTOBUF_LIBRARY} pthread dl ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} glut OpenCL z avutil avformat avcodec swscale ${OpenCV_LIBS})

file(GLOB_RECURSE NDDI_SRC_FILES ${PROJECT_SOURCE_DIR}/src/nddi/*.cpp)
set(PIXELBRIDGE_SRC_FILES src/PixelBridgeMain.cpp src/GrpcNddiDisplay.cpp src/CachedTiler.cpp src/DctTiler.cpp src/FfmpegPlayer.cpp src/FlatTiler.cpp src/ItTiler.cpp src/MultiDctTiler.cpp src/RandomPlayer.cpp src/Rewinder.cpp src/ScaledDctTiler.cpp)

add_executable(nddiwall_server src/GrpcNddiDisplay.cpp src/NddiWallServer.cpp ${NDDI_SRC_FILES} ${GENERATED_PROTOBUF_FILES})
add_executable(nddiwall_test_client src/GrpcNddiDisplay.cpp src/NddiWallTestClient.cpp ${NDDI_SRC_FILES} ${GENERATED_PROTOBUF_FILES})
add_executable(nddiwall_player_client src/GrpcNddiDisplay.cpp src/NddiWallPlayer.cpp ${NDDI_SRC_FILES} ${GENERATED_PROTOBUF_FILES})
add_executable(nddiwall_pixelbridge_client src/GrpcNddiDisplay.cpp ${PIXELBRIDGE_SRC_FILES} ${NDDI_SRC_FILES} ${GENERATED_PROTOBUF_FILES})
add_executable(nddiwall_master_client src/GrpcNddiDisplay.cpp src/NddiWallMasterClient.cpp ${NDDI_SRC_FILES} ${GENERATED_PROTOBUF_FILES})
