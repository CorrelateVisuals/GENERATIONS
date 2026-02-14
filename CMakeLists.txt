cmake_minimum_required(VERSION 3.20)

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
    src/*.h
)

set(SHADER_DIR ${PROJECT_SOURCE_DIR}/shaders)
file(GLOB SHADERS ${SHADER_DIR}/*.vert ${SHADER_DIR}/*.frag ${SHADER_DIR}/*.comp ${SHADER_DIR}/*.geom ${SHADER_DIR}/*.tesc ${SHADER_DIR}/*.tese ${SHADER_DIR}/*.mesh ${SHADER_DIR}/*.task ${SHADER_DIR}/*.rgen ${SHADER_DIR}/*.rchit ${SHADER_DIR}/*.rmiss)

find_package(Vulkan)

# Use glslc if available, otherwise fall back to glslangValidator
if(Vulkan_GLSLC_EXECUTABLE)
    set(SHADER_COMPILER ${Vulkan_GLSLC_EXECUTABLE})
    foreach(SHADER IN LISTS SHADERS)
        get_filename_component(FILENAME ${SHADER} NAME)
        string(REPLACE "shader." "" new_name ${FILENAME})
        add_custom_command(OUTPUT ${SHADER_DIR}/${new_name}.spv
            COMMAND ${SHADER_COMPILER} ${SHADER} -o ${SHADER_DIR}/${new_name}.spv
            DEPENDS ${SHADER}
            COMMENT "Compiling ${FILENAME}")
        list(APPEND SPV_SHADERS ${SHADER_DIR}/${new_name}.spv)
    endForeach()
elseif(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
    set(SHADER_COMPILER ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE})
    foreach(SHADER IN LISTS SHADERS)
        get_filename_component(FILENAME ${SHADER} NAME)
        string(REPLACE "shader." "" new_name ${FILENAME})
        add_custom_command(OUTPUT ${SHADER_DIR}/${new_name}.spv
            COMMAND ${SHADER_COMPILER} -V ${SHADER} -o ${SHADER_DIR}/${new_name}.spv
            DEPENDS ${SHADER}
            COMMENT "Compiling ${FILENAME}")
        list(APPEND SPV_SHADERS ${SHADER_DIR}/${new_name}.spv)
    endForeach()
else()
    message(FATAL_ERROR "No shader compiler found. Install glslang-tools or shaderc.")
endif()

add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})

add_custom_target(architecture_check
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/check_folder_dependencies.py
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Checking folder dependency boundaries"
)

add_executable(CapitalEngine ${CAPITALENGINE_SOURCES} ${SHADERS})
add_dependencies(CapitalEngine architecture_check)

target_link_libraries(CapitalEngine glfw)
target_link_libraries(CapitalEngine vulkan)
