cmake_minimum_required(VERSION 3.16)
project(DFW2DEPS)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#include(ExternalProject)

set(LIBFOLDER ${CMAKE_CURRENT_SOURCE_DIR}/../libs)
set(GRPC_DIR ${LIBFOLDER}/grpc)

add_subdirectory("DFW2" "${CMAKE_CURRENT_BINARY_DIR}/Build")

#execute_process(COMMAND git clone -b CRYPTOPP_8_2_0 https://github.com/weidai11/cryptopp.git ${LIBFOLDER}/cryptopp)
#execute_process(COMMAND git clone -b CRYPTOPP_8_2_0 https://github.com/noloader/cryptopp-cmake.git ${LIBFOLDER}/cryptopp-cmake)
#execute_process(COMMAND git clone -b 4.9.2 https://github.com/antlr/antlr4.git ${LIBFOLDER}/antlr4)
#execute_process(COMMAND git clone -bv1.45.1 --recurse-submodules --shallow-submodules git@github.com:grpc/grpc.git ${GRPC_DIR}
#execute_process(COMMAND git clone -b 1.45.1 https://github.com/grcp/grpc.git ${GRPC_DIR})

#set(GRPC_INSTALL_DIR ${GRPC_DIR}/.grpc)

#file(COPY ${LIBFOLDER}/cryptopp-cmake/CMakeLists.txt 
#          ${LIBFOLDER}/cryptopp-cmake/cryptopp-config.cmake 
#     DESTINATION ${LIBFOLDER}/cryptopp
#)

#message(STATUS "grpc install dir:" ${GRPC_INSTALL_DIR})

#add_subdirectory(${LIBFOLDER}/cryptopp ${LIBFOLDER}/cryptopp/build)
#add_subdirectory(${LIBFOLDER}/antlr4/runtime/Cpp ${LIBFOLDER}/antlr4/build)

#ExternalProject_Add(gRPC
#  PREFIX grpc
#  SOURCE_DIR ${GRPC_DIR}
#  CMAKE_CACHE_ARGS
#        -DgRPC_BUILD_TESTS:BOOL=OFF
#        -DgRPC_INSTALL:BOOL=ON
#        -DCARES_STATIC_PIC:BOOL=ON
#        -CMAKE_INSTALL_PREFIX:PATH=${GRPC_INSTALL_DIR}
#)







