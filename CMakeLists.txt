cmake_minimum_required(VERSION 3.20.2)

set(CMAKE_BUILD_TYPE Debug)

add_compile_options(
  -Wno-unused-parameter
  -Wno-unused-variable
  -Wall
  -Wextra
  -Werror
  -Wconversion
  -Wnull-dereference
  -Wsuggest-override
  -Wpointer-arith
  -Wshift-overflow=1
  -Wduplicated-cond
  -Wduplicated-branches
  -Wsuggest-final-types
  -Wcast-align
  -Wnon-virtual-dtor
  -Wmisleading-indentation)

project(
  rush
  VERSION 0.0.1
  DESCRIPTION "rush protocol transport library")

option(WITH_GNUTLS "use gnutls for tls" OFF)

IF (WITH_GNUTLS)
  add_compile_definitions(TLS_USE_GNUTLS)
ELSE()
  add_compile_definitions(TLS_USE_OPENSSL)
ENDIF()

include(GNUInstallDirs)

find_library(LIBEV_LIBRARIES NAMES ev REQUIRED)
find_path(LIBEV_INCLUDE_DIR ev.h)

find_package(PkgConfig REQUIRED)

IF (WITH_GNUTLS)
  set(GNUTLS "gnutls = 3.7.8")
  pkg_check_modules(GNUTLS REQUIRED ${GNUTLS})
  include_directories(${GNUTLS_INCLUDE_DIRS})

  pkg_check_modules(NGTCP2_GNUTLS_REQUIRED libngtcp2_crypto_gnutls)
  include_directories(${NGTCP2_GNUTLS_DIRS})
ELSE()
  set(OPEN_SSL_QUIC "openssl = 1.1.1s+quic")
  pkg_check_modules(OPENSSL REQUIRED ${OPEN_SSL_QUIC})
  include_directories(${OPENSSL_INCLUDE_DIRS})

  pkg_check_modules(NGTCP2_OPENSSL REQUIRED libngtcp2_crypto_openssl)
  include_directories(${NGTCP2_OPENSSL_DIRS})
ENDIF()

set(LIB_NGTCP2 "libngtcp2 >= 0.11.0")
pkg_check_modules(NGTCP2 REQUIRED ${LIB_NGTCP2})
include_directories(${NGTCP2_INCLUDE_DIRS})

set(
  SOURCE_FILES
  ${PROJECT_SOURCE_DIR}/src/Rush.cpp
  ${PROJECT_SOURCE_DIR}/src/Evloop.cpp
  ${PROJECT_SOURCE_DIR}/src/Utils.cpp
  ${PROJECT_SOURCE_DIR}/src/TLSClientContext.cpp
  ${PROJECT_SOURCE_DIR}/src/RushClient.cpp
  ${PROJECT_SOURCE_DIR}/src/Buffer.cpp
  ${PROJECT_SOURCE_DIR}/src/RushMuxer.cpp
  ${PROJECT_SOURCE_DIR}/src/Pool.cpp
  ${PROJECT_SOURCE_DIR}/src/Frames.cpp
  ${PROJECT_SOURCE_DIR}/src/CodecUtils.cpp
  ${PROJECT_SOURCE_DIR}/src/Serializer.cpp
  ${PROJECT_SOURCE_DIR}/src/QuicConnection.cpp)

IF (WITH_GNUTLS)
  set(SOURCE_FILES ${SOURCE_FILES}
      ${PROJECT_SOURCE_DIR}/src/GnutlsClientContext.cpp)
ELSE()
  set(SOURCE_FILES ${SOURCE_FILES}
      ${PROJECT_SOURCE_DIR}/src/OpensslClientContext.cpp)
ENDIF()

add_library(rush ${SOURCE_FILES})

set(PUBLIC_HEADERS ${PROJECT_SOURCE_DIR}/include/Rush.h
                   ${PROJECT_SOURCE_DIR}/include/CodecUtils.h)

set_target_properties(rush PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(rush PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")

target_include_directories(rush PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(rush ${NGTCP2_LIBRARIES})


IF (WITH_GNUTLS)
  target_link_libraries(rush ${GNUTLS_LIBRARIES})
  target_link_libraries(rush ${NGTCP2_GNUTLS_LIBRARIES})
ELSE()
  target_link_libraries(rush ${OPENSSL_LIBRARIES})
  target_link_libraries(rush ${NGTCP2_OPENSSL_LIBRARIES})
ENDIF()

target_link_libraries(rush ${LIBEV_LIBRARIES})

set(LIBS_PRIVATE "-lpthread -lstdc++ -lev")

IF (WITH_GNUTLS)
  set(REQUIRES_PRIVATE "${LIB_NGTCP2} libngtcp2_crypto_gnutls ${GNUTLS}")
ELSE()
  set(REQUIRES_PRIVATE "${LIB_NGTCP2} libngtcp2_crypto_openssl ${OPEN_SSL_QUIC}")
ENDIF()

configure_file(librush.pc.in librush.pc @ONLY)

install(
  TARGETS rush
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(FILES ${CMAKE_BINARY_DIR}/librush.pc
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
