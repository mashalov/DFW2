cmake_minimum_required(VERSION 3.16)
project(EMSServer)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-march=native)
add_compile_definitions(DLONG)

set_directory_properties(PROPERTIES COMPILE_DEFINITIONS_DEBUG "_DEBUG")
add_subdirectory("../UMC/UMC" "${CMAKE_CURRENT_BINARY_DIR}/Build")
add_subdirectory("../ResultFile" "${CMAKE_CURRENT_BINARY_DIR}/Res")


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
    LoadFlowTanh.cpp
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
    DynaGenerator2C.cpp
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
    PlatformFolders.cpp
    DynaModelCompile.cpp
    ResultsWriterABI.cpp
    DynaGeneratorPark3C.cpp
    DynaGeneratorDQBase.cpp
    DynaGeneratorPark4C.cpp
    DynaExcConMustangNW.cpp
    SerializerValidation.cpp
    DynaReactor.cpp
    NodeMeasures.cpp
    BranchMeasures.cpp
    Logger.cpp
    TestDevice.cpp
    )             

find_library(dl NAMES dl PATH /usr/lib/x86_x64-linux-gnu/libdl.so)
find_package(Threads REQUIRED)
add_subdirectory("SuiteSparse/KLU")
add_executable(EMSServer ${EMSSSOURCES})
target_include_directories(EMSServer PRIVATE ${KLU_INCLUDES} ${DFW_INCLUDES}) 
find_library(dl NAMES dl PATH /usr/lib/x86_x64-linux-gnu/libdl.so)
target_link_libraries(EMSServer klu kluz dl Threads::Threads stdc++fs)
