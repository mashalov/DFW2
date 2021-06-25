cmake_minimum_required(VERSION 3.16)
project(UMC)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../../fmt/include/"
             "${CMAKE_CURRENT_SOURCE_DIR}/../antlr4-master/antlr4-master/runtime/Cpp/runtime/src/" )

set(SOURCES 
    ../../../fmt/src/format.cc 
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
#    CompilerBase.cpp
    dllmain.cpp
    EquationsOps.cpp
    UMC.cpp
    )             

add_library(UMC ${SOURCES})
target_include_directories(UMC PRIVATE ${INCLUDES}) 

