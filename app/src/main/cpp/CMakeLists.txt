cmake_minimum_required(VERSION 3.22.1)
project("vulkan_info")

add_library(vulkan_info SHARED vulkan_info.cpp)

target_link_libraries(vulkan_info
    android
    dl
    log)
