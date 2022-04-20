cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
# SPNG OR LODE
set(USE_LIB_PNG "SPNG")

if (MSVC)
    string(TOLOWER ${CMAKE_VS_PLATFORM_NAME} ARCHITECTURE)
else ()
    string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} ARCHITECTURE)
endif ()

if (ARCHITECTURE STREQUAL "amd64" OR ARCHITECTURE STREQUAL "x86_64")
    set(ARCHITECTURE "x64")
elseif (ARCHITECTURE STREQUAL "win32")
    set(ARCHITECTURE "x86")
endif ()
message("architecture: ${ARCHITECTURE}")

if (USE_LIB_PNG STREQUAL "LODE")
    set(LIB_PNG_SOURCES "${CMAKE_SOURCE_DIR}/external/lodepng.cpp")
elseif (USE_LIB_PNG STREQUAL "SPNG")
    set(LIB_PNG_SOURCES "${CMAKE_SOURCE_DIR}/external/miniz.c" "${CMAKE_SOURCE_DIR}/external/spng.c")
endif ()

add_library(${projectName}-${ARCHITECTURE} SHARED "DxTexWrapper.cpp" Content.h ${LIB_PNG_SOURCES})

target_link_libraries(${projectName}-${ARCHITECTURE} "${CMAKE_SOURCE_DIR}/lib/DirectXTex-${ARCHITECTURE}.lib")
target_include_directories(${projectName}-${ARCHITECTURE} PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/external/include")

target_compile_features(${projectName}-${ARCHITECTURE} PUBLIC cxx_std_${CMAKE_CXX_STANDARD})

message("C Compiler (${CMAKE_C_COMPILER_ID}) ${CMAKE_C_COMPILER}")
message("C++ Compiler (${CMAKE_CXX_COMPILER_ID}) ${CMAKE_CXX_COMPILER}")
message("C Compiler frontend variant: ${CMAKE_C_COMPILER_FRONTEND_VARIANT}")
message("C++ Compiler frontend variant: ${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}")


if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    message("Compiler is MSVC")
    target_compile_options(${projectName}-${ARCHITECTURE} PRIVATE /W4 /WX /EHs)
    if (USE_LIB_PNG STREQUAL "LODE")
        set_property(SOURCE "${CMAKE_SOURCE_DIR}/external/lodepng.cpp" APPEND PROPERTY COMPILE_OPTIONS "/wd4334" "/wd4267")
        add_definitions("/DDXTWRAPPER_USE_LIBLODEPNG")
    elseif (USE_LIB_PNG STREQUAL "SPNG")
        set_property(SOURCE "${CMAKE_SOURCE_DIR}/external/miniz.c" APPEND PROPERTY COMPILE_OPTIONS "/wd4127")
        set_property(SOURCE "${CMAKE_SOURCE_DIR}/external/spng.c" APPEND PROPERTY COMPILE_OPTIONS "/wd4245" "/wd4204" "/wd4706")
        add_definitions("/DSPNG_STATIC" "/DSPNG_USE_MINIZ")
        add_definitions("/DDXTWRAPPER_USE_LIBSPNG")
    endif ()
else ()
    if (USE_LIB_PNG STREQUAL "LODE")
        set_property(SOURCE "${CMAKE_SOURCE_DIR}/external/lodepng.cpp" APPEND PROPERTY COMPILE_OPTIONS "-Wno-shorten-64-to-32" "-Wno-zero-as-null-pointer-constant" "-Wno-old-style-cast" "-Wno-deprecated-declarations" "-Wno-extra-semi-stmt" "-Wno-implicit-int-conversion" "-Wno-covered-switch-default" "-Wno-missing-prototypes" "-Wno-cast-qual" "-Wno-sign-conversion")
        add_definitions("-DDXTWRAPPER_USE_LIBLODEPNG")
    elseif (USE_LIB_PNG STREQUAL "SPNG")
        set_property(SOURCE "${CMAKE_SOURCE_DIR}/external/spng.c" APPEND PROPERTY COMPILE_OPTIONS -Wno-unused-macros -Wno-reserved-macro-identifier -Wno-implicit-int-conversion -Wno-sign-conversion -Wno-tautological-value-range-compare -Wno-sign-conversion -Wno-cast-qual -Wno-shorten-64-to-32 -Wno-double-promotion -Wno-unreachable-code-break -Wno-implicit-float-conversion -Wno-covered-switch-default -Wno-float-conversion)
        set_property(SOURCE "${CMAKE_SOURCE_DIR}/external/miniz.c" APPEND PROPERTY COMPILE_OPTIONS "-Wno-comma" "-Wno-sign-conversion" "-Wno-tautological-type-limit-compare" "-Wno-extra-semi-stmt" "-Wno-cast-align" "-Wno-cast-qual" "-Wno-switch-enum" "-Wno-unused-macros")
        add_definitions("-DDXTWRAPPER_USE_LIBSPNG")
        add_definitions("-DSPNG_STATIC" "-DSPNG_USE_MINIZ")
    endif ()

    if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC") # clang-cl
        message("Clang++ frontend variant = MSVC (clang-cl)")
        target_compile_options(${projectName}-${ARCHITECTURE} PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-c++98-compat-pedantic -Wc++${CMAKE_CXX_STANDARD}-compat /EHs)
    elseif (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU") #gnu
        message("CLANG++ frontend variant = GNU")
        target_compile_options(${projectName}-${ARCHITECTURE} PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-c++98-compat-pedantic -Wc++${CMAKE_CXX_STANDARD}-compat -fcxx-exceptions -fexceptions)
    endif ()
endif ()
