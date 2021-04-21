cmake_minimum_required(VERSION 3.4)
cmake_policy(SET CMP0048 NEW)
project(oauth2_cpp VERSION 0.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(src
        "src/main.cpp"
        )

include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckSymbolExists)

##############################################################
# CMake options that can be overridden at configuration time #
##############################################################

option(USE_LIBCURL "Enable libcurl (defaults to `ON` on non-Windows)" OFF)

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
    # pass
elseif (CRYPTO_LIB STREQUAL "WinCrypt")
    # pass
elseif (NOT DEFINED CRYPTO_LIB)
    message(FATAL_ERROR "CRYPTO_LIB undefined")
else ()
    message("CRYPTO_LIB is ${CRYPTO_LIB}")
endif ()

########################
# HTTP/HTTPS libraries #
########################

if ((CMAKE_SYSTEM_NAME STREQUAL "FreeBSD"
        OR CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
        AND USE_LIBCURL STREQUAL "OFF")
    set(USE_LIBFETCH 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL "OpenBSD" AND USE_LIBCURL STREQUAL "OFF")
    set(USE_OPENBSD_FTP 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(USE_WININET 1)
else ()
    set(USE_LIBCURL 1)
    include(cmake/FindCurlCustom.cmake)
    get_curl(CURL_LINK_LIBRARIES)
endif ()

################################
# Configure file (interpolate) #
################################

configure_file(src/config.h.in ${PROJECT_BINARY_DIR}/src/config.h @ONLY)

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

add_executable(${PROJECT_NAME} "${src}")
# set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "oauth2_cpp")
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_BINARY_DIR}/src)

if (DEFINED CURL_LINK_LIBRARIES)
    target_link_libraries(${PROJECT_NAME} ${CURL_LINK_LIBRARIES})
elseif (DEFINED USE_WININET)
    target_link_libraries("${PROJECT_NAME}" wininet)
elseif (DEFINED USE_LIBFETCH)
    target_link_libraries("${PROJECT_NAME}" fetch)
endif ()

if (DEFINED OPENSSL_LIBRARIES)
    target_link_libraries(${PROJECT_NAME} OpenSSL::SSL)
    target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PRIVATE ${OPENSSL_INCLUDE_DIR})
endif ()
