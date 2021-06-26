cmake_minimum_required(VERSION 3.16)
project(UMC)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../../fmt/include/"
             "${CMAKE_CURRENT_SOURCE_DIR}/../antlr4-master/antlr4-master/runtime/Cpp/runtime/src/" )

set(SOURCES 
    ../../../fmt/src/format.cc 
    Generated/EquationsBaseVisitor.cpp
    Generated/EquationsLexer.cpp
    Generated/EquationsParser.cpp
    Generated/EquationsVisitor.cpp
    AntlrASTErrorListener.cpp
    AntlrASTVisitor.cpp
    ASTCodeGeneratorBase.cpp
    ASTEquation.cpp
    ASTNodeBase.cpp
    ASTNodeDeadBand.cpp
    ASTNodeDerLag.cpp
    ASTNodeDiv.cpp
    ASTNodeExpand.cpp
    ASTNodeLag.cpp
    ASTNodeLagLim.cpp
    ASTNodeLimit.cpp
    ASTNodeLogicOps.cpp
    ASTNodeMinus.cpp
    ASTNodeMul.cpp
    ASTNodePow.cpp
    ASTNodeShrink.cpp
    ASTNodeSum.cpp
    ASTreeBase.cpp
    ASTTrigonometry.cpp
    ASTTrivialEquations.cpp
    ASTVariable.cpp
    CompilerBase.cpp
    CompilerGCC.cpp
    dllmain.cpp
    EquationsOps.cpp
    UMC.cpp
    )             

#add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../antlr4-master/antlr4-master/runtime/Cpp/"
# "${CMAKE_CURRENT_SOURCE_DIR}/../antlr4-master/antlr4-master/runtime/Cpp/build/")

 #add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../CryptoPP/"
 #"${CMAKE_CURRENT_SOURCE_DIR}/../CryptoPP/build/")

#add_library(UMC SHARED ${SOURCES})
add_executable(UMC  ${SOURCES})
target_include_directories(UMC PRIVATE ${INCLUDES}) 
find_library(dl NAMES dl PATH /usr/lib/x86_x64-linux-gnu/libdl.so)
target_link_libraries(UMC 
    "${CMAKE_CURRENT_SOURCE_DIR}/../CryptoPP/libcryptopp.a"
    "${CMAKE_CURRENT_SOURCE_DIR}/../antlr4-master/antlr4-master/runtime/Cpp/dist/libantlr4-runtime.a"
    dl
    )


