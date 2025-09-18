cmake_minimum_required(VERSION 3.22)
project(WinChess VERSION 1.0)
set(CMAKE_CXX_STANDARD 17)

# -------------------------------------------------------------
# Detect generator type
# -------------------------------------------------------------
get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

# -------------------------------------------------------------
# Set Runtime Library for msvc
# -------------------------------------------------------------
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "Set Runtime Library for msvc" FORCE)
endif()

# -------------------------------------------------------------
# Create Output Directory Prefix
# -------------------------------------------------------------
if(IS_MULTI_CONFIG)
    if(DEFINED CMAKE_VS_PLATFORM_NAME)
        set(PLATFORM_FOLDER ${CMAKE_VS_PLATFORM_NAME})
    else()
        set(PLATFORM_FOLDER ${CMAKE_SYSTEM_PROCESSOR})
    endif()
    set(BIN_OUTPUT_DIRECTORY_PREFIX ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${PLATFORM_FOLDER})
else()
    if(DEFINED CMAKE_VS_PLATFORM_NAME)
        set(PLATFORM_FOLDER ${CMAKE_VS_PLATFORM_NAME})
    else()
        set(PLATFORM_FOLDER ${CMAKE_SYSTEM_PROCESSOR})
    endif()
    set(BIN_OUTPUT_DIRECTORY_PREFIX ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/${PLATFORM_FOLDER})
endif()

# -------------------------------------------------------------
# Source files
# -------------------------------------------------------------
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")

# -------------------------------------------------------------
# Create main executable target
# -------------------------------------------------------------
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# -------------------------------------------------------------
# Add subdirectories and libraries
# -------------------------------------------------------------
add_subdirectory(external/GLFW)
add_library(imgui
    external/ImGui/imgui.cpp
    external/ImGui/imgui_draw.cpp
    external/ImGui/imgui_tables.cpp
    external/ImGui/imgui_widgets.cpp
)
add_library(glad
    external/GLAD/src/glad.c
)
find_package(OpenGL REQUIRED)

# -------------------------------------------------------------
# Add Target Properties etc.
# -------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/external/GLFW/include"
    "${CMAKE_SOURCE_DIR}/external/ImGui"
    "${CMAKE_SOURCE_DIR}/external/GLAD/include"
    "${CMAKE_SOURCE_DIR}/external/StbImage"
)
target_include_directories(imgui PRIVATE
    "${CMAKE_SOURCE_DIR}/external/GLFW/include"
    "${CMAKE_SOURCE_DIR}/external/ImGui"
)
target_include_directories(glad PRIVATE "${CMAKE_SOURCE_DIR}/external/GLAD/include")

target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    imgui
    glad
    OpenGL::GL
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIRECTORY_PREFIX}/${PROJECT_NAME}
)
set_target_properties(glfw PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIRECTORY_PREFIX}/GLFW
)
set_target_properties(imgui PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIRECTORY_PREFIX}/ImGui
)
set_target_properties(glad PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIRECTORY_PREFIX}/GLAD
)

# -------------------------------------------------------------
# Visual Studio startup project
# -------------------------------------------------------------
if(MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()

# -------------------------------------------------------------
# Compiler options
# -------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra -Wshadow -Wformat=2 -Wpedantic -DDEBUG>
        $<$<CONFIG:Release>:-O3 -march=native -flto -funroll-loops -DNDEBUG -fstrict-aliasing -ffast-math>
        $<$<CONFIG:RelWithDebInfo>:-O2 -g -DNDEBUG>
        $<$<CONFIG:MinSizeRel>:-Os -DNDEBUG>
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:-flto>
        $<$<CONFIG:RelWithDebInfo>:-flto>
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:/Zi /Od /W4 /DDEBUG /EHsc /permissive->
        $<$<CONFIG:Release>:/O2 /Ob2 /GL /DNDEBUG /EHsc /fp:fast>
        $<$<CONFIG:RelWithDebInfo>:/O2 /Zi /DNDEBUG /EHsc>
        $<$<CONFIG:MinSizeRel>:/O1 /Os /DNDEBUG /EHsc>
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/LTCG>
        $<$<CONFIG:RelWithDebInfo>:/LTCG>
    )
endif()
if(MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC /utf-8)
endif()

# -------------------------------------------------------------
# Enable LTO for release-like builds
# -------------------------------------------------------------
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)