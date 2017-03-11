project(cpp_serializers)
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Doesn't support non 64-bit platforms")
endif()

include(FindThreads)
include(CheckTypeSize)
include(CheckIncludeFiles)
include(CheckCXXCompilerFlag)
include(ExternalProject)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

CHECK_CXX_COMPILER_FLAG("-std=c++11" CXX11)
if (NOT CXX11)
    message(FATAL_ERROR "C++ compiler doesn't support C++11")
endif()

CHECK_INCLUDE_FILES("inttypes.h" HAVE_INTTYPES_H)
CHECK_INCLUDE_FILES("netinet/in.h" HAVE_NETINET_IN_H)

if (HAVE_INTTYPES_H)
    add_definitions("-DHAVE_INTTYPES_H")
endif()

if (HAVE_NETINET_IN_H)
    add_definitions("-DHAVE_NETINET_IN_H")
endif()

include_directories(${cpp_serializers_SOURCE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W")


# need for building capnp
find_program(AUTORECONF NAMES autoreconf)
if (NOT AUTORECONF)
    message(FATAL_ERROR "Autoreconf not found")
else()
    message(STATUS "Found autoreconf: ${AUTORECONF}")
endif()

find_program(LIBTOOL NAMES libtool libtoolize)
if (NOT LIBTOOL)
    message(FATAL_ERROR "Libtool not found")
else()
    message(STATUS "Found libtool: ${LIBTOOL}")
endif()


# protobuf
set(protobuf_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external/protobuf)
ExternalProject_Add(
    protobuf
    PREFIX ${protobuf_PREFIX}
    URL "https://github.com/google/protobuf/releases/download/v3.1.0/protobuf-cpp-3.1.0.tar.gz"
    URL_MD5 "bd5e3eed635a8d32e2b99658633815ef"
    CONFIGURE_COMMAND CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} ${protobuf_PREFIX}/src/protobuf/configure --prefix=${protobuf_PREFIX} --libdir=${protobuf_PREFIX}/lib --enable-shared=no
    BUILD_COMMAND $(MAKE)
    INSTALL_COMMAND $(MAKE) install
    BUILD_IN_SOURCE 1
    LOG_UPDATE ON
    LOG_BUILD ON
    LOG_INSTALL ON
)
include_directories(${protobuf_PREFIX}/include)
set(PROTOBUF_LIBRARIES ${protobuf_PREFIX}/lib/libprotobuf.a)
set(PROTOBUF_GENERATOR ${protobuf_PREFIX}/bin/protoc)


#capnproto
set(capnproto_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external/capnproto)
ExternalProject_Add(
    capnproto
    PREFIX ${capnproto_PREFIX}
    URL "https://github.com/sandstorm-io/capnproto/archive/v0.5.3.tar.gz"
    URL_MD5 "909bd13ad6b8bc840ac78ab8f5bcb0a4"
    CONFIGURE_COMMAND CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} cd ${capnproto_PREFIX}/src/capnproto/c++ && ${AUTORECONF} -i && cd - && ${capnproto_PREFIX}/src/capnproto/c++/configure --prefix=${capnproto_PREFIX} --libdir=${capnproto_PREFIX}/lib --enable-shared=no
    BUILD_COMMAND $(MAKE)
    INSTALL_COMMAND $(MAKE) install
    BUILD_IN_SOURCE 1
    LOG_UPDATE ON
    LOG_BUILD ON
    LOG_INSTALL ON
)
include_directories(${capnproto_PREFIX}/include)
set(CAPNPROTO_LIBRARIES ${capnproto_PREFIX}/lib/libcapnp.a ${capnproto_PREFIX}/lib/libkj.a)
set(CAPNPROTO_GENERATOR ${capnproto_PREFIX}/bin/capnp)
set(CAPNPROTO_CPP_GENERATOR ${capnproto_PREFIX}/bin/capnpc-c++)

# flatbuffers
set(flatbuffers_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external/flatbuffers)
ExternalProject_Add(
    flatbuffers
    PREFIX ${flatbuffers_PREFIX}
    URL "https://github.com/google/flatbuffers/archive/v1.4.0.tar.gz"
    URL_MD5 "03d5b0e6233362b0951f22374d36b553"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${flatbuffers_PREFIX} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    LOG_UPDATE ON
    LOG_CONFIGURE ON
    LOG_BUILD ON
)
include_directories(${flatbuffers_PREFIX}/include)
set(FLATBUFFERS_LIBRARIES ${flatbuffers_PREFIX}/lib/libflatbuffers.a)
set(FLATBUFFERS_GENERATOR ${flatbuffers_PREFIX}/bin/flatc)


