cmake_minimum_required(VERSION 3.25)
project(BA C)

set(CMAKE_C_STANDARD 23)

add_executable(BA bwt.c
        data.c
        data.h
)
