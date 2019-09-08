cmake_minimum_required(VERSION 3.13)
project(CompOrgLab1)

set(CMAKE_CXX_STANDARD 14)

include_directories(src)

add_executable(CompOrgLab1
        src/Makefile
        src/mu-mips.c
        src/mu-mips.exe
        src/mu-mips.h
        src/test1.in
        src/test2.in
        src/test3.in)
