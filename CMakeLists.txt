cmake_minimum_required(VERSION 3.25)
project(redasm-commands C)

if(NOT TARGET redasm::redasm)
    find_package(redasm REQUIRED)
endif()

add_subdirectory(coff)
