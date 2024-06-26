# CMake generated Testfile for 
# Source directory: D:/source/repos/DFW2/UMC/CryptoPP
# Build directory: D:/source/repos/DFW2/UMC/CryptoPP
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(build_cryptest "C:/Program Files/CMake/bin/cmake.exe" "--build" "D:/source/repos/DFW2/UMC/CryptoPP" "--target" "cryptest")
  set_tests_properties(build_cryptest PROPERTIES  _BACKTRACE_TRIPLES "D:/source/repos/DFW2/UMC/CryptoPP/CMakeLists.txt;1247;add_test;D:/source/repos/DFW2/UMC/CryptoPP/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(build_cryptest "C:/Program Files/CMake/bin/cmake.exe" "--build" "D:/source/repos/DFW2/UMC/CryptoPP" "--target" "cryptest")
  set_tests_properties(build_cryptest PROPERTIES  _BACKTRACE_TRIPLES "D:/source/repos/DFW2/UMC/CryptoPP/CMakeLists.txt;1247;add_test;D:/source/repos/DFW2/UMC/CryptoPP/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(build_cryptest "C:/Program Files/CMake/bin/cmake.exe" "--build" "D:/source/repos/DFW2/UMC/CryptoPP" "--target" "cryptest")
  set_tests_properties(build_cryptest PROPERTIES  _BACKTRACE_TRIPLES "D:/source/repos/DFW2/UMC/CryptoPP/CMakeLists.txt;1247;add_test;D:/source/repos/DFW2/UMC/CryptoPP/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(build_cryptest "C:/Program Files/CMake/bin/cmake.exe" "--build" "D:/source/repos/DFW2/UMC/CryptoPP" "--target" "cryptest")
  set_tests_properties(build_cryptest PROPERTIES  _BACKTRACE_TRIPLES "D:/source/repos/DFW2/UMC/CryptoPP/CMakeLists.txt;1247;add_test;D:/source/repos/DFW2/UMC/CryptoPP/CMakeLists.txt;0;")
else()
  add_test(build_cryptest NOT_AVAILABLE)
endif()
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(cryptest "D:/source/repos/DFW2/UMC/CryptoPP/Debug/cryptest.exe" "v")
  set_tests_properties(cryptest PROPERTIES  DEPENDS "build_cryptest" _BACKTRACE_TRIPLES "D:/source/repos/DFW2/UMC/CryptoPP/CMakeLists.txt;1248;add_test;D:/source/repos/DFW2/UMC/CryptoPP/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(cryptest "D:/source/repos/DFW2/UMC/CryptoPP/Release/cryptest.exe" "v")
  set_tests_properties(cryptest PROPERTIES  DEPENDS "build_cryptest" _BACKTRACE_TRIPLES "D:/source/repos/DFW2/UMC/CryptoPP/CMakeLists.txt;1248;add_test;D:/source/repos/DFW2/UMC/CryptoPP/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(cryptest "D:/source/repos/DFW2/UMC/CryptoPP/MinSizeRel/cryptest.exe" "v")
  set_tests_properties(cryptest PROPERTIES  DEPENDS "build_cryptest" _BACKTRACE_TRIPLES "D:/source/repos/DFW2/UMC/CryptoPP/CMakeLists.txt;1248;add_test;D:/source/repos/DFW2/UMC/CryptoPP/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(cryptest "D:/source/repos/DFW2/UMC/CryptoPP/RelWithDebInfo/cryptest.exe" "v")
  set_tests_properties(cryptest PROPERTIES  DEPENDS "build_cryptest" _BACKTRACE_TRIPLES "D:/source/repos/DFW2/UMC/CryptoPP/CMakeLists.txt;1248;add_test;D:/source/repos/DFW2/UMC/CryptoPP/CMakeLists.txt;0;")
else()
  add_test(cryptest NOT_AVAILABLE)
endif()
