cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 17)

if (MSVC)
    string(TOLOWER ${CMAKE_VS_PLATFORM_NAME} ARCHITECTURE)
else ()
    string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} ARCHITECTURE)
endif ()

if (ARCHITECTURE STREQUAL "amd64")
    set(ARCHITECTURE "x64")
elseif (ARCHITECTURE STREQUAL "win32")
    set(ARCHITECTURE "x86")
endif ()
message("architecture: ${ARCHITECTURE}")

add_library(${projectName}-${ARCHITECTURE} SHARED "DxTexWrapper.cpp" Data.h)
target_link_libraries(${projectName}-${ARCHITECTURE} "${CMAKE_SOURCE_DIR}/lib/DirectXTex-${ARCHITECTURE}.lib")
target_include_directories(${projectName}-${ARCHITECTURE} PRIVATE "${CMAKE_SOURCE_DIR}/include")

target_compile_features(${projectName}-${ARCHITECTURE} PUBLIC cxx_std_${CMAKE_CXX_STANDARD})

message("C Compiler (${CMAKE_C_COMPILER_ID}) ${CMAKE_C_COMPILER}")
message("C++ Compiler (${CMAKE_CXX_COMPILER_ID}) ${CMAKE_CXX_COMPILER}")
message("C Compiler frontend variant: ${CMAKE_C_COMPILER_FRONTEND_VARIANT}")
message("C++ Compiler frontend variant: ${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}")

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    message("Compiler is MSVC")
    target_compile_options(${projectName}-${ARCHITECTURE} PRIVATE /W4 /WX /EHs)
else()
    if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC") # clang-cl
        message("Clang++ frontend variant = MSVC (clang-cl)")
        target_compile_options(${projectName}-${ARCHITECTURE} PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-c++98-compat-pedantic -Wc++${CMAKE_CXX_STANDARD}-compat /EHs)
    elseif (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU") #gnu
        message("CLANG++ frontend variant = GNU")
        target_compile_options(${projectName}-${ARCHITECTURE} PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-c++98-compat-pedantic -Wc++${CMAKE_CXX_STANDARD}-compat -fcxx-exceptions -fexceptions)
    endif ()
endif()
