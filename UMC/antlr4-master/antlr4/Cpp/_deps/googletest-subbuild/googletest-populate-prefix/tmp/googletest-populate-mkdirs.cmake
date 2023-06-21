# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/source/repos/DFW2/UMC/antlr4-master/antlr4/Cpp/_deps/googletest-src"
  "D:/source/repos/DFW2/UMC/antlr4-master/antlr4/Cpp/_deps/googletest-build"
  "D:/source/repos/DFW2/UMC/antlr4-master/antlr4/Cpp/_deps/googletest-subbuild/googletest-populate-prefix"
  "D:/source/repos/DFW2/UMC/antlr4-master/antlr4/Cpp/_deps/googletest-subbuild/googletest-populate-prefix/tmp"
  "D:/source/repos/DFW2/UMC/antlr4-master/antlr4/Cpp/_deps/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp"
  "D:/source/repos/DFW2/UMC/antlr4-master/antlr4/Cpp/_deps/googletest-subbuild/googletest-populate-prefix/src"
  "D:/source/repos/DFW2/UMC/antlr4-master/antlr4/Cpp/_deps/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp"
)

set(configSubDirs Debug;Release;MinSizeRel;RelWithDebInfo)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/source/repos/DFW2/UMC/antlr4-master/antlr4/Cpp/_deps/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp/${subDir}")
endforeach()
