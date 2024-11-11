cmake_minimum_required(VERSION 3.25)

project(IBB VERSION 1.0.0 DESCRIPTION "BWT construction algorithm based for length-diverse datasets")

set(CMAKE_C_STANDARD 23)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")


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

set(SOURCES
        bwt.c
        data.c
        tpool.c
        values.c
        constants.h
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER bwt.h)

add_executable(${PROJECT_NAME}-cli main.c)
add_dependencies(${PROJECT_NAME}-cli ${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME}-cli PRIVATE ${PROJECT_NAME})


