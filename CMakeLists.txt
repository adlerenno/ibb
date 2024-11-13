cmake_minimum_required(VERSION 3.25)



project(IBB VERSION 1.0.0 DESCRIPTION "BWT construction algorithm based for length-diverse datasets")

set(CMAKE_C_STANDARD 23)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")

set(SOURCES
        bwt.c
        bwt.h
        data.c
        data.h
        tpool.c
        tpool.h
        constants.h
)



set(POP_COUNT DETECT CACHE STRING "Defines the used pop-count methods")
set_property(CACHE POP_COUNT PROPERTY STRINGS DETECT AVX512 AVX BASIC)
set(AVX512_FLAG "-mavx512vpopcntdq")
set(AVX_FLAG "-mavx")

if (POP_COUNT STREQUAL "DETECT")
    # Check for AVX-512 support in GCC/Clang (Linux/macOS). Does not work on Windows.
    include(CheckCXXCompilerFlag)


    # Check if the compiler accepts the AVX-512 flag
    if (AVX512_FLAG)
        check_cxx_compiler_flag(${AVX512_FLAG} HAS_AVX512_SUPPORT)
    endif()
    if (AVX_FLAG)
        check_cxx_compiler_flag(${AVX512_FLAG} HAS_AVX_SUPPORT)
    endif ()

    if (HAS_AVX512_SUPPORT)
        set(POP_COUNT AVX512)
    elseif(HAS_AVX_SUPPORT)
        set(POP_COUNT AVX)
    else ()
        set(POP_COUNT BASIC)
    endif ()
endif ()

if (POP_COUNT STREQUAL "AVX512")
    add_compile_options(${AVX512_FLAG})
    add_compile_options(-mavx512f)
    list(APPEND SOURCES
            popcount.c
            popcount.h
    )
    message(STATUS "AXV512 Popcount")
elseif (POP_COUNT STREQUAL "AVX")
    add_definitions(${AVX_FLAG})
    list(APPEND SOURCES
            popcount_basic.c
            popcount.h
    )
    message(STATUS "AVX Popcount")
else ()
    list(APPEND SOURCES
            values.c
            values.h
    )
    message(STATUS "Basic Popcount")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    add_definitions(-DCLANG)  # Defines the CLANG Preprocessor directive
endif()

option(ENABLE_TIME_EVALUATION "Outputs time needed for BWT-construction" ON)
if (ENABLE_TIME_EVALUATION)
    add_definitions(-DTIME)
endif ()

option(ENABLE_VERBOSE "Outputs information about steps and progress" ON)
if(ENABLE_VERBOSE)
    add_definitions(-DVERBOSE)
endif()

add_library(${PROJECT_NAME} SHARED ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER bwt.h)

add_executable(${PROJECT_NAME}-cli main.c)
add_dependencies(${PROJECT_NAME}-cli ${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME}-cli PRIVATE ${PROJECT_NAME})


