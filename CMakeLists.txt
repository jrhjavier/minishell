cmake_minimum_required(VERSION 3.23)
project(MiniShell C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(MiniShell
        parser.h
        myshell.c)
