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

# add_compile_options(-fsanitize=address) add_compile_options(-static-libasan)
# add_compile_options(-fsanitize=thread) add_compile_options(-static-libtsan)

# add_compile_options(-fsanitize=undefined)
# add_compile_options(-static-libubsan)

# add_link_options(-fsanitize=address) add_link_options(-static-libasan)
# add_link_options(-fsanitize=thread) add_link_options(-static-libtsan)

# add_link_options(-fsanitize=undefined) add_link_options(-static-libubsan)

project(
  rush
  VERSION 0.0.1
  DESCRIPTION "rush protocol transport library")

include(GNUInstallDirs)

find_library(LIBEV_LIBRARIES NAMES ev REQUIRED)
find_path(LIBEV_INCLUDE_DIR ev.h)

find_package(PkgConfig REQUIRED)

set(OPEN_SSL_QUIC "openssl = 1.1.1q+quic")
pkg_check_modules(OPENSSL REQUIRED ${OPEN_SSL_QUIC})
include_directories(${OPENSSL_INCLUDE_DIRS})

pkg_check_modules(NGTCP2 REQUIRED libngtcp2)
include_directories(${NGTCP2_INCLUDE_DIRS})

pkg_check_modules(NGTCP2_OPENSSL REQUIRED libngtcp2_crypto_openssl)
include_directories(${NGTCP2_OPENSSL_DIRS})

# set(CMAKE_CXX_CLANG_TIDY clang-tidy.par -checks=concurrency-)

add_library(
  rush
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

set(PUBLIC_HEADERS ${PROJECT_SOURCE_DIR}/include/Rush.h
                   ${PROJECT_SOURCE_DIR}/include/CodecUtils.h)

set_target_properties(rush PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(rush PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")

target_include_directories(rush PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(rush ${OPENSSL_LIBRARIES})
target_link_libraries(rush ${NGTCP2_LIBRARIES})
target_link_libraries(rush ${NGTCP2_OPENSSL_LIBRARIES})
target_link_libraries(rush ${LIBEV_LIBRARIES})

set(LIBS_PRIVATE "-lpthread -lstdc++ -lev")
set(REQUIRES_PRIVATE
    "libngtcp2 libngtcp2_crypto_openssl ${OPEN_SSL_QUIC} libcrypto")

configure_file(librush.pc.in librush.pc @ONLY)

install(
  TARGETS rush
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(FILES ${CMAKE_BINARY_DIR}/librush.pc
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
