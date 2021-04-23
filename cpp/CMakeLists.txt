cmake_minimum_required(VERSION 3.4)
cmake_policy(SET CMP0048 NEW)
project(oauth2_cpp VERSION 0.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(src "src/main.cpp" src/url.h src/char_utils.h src/random_string.cpp)

include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckSymbolExists)

##############################################################
# CMake options that can be overridden at configuration time #
##############################################################

# "Crypto library to use, defaults to Linux,BSD,SunOS: OpenSSL; Windows: STunnel; macOS: LibreSSL"
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND NOT DEFINED CRYPTO_LIB)
    check_include_files("CommonCrypto/CommonCrypto.h;CommonCrypto/CommonDigest.h" HAVE_COMMON_CRYPTO_H)
    if (NOT HAVE_COMMON_CRYPTO_H)
        message(FATAL_ERROR "CommonCrypto.h not found")
    endif ()
    set(CRYPTO_LIB CommonCrypto)
    set(USE_COMMON_CRYPTO 1)
elseif ((CMAKE_SYSTEM_NAME STREQUAL "OpenBSD" AND NOT DEFINED CRYPTO_LIB) OR CRYPTO_LIB STREQUAL "LibreSSL")
    set(CRYPTO_LIB LibreSSL)
    include(cmake/FindLibreSSL.cmake)
    set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")
    set(USE_LIBRESSL 1)
