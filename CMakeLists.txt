cmake_minimum_required(VERSION 3.28)
project(Mirage)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
add_subdirectory(third_party/glm)
add_subdirectory(third_party/openxr)
add_subdirectory(third_party/sdl3)

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(third_party/assimp)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(assimp PRIVATE -Wno-error=unsafe-buffer-usage -Wno-unsafe-buffer-usage)
endif()

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE third_party/stb)

set(IMGUI_SOURCES
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_vulkan.cpp
    third_party/imgui/backends/imgui_impl_sdl3.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})

target_include_directories(imgui
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends
)

target_link_libraries(imgui
    PUBLIC
    Vulkan::Vulkan
    SDL3::SDL3
)

file(GLOB_RECURSE SRC_FILES src/*.cpp)

add_executable(Mirage ${SRC_FILES})

target_link_libraries(Mirage PRIVATE glm stb openxr_loader assimp::assimp Vulkan::Vulkan SDL3::SDL3 imgui)

find_program(GLSL_VALIDATOR glslangValidator HINTS Vulkan::glslangValidator /usr/bin /usr/local/bin ${VULKAN_SDK}/Bin/ ${VULKAN_SDK}/Bin32/)
if(NOT GLSL_VALIDATOR)
    message(FATAL_ERROR "glslangValidator not found! Please ensure Vulkan SDK is installed")
endif()

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${CMAKE_SOURCE_DIR}/shaders/*.frag"
    "${CMAKE_SOURCE_DIR}/shaders/*.vert"
    "${CMAKE_SOURCE_DIR}/shaders/*.comp"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${CMAKE_SOURCE_DIR}/shaders/${FILE_NAME}.spv")
    get_filename_component(SPIRV_NAME ${SPIRV} NAME)
    
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL}
        COMMENT "Compiling shader ${FILE_NAME} -> ${SPIRV_NAME}"
    )
    
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach()

add_custom_target(
    Shaders
    DEPENDS ${SPIRV_BINARY_FILES}
)

add_dependencies(Mirage Shaders)

file(GLOB SPIRV_FILES "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.spv")
file(COPY ${SPIRV_FILES} DESTINATION "${CMAKE_BINARY_DIR}/shaders")
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
