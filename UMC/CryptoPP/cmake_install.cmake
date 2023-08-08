# Install script for directory: D:/source/repos/DFW2/UMC/CryptoPP

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/cryptopp")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "D:/source/repos/DFW2/UMC/CryptoPP/Debug/cryptopp-shared.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "D:/source/repos/DFW2/UMC/CryptoPP/Release/cryptopp-shared.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "D:/source/repos/DFW2/UMC/CryptoPP/MinSizeRel/cryptopp-shared.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "D:/source/repos/DFW2/UMC/CryptoPP/RelWithDebInfo/cryptopp-shared.lib")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "D:/source/repos/DFW2/UMC/CryptoPP/Debug/cryptopp-shared.dll")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "D:/source/repos/DFW2/UMC/CryptoPP/Release/cryptopp-shared.dll")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "D:/source/repos/DFW2/UMC/CryptoPP/MinSizeRel/cryptopp-shared.dll")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "D:/source/repos/DFW2/UMC/CryptoPP/RelWithDebInfo/cryptopp-shared.dll")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/source/repos/DFW2/UMC/CryptoPP/Debug/cryptopp-static.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/source/repos/DFW2/UMC/CryptoPP/Release/cryptopp-static.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/source/repos/DFW2/UMC/CryptoPP/MinSizeRel/cryptopp-static.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/source/repos/DFW2/UMC/CryptoPP/RelWithDebInfo/cryptopp-static.lib")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/cryptopp" TYPE FILE FILES
    "D:/source/repos/DFW2/UMC/CryptoPP/3way.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/adler32.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/adv_simd.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/aes.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/aes_armv4.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/algebra.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/algparam.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/allocate.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/arc4.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/argnames.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/aria.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/arm_simd.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/asn.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/authenc.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/base32.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/base64.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/basecode.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/blake2.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/blowfish.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/blumshub.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/camellia.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/cast.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/cbcmac.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/ccm.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/chacha.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/chachapoly.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/cham.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/channels.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/cmac.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/config.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/config_align.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/config_asm.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/config_cpu.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/config_cxx.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/config_dll.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/config_int.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/config_misc.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/config_ns.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/config_os.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/config_ver.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/cpu.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/crc.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/cryptlib.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/darn.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/default.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/des.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/dh.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/dh2.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/dll.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/dmac.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/donna.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/donna_32.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/donna_64.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/donna_sse.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/drbg.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/dsa.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/eax.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/ec2n.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/eccrypto.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/ecp.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/ecpoint.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/elgamal.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/emsa2.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/eprecomp.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/esign.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/factory.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/fhmqv.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/files.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/filters.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/fips140.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/fltrimpl.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/gcm.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/gf256.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/gf2_32.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/gf2n.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/gfpcrypt.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/gost.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/gzip.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/hashfwd.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/hc128.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/hc256.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/hex.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/hight.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/hkdf.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/hmac.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/hmqv.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/hrtimer.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/ida.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/idea.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/integer.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/iterhash.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/kalyna.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/keccak.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/lea.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/lsh.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/lubyrack.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/luc.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/mars.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/md2.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/md4.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/md5.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/mdc.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/mersenne.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/misc.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/modarith.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/modes.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/modexppc.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/mqueue.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/mqv.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/naclite.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/nbtheory.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/nr.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/oaep.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/oids.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/osrng.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/ossig.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/padlkrng.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/panama.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/pch.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/pkcspad.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/poly1305.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/polynomi.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/ppc_simd.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/pssr.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/pubkey.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/pwdbased.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/queue.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/rabbit.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/rabin.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/randpool.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/rc2.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/rc5.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/rc6.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/rdrand.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/resource.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/rijndael.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/ripemd.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/rng.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/rsa.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/rw.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/safer.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/salsa.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/scrypt.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/seal.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/secblock.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/secblockfwd.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/seckey.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/seed.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/serpent.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/serpentp.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/sha.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/sha1_armv4.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/sha256_armv4.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/sha3.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/sha512_armv4.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/shacal2.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/shake.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/shark.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/simeck.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/simon.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/simple.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/siphash.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/skipjack.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/sm3.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/sm4.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/smartptr.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/sosemanuk.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/speck.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/square.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/stdcpp.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/strciphr.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/tea.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/threefish.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/tiger.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/trap.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/trunhash.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/ttmac.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/tweetnacl.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/twofish.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/vmac.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/wake.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/whrlpool.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/words.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/xed25519.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/xtr.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/xtrcrypt.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/xts.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/zdeflate.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/zinflate.h"
    "D:/source/repos/DFW2/UMC/CryptoPP/zlib.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/cryptopp" TYPE FILE FILES
    "D:/source/repos/DFW2/UMC/CryptoPP/cryptopp-config.cmake"
    "D:/source/repos/DFW2/UMC/CryptoPP/cryptopp-config-version.cmake"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/cryptopp/cryptopp-targets.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/cryptopp/cryptopp-targets.cmake"
         "D:/source/repos/DFW2/UMC/CryptoPP/CMakeFiles/Export/lib/cmake/cryptopp/cryptopp-targets.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/cryptopp/cryptopp-targets-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/cryptopp/cryptopp-targets.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/cryptopp" TYPE FILE FILES "D:/source/repos/DFW2/UMC/CryptoPP/CMakeFiles/Export/lib/cmake/cryptopp/cryptopp-targets.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/cryptopp" TYPE FILE FILES "D:/source/repos/DFW2/UMC/CryptoPP/CMakeFiles/Export/lib/cmake/cryptopp/cryptopp-targets-debug.cmake")
  endif()
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/cryptopp" TYPE FILE FILES "D:/source/repos/DFW2/UMC/CryptoPP/CMakeFiles/Export/lib/cmake/cryptopp/cryptopp-targets-minsizerel.cmake")
  endif()
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/cryptopp" TYPE FILE FILES "D:/source/repos/DFW2/UMC/CryptoPP/CMakeFiles/Export/lib/cmake/cryptopp/cryptopp-targets-relwithdebinfo.cmake")
  endif()
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/cryptopp" TYPE FILE FILES "D:/source/repos/DFW2/UMC/CryptoPP/CMakeFiles/Export/lib/cmake/cryptopp/cryptopp-targets-release.cmake")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "D:/source/repos/DFW2/UMC/CryptoPP/Debug/cryptest.exe")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "D:/source/repos/DFW2/UMC/CryptoPP/Release/cryptest.exe")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "D:/source/repos/DFW2/UMC/CryptoPP/MinSizeRel/cryptest.exe")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "D:/source/repos/DFW2/UMC/CryptoPP/RelWithDebInfo/cryptest.exe")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/cryptopp" TYPE DIRECTORY FILES "D:/source/repos/DFW2/UMC/CryptoPP/TestData")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/cryptopp" TYPE DIRECTORY FILES "D:/source/repos/DFW2/UMC/CryptoPP/TestVectors")
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "D:/source/repos/DFW2/UMC/CryptoPP/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
