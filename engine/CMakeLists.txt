cmake_minimum_required(VERSION 3.27)
project(hclib VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(HCENGINE_HEADER_FILES
    "include/debug_utils.hpp"
    "include/engine.hpp"
    "include/hello_cube.hpp"
    "include/types.hpp"
    "src/logger.hpp"
)

set(HCENGINE_SRC_FILES
    "src/debug_utils.cpp"
    "src/engine.cpp"
    "src/logger.cpp"
)

add_library(${PROJECT_NAME} STATIC ${HCENGINE_HEADER_FILES};${HCENGINE_SRC_FILES})
add_library(hclib::hclib ALIAS hclib)

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
message(STATUS "Configuring package: spdlog")
find_package(spdlog CONFIG REQUIRED)
message(STATUS "Configuring package: glfw")
find_package(glfw3 CONFIG REQUIRED)
message(STATUS "Configuring package: glm")
find_package(glm CONFIG REQUIRED)

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
)
