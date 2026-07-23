cmake_minimum_required(VERSION 3.22.1)
project("vulkan_info")

add_library(vulkan_info SHARED vulkan_info.cpp)

target_link_libraries(vulkan_info
    android
    dl
    log)

# ============================================================================
# Shader compilation: GLSL -> SPIR-V -> C header (uint32_t array)
# Uses glslc from Android NDK (shaderc) when available.
# The committed _spv.h headers are used as fallback when glslc is not found.
# To manually regenerate: glslc -o shader.spv shader.vert -fshader-stage=vertex
#                         python3 scripts/spv_to_header.py shader.spv shader_spv.h shader_spv
# ============================================================================
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

# Try to find glslc (ships with Android NDK)
find_program(GLSLC_EXECUTABLE glslc
    HINTS
        ${ANDROID_NDK}/toolchains/llvm/prebuilt/*/bin
        ${ANDROID_NDK_HOME}/toolchains/llvm/prebuilt/*/bin
    NO_DEFAULT_PATH
)
find_program(GLSLC_EXECUTABLE glslc)

find_program(PYTHON_EXECUTABLE NAMES python3 python)

if(GLSLC_EXECUTABLE AND PYTHON_EXECUTABLE)
    message(STATUS "Found glslc: ${GLSLC_EXECUTABLE}")
    message(STATUS "Found python: ${PYTHON_EXECUTABLE}")

    set(SPV_TO_HEADER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/spv_to_header.py)

    # Compile and convert vertex shader
    add_custom_command(
        OUTPUT ${SHADER_DIR}/sphere_vert_spv.h
        COMMAND ${GLSLC_EXECUTABLE} -o ${SHADER_DIR}/sphere_vert.spv
                ${SHADER_DIR}/sphere.vert -fshader-stage=vertex
        COMMAND ${PYTHON_EXECUTABLE} ${SPV_TO_HEADER_SCRIPT}
                ${SHADER_DIR}/sphere_vert.spv
                ${SHADER_DIR}/sphere_vert_spv.h
                sphere_vert_spv
        DEPENDS ${SHADER_DIR}/sphere.vert ${SPV_TO_HEADER_SCRIPT}
        COMMENT "Compiling sphere.vert -> SPIR-V -> header"
        VERBATIM
    )

    # Compile and convert fragment shader
    add_custom_command(
        OUTPUT ${SHADER_DIR}/sphere_frag_spv.h
        COMMAND ${GLSLC_EXECUTABLE} -o ${SHADER_DIR}/sphere_frag.spv
                ${SHADER_DIR}/sphere.frag -fshader-stage=fragment
        COMMAND ${PYTHON_EXECUTABLE} ${SPV_TO_HEADER_SCRIPT}
                ${SHADER_DIR}/sphere_frag.spv
                ${SHADER_DIR}/sphere_frag_spv.h
                sphere_frag_spv
        DEPENDS ${SHADER_DIR}/sphere.frag ${SPV_TO_HEADER_SCRIPT}
        COMMENT "Compiling sphere.frag -> SPIR-V -> header"
        VERBATIM
    )

    add_custom_target(shader_headers ALL
        DEPENDS
            ${SHADER_DIR}/sphere_vert_spv.h
            ${SHADER_DIR}/sphere_frag_spv.h
    )
else()
    if(NOT GLSLC_EXECUTABLE)
        message(WARNING "glslc not found; using pre-generated SPIR-V headers. "
                        "Install Android NDK to enable automatic shader compilation.")
    endif()
    if(NOT PYTHON_EXECUTABLE)
        message(WARNING "python3 not found; SPIR-V headers will not be auto-regenerated.")
    endif()
endif()

add_library(vulkan_test SHARED vulkan_test.cpp)

target_include_directories(vulkan_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(vulkan_test
    android
    dl
    log)
