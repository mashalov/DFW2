cmake_minimum_required(VERSION 3.16)
project(UMC)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-march=native)

set(INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../../fmt/include/"
             "${CMAKE_CURRENT_SOURCE_DIR}/../antlr4-master/antlr4-master/runtime/Cpp/runtime/src/" )

set(ANTLR4CPP_JAR_LOCATION ${PROJECT_SOURCE_DIR}/antlr/antlr-4.9.2-complete.jar)

set(ANTLR4_PRODUCTS
  "${CMAKE_CURRENT_SOURCE_DIR}/Generated/EquationsBaseVisitor.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Generated/EquationsLexer.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Generated/EquationsVisitor.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Generated/EquationsParser.cpp"
)

SET(BUILD_STATIC OFF CACHE BOOL "CryptoPP static off")
SET(BUILD_TESTING OFF CACHE BOOL "CryptoPP tests off")

add_subdirectory("../CryptoPP" "${CMAKE_CURRENT_BINARY_DIR}/Build")

add_custom_command(OUTPUT ${ANTLR4_PRODUCTS}
  COMMAND java -jar ${ANTLR4CPP_JAR_LOCATION} -Dlanguage=Cpp -no-listener -visitor -o ${CMAKE_CURRENT_SOURCE_DIR}/Generated ${CMAKE_CURRENT_SOURCE_DIR}/Equations.g4
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Equations.g4"
  COMMENT "Generating lexer and parser by Antlr4"
  VERBATIM
)

set(SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../fmt/src/format.cc
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
    utils.cpp
    ../../DFW2/Messages.cpp
    )             

add_custom_target(ANTL4RUNTIME DEPENDS ${ANTLR4_PRODUCTS})
add_library(UMC SHARED ${SOURCES})

target_include_directories(UMC PRIVATE ${INCLUDES}) 
find_library(dl NAMES dl PATH /usr/lib/x86_x64-linux-gnu/libdl.so)
target_link_libraries(UMC 
    "${CMAKE_CURRENT_SOURCE_DIR}/../CryptoPP/libcryptopp.a"
    "${CMAKE_CURRENT_SOURCE_DIR}/../antlr4-master/antlr4-master/runtime/Cpp/dist/libantlr4-runtime.a"
    dl stdc++fs
    )


