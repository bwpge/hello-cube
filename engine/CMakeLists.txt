cmake_minimum_required(VERSION 3.27)
project(hvklib VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ENGINE_HEADER_FILES
    "include/hvk/allocator.hpp"
    "include/hvk/camera.hpp"
    "include/hvk/core.hpp"
    "include/hvk/debug_utils.hpp"
    "include/hvk/descriptor_utils.hpp"
    "include/hvk/depth_buffer.hpp"
    "include/hvk/engine.hpp"
    "include/hvk/hello_vulkan.hpp"
    "include/hvk/mesh.hpp"
    "include/hvk/pipeline_builder.hpp"
    "include/hvk/resource_manager.hpp"
    "include/hvk/scene.hpp"
    "include/hvk/shader.hpp"
    "include/hvk/texture.hpp"
    "include/hvk/timer.hpp"
    "include/hvk/types.hpp"
    "include/hvk/buffer.hpp"
    "include/hvk/upload_context.hpp"
    "include/hvk/vk_context.hpp"
)

set(ENGINE_SRC_FILES
    "src/allocator.cpp"
    "src/camera.cpp"
    "src/debug_utils.cpp"
    "src/descriptor_utils.cpp"
    "src/depth_buffer.cpp"
    "src/engine.cpp"
    "src/logger.hpp"
    "src/logger.cpp"
    "src/mesh.cpp"
    "src/pipeline_builder.cpp"
    "src/resource_manager.cpp"
    "src/scene.cpp"
    "src/shader.cpp"
    "src/texture.cpp"
    "src/timer.cpp"
    "src/buffer.cpp"
    "src/upload_context.cpp"
    "src/vk_context.cpp"
)

add_library(${PROJECT_NAME} STATIC ${ENGINE_HEADER_FILES};${ENGINE_SRC_FILES})
add_library(hvklib::hvklib ALIAS hvklib)

# compiler specific options
if(MSVC)
    # use static runtime linking on msvc
    set_property(TARGET ${PROJECT_NAME}
        PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# precompiled header
target_precompile_headers(${PROJECT_NAME}
    PRIVATE
        "<algorithm>"
        "<filesystem>"
        "<fstream>"
        "<functional>"
        "<limits>"
        "<memory>"
        "<optional>"
        "<set>"
        "<stdexcept>"
        "<sstream>"
        "<string>"
        "<string_view>"
        "<unordered_map>"
        "<utility>"
        "<vector>"
        "<vulkan/vulkan.hpp>"
        "<vk_mem_alloc.h>"
        "<stb_image.h>"
)

# configure options
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# NOTE: find_package needs to know about the VulkanSDK, which should be configured
#   through the SDK installer, however the editor may not know about the include
#   directory. This is a separate issue that needs to be configured through the
#   editor/extension/plugin. The other packages can be found through vcpkg by
#   setting the CMAKE_TOOLCHAIN_FILE variable (through editor or in this script).
message(STATUS "Configuring package: Vulkan")
find_package(Vulkan REQUIRED)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)
message("   -> lib:     ${Vulkan_LIBRARIES}")
message("   -> include: ${Vulkan_INCLUDE_DIR}")
message("   -> glslc:   ${glslc_executable}")
message(STATUS "Configuring package: VulkanMemoryAllocator")
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
message(STATUS "Configuring package: spdlog")
find_package(spdlog CONFIG REQUIRED)
message(STATUS "Configuring package: glfw")
find_package(glfw3 CONFIG REQUIRED)
message(STATUS "Configuring package: glm")
find_package(glm CONFIG REQUIRED)
message(STATUS "Configuring package: tinyobjloader")
find_package(tinyobjloader CONFIG REQUIRED)
message(STATUS "Configuring package: stb")
find_package(Stb REQUIRED)

# build shaders
set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SHADER_BINARY_DIR "${CMAKE_BINARY_DIR}/shaders")

# file globs are fine here but require a full rebuild to discover new shader
# files, and gets quite confusing if not acutely aware this is a requirement
set(SHADER_SRC_FILES
    "${SHADER_SOURCE_DIR}/mesh.vert"
    "${SHADER_SOURCE_DIR}/mesh.frag"
    "${SHADER_SOURCE_DIR}/hello_triangle.vert"
    "${SHADER_SOURCE_DIR}/hello_triangle.frag"
    "${SHADER_SOURCE_DIR}/wireframe.frag"
    "${SHADER_SOURCE_DIR}/textured_lit.vert"
    "${SHADER_SOURCE_DIR}/textured_lit.frag"
)

add_custom_command(
    COMMAND
    ${CMAKE_COMMAND} -E make_directory ${SHADER_BINARY_DIR}
    OUTPUT ${SHADER_BINARY_DIR}
    COMMENT "Creating ${SHADER_BINARY_DIR}"
)

foreach(SHADER_SRC IN LISTS SHADER_SRC_FILES)
    get_filename_component(FILE_NAME ${SHADER_SRC} NAME)
    set(SPV_SHADER "${SHADER_BINARY_DIR}/${FILE_NAME}.spv")
    add_custom_command(
        COMMAND ${glslc_executable} -o "${SPV_SHADER}" "${SHADER_SRC}"
        OUTPUT "${SPV_SHADER}"
        DEPENDS ${SHADER_SRC} ${SHADER_BINARY_DIR}
        COMMENT "Compiling shader: ${FILE_NAME}"
    )
    list(APPEND SPV_SHADERS "${SPV_SHADER}")
endforeach()
add_custom_target(compile-shaders ALL DEPENDS ${SPV_SHADERS})
add_dependencies(hvklib compile-shaders)

set(MODEL_SRC_DIR "${CMAKE_SOURCE_DIR}/assets")
set(MODEL_OUT_DIR "${CMAKE_BINARY_DIR}/assets")
add_custom_target(copy-assets
    COMMAND
    ${CMAKE_COMMAND} -E copy_directory ${MODEL_SRC_DIR} ${MODEL_OUT_DIR}
)
add_dependencies(hvklib copy-assets)

target_compile_definitions(hvklib
    PUBLIC
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_ENABLE_EXPERIMENTAL
)

# includes
target_include_directories(${PROJECT_NAME}
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
        "$<BUILD_INTERFACE:${Vulkan_INCLUDE_DIR}>"
)

# linking
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        glfw
        spdlog::spdlog
    PRIVATE
        Vulkan::Vulkan
        glm::glm
        GPUOpen::VulkanMemoryAllocator
        tinyobjloader::tinyobjloader
)
