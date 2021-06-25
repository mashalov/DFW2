cmake_minimum_required(VERSION 3.16)
project(ResultFile)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../fmt/include/" )

set(SOURCES 
    ../../fmt/src/format.cc 
    BitStream.cpp
    Compressor.cpp
    CSVWriter.cpp
    Device.cpp
    DeviceCollection.cpp
    Devices.cpp
    DeviceType.cpp
    DeviceTypes.cpp
    DeviceTypeWrite.cpp
    dllmain.cpp
    FileExceptions.cpp
    Result.cpp
    ResultFile.cpp
    ResultFileReader.cpp
    ResultFileWriter.cpp
    ResultRead.cpp
    ResultWrite.cpp
    RLECompressor.cpp
    SlowVariable.cpp
    SlowVariableItem.cpp
    SlowVariables.cpp
    stdafx.cpp
    UnicodeSCSU.cpp
    Variable.cpp
    Variables.cpp
    VariablesCollection.cpp
    )             

add_library(ResultFile ${SOURCES})
target_include_directories(ResultFile PRIVATE ${INCLUDES}) 

