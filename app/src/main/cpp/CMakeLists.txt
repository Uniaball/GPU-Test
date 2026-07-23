cmake_minimum_required(VERSION 3.22.1)
project("vulkan_info")

add_library(vulkan_info SHARED vulkan_info.cpp)

target_link_libraries(vulkan_info
    android
    dl
    log)

add_library(vulkan_test SHARED vulkan_test.cpp)

target_include_directories(vulkan_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(vulkan_test
    android
    dl
    log)
