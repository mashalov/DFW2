cmake_minimum_required(VERSION 3.16)
project(EMSServer)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_compile_definitions(DLONG)

set(KINCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/SuiteSparse/KLU/Include" 
             "${CMAKE_CURRENT_SOURCE_DIR}/SuiteSparse/AMD/Include"
             "${CMAKE_CURRENT_SOURCE_DIR}/SuiteSparse/COLAMD/Include"
             "${CMAKE_CURRENT_SOURCE_DIR}/SuiteSparse/BTF/Include"
             "${CMAKE_CURRENT_SOURCE_DIR}/SuiteSparse/CSparse/Include"
             "${CMAKE_CURRENT_SOURCE_DIR}/SuiteSparse/SuiteSparse_config/")

set(EMSSSOURCES 
    main.cpp 
    format.cc 
    Messages.cpp
    LoadFlow.cpp
    )             

add_subdirectory("SuiteSparse/KLU")
add_executable(EMSServer ${EMSSSOURCES})
target_include_directories(EMSServer PRIVATE ${KINCLUDES})
target_link_libraries(EMSServer klu kluz)
