cmake_minimum_required(VERSION 3.25)
project(BA C)

set(CMAKE_C_STANDARD 23)

add_executable(BA main.c
        heapify.c
        file.c
        file.h
        bwt.c
        sort.h
        sort.h
        heapify.h)
