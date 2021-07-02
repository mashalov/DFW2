cmake_minimum_required(VERSION 3.16)
project(ResultFile)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#add_compile_options(-march=native)	

set(INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../fmt/include/" )

set(SOURCES 
    ../../fmt/src/format.cc 
    BitStream.cpp
    Compressor.cpp
#    CSVWriter.cpp
#    Device.cpp
#    DeviceCollection.cpp
#    Devices.cpp
#    DeviceType.cpp
#    DeviceTypes.cpp
#    DeviceTypeWrite.cpp
#    dllmain.cpp
    FileExceptions.cpp
    Result.cpp
    ResultFile.cpp
#    ResultFileReader.cpp
    ResultFileWriter.cpp
#    ResultRead.cpp
#    ResultWrite.cpp
    RLECompressor.cpp
#    SlowVariable.cpp
#    SlowVariableItem.cpp
#    SlowVariables.cpp
    UnicodeSCSU.cpp
    Variable.cpp
    Variables.cpp
    VariablesCollection.cpp
    )             

add_library(ResultFile SHARED ${SOURCES})
target_include_directories(ResultFile PRIVATE ${INCLUDES}) 

find_library(dl NAMES dl PATH /usr/lib/x86_x64-linux-gnu/libdl.so)
target_link_libraries(ResultFile  dl)