elseif ((CMAKE_SYSTEM_NAME STREQUAL "Linux"
        OR CMAKE_SYSTEM_NAME STREQUAL "FreeBSD"
        OR CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
        AND NOT DEFINED CRYPTO_LIB
        OR CRYPTO_LIB STREQUAL "OpenSSL")
    set(CRYPTO_LIB OpenSSL)
    set(USE_OPENSSL 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows" AND NOT DEFINED CRYPTO_LIB)
    set(CRYPTO_LIB "WinCrypt")
    set(USE_WINCRYPT 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL "SunOS" AND NOT DEFINED CRYPTO_LIB)
    message(FATAL_ERROR "TODO")
endif ()

if (NOT DEFINED TARGET_ARCH)
    set(TARGET_ARCH ${CMAKE_HOST_SYSTEM_PROCESSOR})
endif ()

if ((DEFINED CMAKE_C_FLAGS AND NOT CMAKE_C_FLAGS STREQUAL "")
        OR (DEFINED CMAKE_CXX_FLAGS AND NOT CMAKE_CXX_FLAGS STREQUAL ""))
    set(TARGET_ARCH_REGEX "^.*-march[= ]([^ ]+).*$")
    string(REGEX MATCH "${TARGET_ARCH_REGEX}" TARGET_ARCH_MATCH ${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS})
    if (TARGET_ARCH_MATCH)
        string(REGEX REPLACE "${TARGET_ARCH_REGEX}" "\\1" TARGET_ARCH ${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS})
    endif ()
endif ()

#################
# SSL libraries #
#################
if (CRYPTO_LIB STREQUAL "OpenSSL")
    if ((NOT DEFINED OPENSSL_ROOT_DIR OR NOT IS_DIRECTORY OPENSSL_ROOT_DIR)
            AND CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND IS_DIRECTORY "/usr/local/opt/openssl")
        execute_process(
                COMMAND brew --prefix OpenSSL
                RESULT_VARIABLE BREW_OPENSSL
                OUTPUT_VARIABLE BREW_OPENSSL_PREFIX
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if (BREW_OPENSSL EQUAL 0 AND EXISTS "${BREW_OPENSSL_PREFIX}")
            message(STATUS "Found OpenSSL keg installed by Homebrew at ${BREW_OPENSSL_PREFIX}")
            set(OPENSSL_ROOT_DIR "${BREW_OPENSSL_PREFIX}/")
            set(OPENSSL_INCLUDE_DIR "${BREW_OPENSSL_PREFIX}/include")
            set(OPENSSL_LIBRARIES "${BREW_OPENSSL_PREFIX}/lib")
            set(OPENSSL_CRYPTO_LIBRARY "${BREW_OPENSSL_PREFIX}/lib/libcrypto.dylib")
        endif()
    elseif ((NOT DEFINED OPENSSL_ROOT_DIR OR NOT IS_DIRECTORY OPENSSL_ROOT_DIR)
            AND IS_DIRECTORY "/usr/include/openssl")
        set(OPENSSL_ROOT_DIR "/usr/include/openssl")
    endif ()
    find_package(OpenSSL REQUIRED)
    if (DEFINED OPENSSL_INCLUDE_DIR)
        if (EXISTS "${OPENSSL_INCLUDE_DIR}/openssl/sha.h")
            set(HAVE_OPENSSL_SHA_H "ON")
        endif()
    else()
        check_include_file("openssl/sha.h" HAVE_OPENSSL_SHA_H)
    endif()
    if (NOT HAVE_OPENSSL_SHA_H)
        find_package(PkgConfig REQUIRED)
        pkg_search_module(OPENSSL REQUIRED openssl)
        message(FATAL_ERROR "OpenSSL installation doesn't have the required header files, install them or use alt SSL")
    endif ()
elseif (CRYPTO_LIB STREQUAL "LibreSSL")
    find_package(LibreSSL REQUIRED)
elseif (CRYPTO_LIB STREQUAL "CommonCrypto" OR CRYPTO_LIB STREQUAL "OpenSSL")
    message(FATAL_ERROR "TODO: Implement with CommonCrypto")
elseif (CRYPTO_LIB STREQUAL "WinCrypt")
    message(FATAL_ERROR "TODO: Implement with WinCrypt")
elseif (NOT DEFINED CRYPTO_LIB)
    message(FATAL_ERROR "CRYPTO_LIB undefined")
else ()
    message(FATAL_ERROR "TODO: Implement with ${CRYPTO_LIB}")
endif ()

################################
# Configure file (interpolate) #
################################

if (DEFINED STACK_SIZE)
    # pass
elseif (WIN32)
    set(STACK_SIZE 16384)
else ()
    set(STACK_SIZE 65535)
endif ()

if (NOT DEFINED SERVER_ADDR)
    set(SERVER_ADDR "127.0.0.1")
endif ()

if (NOT DEFINED SERVER_HOST)
    set(SERVER_HOST "localhost")
endif ()

if (NOT DEFINED PORT_TO_BIND)
    set(PORT_TO_BIND "3000")
endif ()

if (NOT DEFINED MSG_BACKLOG)
    set(MSG_BACKLOG 5)
endif ()

if (NOT DEFINED EXPECTED_PATH)
    set(EXPECTED_PATH "/ibm/cloud/appid/callback")
endif ()

if (NOT DEFINED API_HOST)
    set(API_HOST "31f5ff35.eu-gb.apigw.appdomain.cloud")
endif ()

if (NOT DEFINED API_APPLICATION_ENDPOINT_PATH)
    set(API_APPLICATION_ENDPOINT_PATH "/authtest/GetApplicationEndpoint")
endif ()

if (NOT DEFINED API_APPLICATION_ENDPOINT_PATH)
    set(API_APPLICATION_ENDPOINT_PATH "/authtest/GetApplicationEndpoint")
endif ()

if (NOT DEFINED API_GET_ACCESS_TOKEN_PATH)
    set(API_GET_ACCESS_TOKEN_PATH "/authtest/GetAccessToken")
endif ()

configure_file("src/config.h.in" "${PROJECT_BINARY_DIR}/src/config.h" @ONLY)

#############################################
# Compiler, library, and exec configuration #
#############################################

if (MSVC)
    add_compile_options(/WX /permissive-)
else ()
    add_compile_options(
            "$<$<CONFIG:DEBUG>:-Wall>"
            "$<$<CONFIG:DEBUG>:-Werror>"
            "$<$<CONFIG:DEBUG>:-pedantic>"
            "$<$<CONFIG:DEBUG>:-Wno-missing-braces>"
            "$<$<CONFIG:RELEASE>:-O3>"
    )

    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_compile_options("$<$<CONFIG:DEBUG>:-pedantic>")
    else ()
        add_compile_options("$<$<CONFIG:DEBUG>:-Wpedantic>")
    endif ()
endif ()

add_executable("${PROJECT_NAME}" "${src}")
target_include_directories("${PROJECT_NAME}" PUBLIC "${PROJECT_BINARY_DIR}/src")

if (DEFINED OPENSSL_LIBRARIES)
    target_link_libraries("${PROJECT_NAME}" OpenSSL::SSL "${OPENSSL_LIBRARIES}")
    target_include_directories("${PROJECT_NAME}" PRIVATE "${OPENSSL_INCLUDE_DIR}")
endif ()

if (APPLE) # or if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_library(CORE_FOUNDATION_LIBRARY CoreFoundation ONLY)
    find_library(APPLICATION_SERVICES_LIBRARY ApplicationServices ONLY)
    target_link_libraries("${PROJECT_NAME}" "${CORE_FOUNDATION_LIBRARY}" "${APPLICATION_SERVICES_LIBRARY}")
elseif(WIN32)
    target_link_libraries("${PROJECT_NAME}" wsock32 ws2_32)
endif()