# msgpack
set(msgpack_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external/msgpack)
ExternalProject_Add(
    msgpack
    PREFIX ${msgpack_PREFIX}
    URL "https://github.com/msgpack/msgpack-c/releases/download/cpp-2.0.0/msgpack-2.0.0.tar.gz"
    URL_MD5 "55148cd856c72f954a6eb9cc889a7d2a"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${msgpack_PREFIX} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    BUILD_IN_SOURCE 1
    LOG_UPDATE ON
    LOG_BUILD ON
    LOG_INSTALL ON
)
include_directories(${msgpack_PREFIX}/include)
set(MSGPACK_LIBRARIES ${msgpack_PREFIX}/lib/libmsgpackc.a)


set(LINKLIBS

    ${MSGPACK_LIBRARIES}
    ${PROTOBUF_LIBRARIES}
    ${CAPNPROTO_LIBRARIES}
    ${FLATBUFFERS_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}

)


add_custom_command(
    DEPENDS ${cpp_serializers_SOURCE_DIR}/record.proto
    COMMAND ${PROTOBUF_GENERATOR}
    ARGS -I${cpp_serializers_SOURCE_DIR} --cpp_out=${cpp_serializers_SOURCE_DIR}/protobuf ${cpp_serializers_SOURCE_DIR}/record.proto
    OUTPUT  "${cpp_serializers_SOURCE_DIR}/protobuf/record.pb.cc"
    COMMENT "Executing Protobuf compiler"
)
set_source_files_properties(
    ${cpp_serializers_SOURCE_DIR}/protobuf/record.pb.cc
    ${cpp_serializers_SOURCE_DIR}/protobuf/record.pb.h
    PROPERTIES GENERATED TRUE
)
set(PROTOBUF_SERIALIZATION_SOURCES  ${cpp_serializers_SOURCE_DIR}/protobuf/record.pb.cc)

add_custom_command(
    DEPENDS ${cpp_serializers_SOURCE_DIR}/record.capnp
    COMMAND ${CAPNPROTO_GENERATOR}
    ARGS compile -I${cpp_serializers_SOURCE_DIR} --src-prefix=${cpp_serializers_SOURCE_DIR} -o${CAPNPROTO_CPP_GENERATOR}:${cpp_serializers_SOURCE_DIR}/capnproto ${cpp_serializers_SOURCE_DIR}/record.capnp
    OUTPUT  "${cpp_serializers_SOURCE_DIR}/capnproto/record.capnp.c++"
    COMMENT "Executing Cap'n Proto compiler"
)
set_source_files_properties(
    ${cpp_serializers_SOURCE_DIR}/capnproto/record.capnp.c++
    ${cpp_serializers_SOURCE_DIR}/capnproto/record.capnp.h
    PROPERTIES GENERATED TRUE
)
set(CAPNPROTO_SERIALIZATION_SOURCES  ${cpp_serializers_SOURCE_DIR}/capnproto/record.capnp.c++)


add_custom_command(
    DEPENDS ${cpp_serializers_SOURCE_DIR}/record.fbs
    COMMAND ${FLATBUFFERS_GENERATOR}
    ARGS --cpp -o ${cpp_serializers_SOURCE_DIR}/flatbuffers ${cpp_serializers_SOURCE_DIR}/record.fbs
    OUTPUT "${cpp_serializers_SOURCE_DIR}/flatbuffers/record_generated.h"
    COMMENT "Executing FlatBuffers compiler"
)
set_source_files_properties(
    ${cpp_serializers_SOURCE_DIR}/flatbuffers/record_generated.h
    PROPERTIES GENERATED TRUE
)
set(FLATBUFFERS_SERIALIZATION_SOURCES ${cpp_serializers_SOURCE_DIR}/flatbuffers/record_generated.h)



add_executable(
    benchmark
    ${cpp_serializers_SOURCE_DIR}/benchmark.cpp
    ${PROTOBUF_SERIALIZATION_SOURCES}
    ${CAPNPROTO_SERIALIZATION_SOURCES}
    ${FLATBUFFERS_SERIALIZATION_SOURCES}
)
add_dependencies(benchmark msgpack protobuf capnproto flatbuffers)
target_link_libraries(benchmark ${LINKLIBS})
set_target_properties(benchmark PROPERTIES COMPILE_FLAGS "-std=c++11")
