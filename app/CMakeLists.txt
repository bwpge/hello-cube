cmake_minimum_required(VERSION 3.27)
project(hvkapp VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(HTAPP_SRC_FILES
    src/main.cpp
)

add_executable(${PROJECT_NAME} ${HTAPP_SRC_FILES})

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

target_link_libraries(${PROJECT_NAME} hvklib::hvklib)
