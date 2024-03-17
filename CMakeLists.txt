cmake_minimum_required(VERSION 3.14)

# set up search path to vcpkg package manager such that dependencies installed with `vcpkg install` can be found
if (DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message("VCPKG_ROOT found")
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(murify VERSION 0.1)

option(MURIFY_USE_AVX2 "Use AVX2 instruction set" ON)

# library target configuration
file(GLOB MURIFY_LIBRARY_SOURCES
    ${CMAKE_SOURCE_DIR}/test/*.cpp
)
file(GLOB MURIFY_LIBRARY_HEADERS
    ${CMAKE_SOURCE_DIR}/include/murify/*.hpp
)
add_library(murify INTERFACE)

# compiler options
target_compile_features(murify INTERFACE cxx_std_17)
set_property(TARGET murify PROPERTY VISIBILITY_INLINES_HIDDEN ON)
set_property(TARGET murify PROPERTY PUBLIC_HEADER ${MURIFY_LIBRARY_HEADERS})

if(MURIFY_USE_AVX2)
    if(MSVC)
        set(MURIFY_AVX2_COMPILE "/arch:AVX2")
    else()
        set(MURIFY_AVX2_COMPILE "-mavx2")
    endif()
else()
    set(MURIFY_AVX2_COMPILE "")
endif()

if(MSVC)
    target_compile_definitions(murify INTERFACE _CRT_SECURE_NO_WARNINGS)
    # set warning level 4 and treat all warnings as errors
    target_compile_options(murify INTERFACE ${MURIFY_AVX2_COMPILE} /permissive- /W4 /WX /Zc:__cplusplus,enumTypes,lambda,referenceBinding,rvalueCast,strictStrings,ternary)
else()
    # enable lots of warnings and treat all warnings as errors
    target_compile_options(murify INTERFACE ${MURIFY_AVX2_COMPILE} -Wall -Wextra -pedantic -Werror -Wfatal-errors)
endif()

# include directories
target_include_directories(murify INTERFACE ${CMAKE_SOURCE_DIR}/include)

# build configuration
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_result)
if(ipo_result)
    set_property(TARGET murify PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# test target configuration
add_executable(murify-tests ${MURIFY_LIBRARY_SOURCES})
target_link_libraries(murify-tests PRIVATE murify)

# install configuration
include(GNUInstallDirs)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/murify
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
)
