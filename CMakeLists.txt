cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_COMPILER gcc-12)
set(CMAKE_CXX_COMPILER g++-12)

project(GENERATIONS VERSION 1.0)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_CLANG_TIDY "Enable clang-tidy analysis during build" OFF)
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
    else()
        message(FATAL_ERROR "ENABLE_CLANG_TIDY is ON but clang-tidy was not found")
    endif()
endif()

if(ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    else()
        message(FATAL_ERROR "ENABLE_SANITIZERS requires GCC or Clang")
    endif()
endif()

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wno-old-style-cast
    -Winit-self
    -Wno-unused
    -Wno-shadow
    -Wno-parentheses
    -Wlogical-op
    -Wredundant-decls
    -Wcast-align
    -Wno-sign-promo
    -Wno-missing-field-initializers
    -Wmissing-include-dirs
    -Woverloaded-virtual
    -Wctor-dtor-privacy
)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/libraries)


set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

file(GLOB_RECURSE CAPITALENGINE_SOURCES CONFIGURE_DEPENDS
    RELATIVE "${CMAKE_SOURCE_DIR}"
    src/*.cpp
)

find_package(Vulkan)

add_custom_target(architecture_check
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/check_folder_dependencies.py
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Checking folder dependency boundaries"
)

add_custom_command(
    OUTPUT
        ${PROJECT_SOURCE_DIR}/src/vulkan_pipelines/ShaderInterface.h
        ${PROJECT_SOURCE_DIR}/shaders/ParameterUBO.glsl
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/generate_shader_interface.py
    DEPENDS
        ${PROJECT_SOURCE_DIR}/src/vulkan_resources/ParameterUBO.schema
        ${PROJECT_SOURCE_DIR}/tools/generate_shader_interface.py
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generating shader interface from schema"
)

add_custom_target(shader_interface DEPENDS
    ${PROJECT_SOURCE_DIR}/src/vulkan_pipelines/ShaderInterface.h
    ${PROJECT_SOURCE_DIR}/shaders/ParameterUBO.glsl
)

add_executable(CapitalEngine ${CAPITALENGINE_SOURCES})
add_dependencies(CapitalEngine architecture_check)
add_dependencies(CapitalEngine shader_interface)

target_precompile_headers(CapitalEngine PRIVATE
    ${PROJECT_SOURCE_DIR}/src/pch.h
)

if(TARGET Vulkan::Vulkan)
    target_link_libraries(CapitalEngine glfw Vulkan::Vulkan)
else()
    target_link_libraries(CapitalEngine glfw vulkan)
endif()
