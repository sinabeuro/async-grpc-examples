cmake_minimum_required(VERSION 2.8.12)  # Ships with Ubuntu 14.04 (Trusty)
set (CMAKE_CXX_STANDARD 14)

project(async-grpc-examples)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

include_directories(src)

set(ALL_SRCS
    src/client.cpp
    src/main.cpp)

set(ALL_PROTOS
    proto/math_service.proto)

set(ALL_PROTO_SRCS)
set(ALL_PROTO_HDRS)
foreach(RELATIVEPATH ${ALL_PROTOS})
  get_filename_component(DIR ${RELATIVEPATH} DIRECTORY)
  get_filename_component(FILENAME ${RELATIVEPATH} NAME_WE)

  list(APPEND ALL_PROTO_SRCS "${PROJECT_BINARY_DIR}/${DIR}/${FILENAME}.pb.cc")
  list(APPEND ALL_PROTO_HDRS "${PROJECT_BINARY_DIR}/${DIR}/${FILENAME}.pb.h")

  add_custom_command(
    OUTPUT "${PROJECT_BINARY_DIR}/${DIR}/${FILENAME}.pb.cc"
           "${PROJECT_BINARY_DIR}/${DIR}/${FILENAME}.pb.h"
    COMMAND /usr/local/bin/protoc
    ARGS --cpp_out  ${PROJECT_BINARY_DIR} -I
      ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/${RELATIVEPATH}

    DEPENDS ${RELATIVEPATH}
    COMMENT "Running C++ protocol buffer compiler on ${RELATIVEPATH}"
    VERBATIM
  )
endforeach()

foreach(RELATIVEPATH ${ALL_PROTOS})
  get_filename_component(DIR ${RELATIVEPATH} DIRECTORY)
  get_filename_component(FILENAME ${RELATIVEPATH} NAME_WE)

  list(APPEND ALL_PROTO_SRCS "${PROJECT_BINARY_DIR}/${DIR}/${FILENAME}.grpc.pb.cc")
  list(APPEND ALL_PROTO_HDRS "${PROJECT_BINARY_DIR}/${DIR}/${FILENAME}.grpc.pb.h")

  execute_process(COMMAND /usr/local/bin/protoc -I=${PROJECT_SOURCE_DIR}
          --cpp_out=${PROJECT_BINARY_DIR}
          --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin
          --grpc_out=${PROJECT_BINARY_DIR}
          ${RELATIVEPATH}
          WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
endforeach()

set_source_files_properties(${ALL_PROTO_SRCS} ${ALL_PROTO_HDRS} PROPERTIES GENERATED TRUE)

add_executable(${PROJECT_NAME} ${ALL_SRCS} ${ALL_PROTO_HDRS} ${ALL_PROTO_SRCS})

target_link_libraries(${PROJECT_NAME} PUBLIC glog)
target_link_libraries(${PROJECT_NAME} PUBLIC gflags)

target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC
  ${Protobuf_INCLUDE_DIR})
# TODO(hrapp): This should not explicitly list pthread and use
# PROTOBUF_LIBRARIES, but that failed on first try.
target_link_libraries(${PROJECT_NAME} PUBLIC ${Protobuf_LIBRARY} pthread)

# TODO(cschuet): Write FindGRPC.cmake
target_link_libraries(${PROJECT_NAME} PUBLIC gRPC::grpc++)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

set(TARGET_COMPILE_FLAGS "${TARGET_COMPILE_FLAGS} ${GOOG_CXX_FLAGS}")
set_target_properties(${PROJECT_NAME} PROPERTIES
  COMPILE_FLAGS ${TARGET_COMPILE_FLAGS})

install(
  TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION bin
)
