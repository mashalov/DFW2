cmake_minimum_required(VERSION 3.16)
project(EMSServer)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_compile_definitions(DLONG)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set_directory_properties(PROPERTIES COMPILE_DEFINITIONS_DEBUG "_DEBUG")


set(DFW_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../fmt/include/"
                 "${CMAKE_CURRENT_SOURCE_DIR}/../njson/")

set(KLU_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/SuiteSparse/KLU/Include" 
             "${CMAKE_CURRENT_SOURCE_DIR}/SuiteSparse/AMD/Include"
             "${CMAKE_CURRENT_SOURCE_DIR}/SuiteSparse/COLAMD/Include"
             "${CMAKE_CURRENT_SOURCE_DIR}/SuiteSparse/BTF/Include"
             "${CMAKE_CURRENT_SOURCE_DIR}/SuiteSparse/CSparse/Include"
             "${CMAKE_CURRENT_SOURCE_DIR}/SuiteSparse/SuiteSparse_config/")

set(EMSSSOURCES 
    main.cpp 
    ../../fmt/src/format.cc 
    Messages.cpp
    LoadFlow.cpp
    Device.cpp
    DeviceContainer.cpp
    DynaNode.cpp
    DynaBranch.cpp
    DeviceId.cpp
    DynaLRC.cpp
    DeviceContainerProperties.cpp
    Serializer.cpp
    DynaModelUtility.cpp
    NetworkGraph.cpp
    IterationControl.cpp
    Automatic.cpp
    Discontinuities.cpp
    DynaModel.cpp
    Matrix.cpp
    DynaPrimitive.cpp
    CustomDevice.cpp
    CustomDeviceContainer.cpp
    CustomDeviceDLL.cpp
    DynaPowerInjector.cpp
    DynaVoltageSource.cpp
    DynaGeneratorInfBus.cpp
    DynaGeneratorMotion.cpp
    DynaGeneratorMustang.cpp
    DynaGenerator1C.cpp
    DynaGenerator3C.cpp
    DynaDECMustang.cpp
    DynaExcConMustang.cpp
    DynaExciterBase.cpp
    DynaExciterMustang.cpp
    Nordsiek.cpp
    Relay.cpp
    Abs.cpp
    Results.cpp
    LimiterConst.cpp
    LimitedLag.cpp
    RSTrigger.cpp
    AndOrNot.cpp
    Sum.cpp
    DeadBand.cpp
    DerlagContinuous.cpp
    Comparator.cpp
    Expand.cpp
    ZCDetector.cpp
    DynaModelSerialize.cpp
    SerializerJson.cpp
    )             

add_subdirectory("SuiteSparse/KLU")
add_executable(EMSServer ${EMSSSOURCES})
target_include_directories(EMSServer PRIVATE ${KLU_INCLUDES} ${DFW_INCLUDES}) 
find_library(dl NAMES dl PATH /usr/lib/x86_x64-linux-gnu/libdl.so)
target_link_libraries(EMSServer klu kluz dl)
