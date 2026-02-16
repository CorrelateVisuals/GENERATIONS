cmake_minimum_required(VERSION 3.20)

project(GENERATIONS VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wold-style-cast
    -Winit-self
    -Wno-unused
    -Wshadow
    -Wno-parentheses
    -Wlogical-op
    -Wredundant-decls
    -Wcast-align
    -Wsign-promo
    -Wmissing-include-dirs
    -Woverloaded-virtual
    -Wctor-dtor-privacy
)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/libraries)


set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

file(GLOB_RECURSE CAPITALENGINE_SOURCES
    RELATIVE "${CMAKE_SOURCE_DIR}"
    src/*.cpp
    src/*.h
    stb/*.h
)

set(SHADER_DIR ${PROJECT_SOURCE_DIR}/shaders)
file(GLOB SHADERS ${SHADER_DIR}/*.vert ${SHADER_DIR}/*.frag ${SHADER_DIR}/*.comp ${SHADER_DIR}/*.geom ${SHADER_DIR}/*.tesc ${SHADER_DIR}/*.tese ${SHADER_DIR}/*.mesh ${SHADER_DIR}/*.task ${SHADER_DIR}/*.rgen ${SHADER_DIR}/*.rchit ${SHADER_DIR}/*.rmiss)

find_package(Vulkan)

if(NOT Vulkan_GLSLC_EXECUTABLE)
    find_program(GLSLC_EXECUTABLE glslangValidator)
    if(GLSLC_EXECUTABLE)
        set(Vulkan_GLSLC_EXECUTABLE ${GLSLC_EXECUTABLE})
        set(GLSLC_FLAGS -V)
    endif()
endif()

foreach(SHADER IN LISTS SHADERS)
    get_filename_component(FILENAME ${SHADER} NAME)
    string(REPLACE "shader." "" new_name ${FILENAME})
    add_custom_command(OUTPUT ${SHADER_DIR}/${new_name}.spv
        COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${GLSLC_FLAGS} ${SHADER} -o ${SHADER_DIR}/${new_name}.spv
        DEPENDS ${SHADER}
        COMMENT "Compiling ${FILENAME}")
    list(APPEND SPV_SHADERS ${SHADER_DIR}/${new_name}.spv)
endForeach()

add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})

add_executable(CapitalEngine ${CAPITALENGINE_SOURCES} ${SHADERS})

target_link_libraries(CapitalEngine glfw)
target_link_libraries(CapitalEngine vulkan)
