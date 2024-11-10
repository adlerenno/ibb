cmake_minimum_required(VERSION 3.25)
project(BA C)

set(CMAKE_C_STANDARD 23)

add_executable(BA bwt.c
               bwt.h
               data.c
               data.h
               tpool.c
               tpool.h
               popcount.c
               popcount.h
               main.c
)
