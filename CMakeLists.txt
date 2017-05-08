project("erpiko")
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
cmake_policy(SET CMP0054 NEW)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
    CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(warnings "-Wall -Wextra -Werror")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(warnings "/EHsc")
endif()


if (NOT CONFIGURED_ONCE)
  if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "-std=c++14 ${warnings}"
        CACHE STRING "Flags used by the compiler during all build types." FORCE)
  endif()
endif()

include_directories(include)
include_directories(deps/libressl/include)

add_library(libresslcrypto STATIC IMPORTED)
add_library(libresslssl STATIC IMPORTED)
add_library(libressltls STATIC IMPORTED)
if (NOT MSVC)
  set_property(TARGET libresslcrypto PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/libressl/crypto/.libs/libcrypto.a)
  set_property(TARGET libresslssl PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/libressl/ssl/.libs/libssl.a)
  set_property(TARGET libressltls PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/libressl/tls/.libs/libtls.a)
else()
  if (MSVC_C_ARCHITECTURE_ID MATCHES 64 OR MSVC_CXX_ARCHITECTURE_ID MATCHES 64)
    set_property(TARGET libresslcrypto PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/libressl/crypto/crypto-41.lib)
    set_property(TARGET libresslssl PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/libressl/ssl/libssl-43.lib)
    set_property(TARGET libressltls PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/libressl/tls/libtls-15.lib)
  else()
    set_property(TARGET libresslcrypto PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/libressl/crypto/crypto-41.lib)
    set_property(TARGET libresslssl PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/libressl/ssl/ssl-43.lib)
    set_property(TARGET libressltls PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/libressl/tls/tls-15.lib)
  endif()
endif()

enable_testing()
add_subdirectory(src)
add_subdirectory(tests)

set(CONFIGURED_ONCE TRUE CACHE INTERNAL
  "A flag showing that CMake has configured at least once.")
