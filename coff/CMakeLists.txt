cmake_minimum_required(VERSION 3.25)

project(coff C)

if(NOT TARGET redasm::redasm)
    find_package(redasm REQUIRED)
endif()

add_library(${PROJECT_NAME} SHARED)

target_sources(${PROJECT_NAME}
    PRIVATE 
        coff.c
        plugin.c
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        redasm::redasm
)
