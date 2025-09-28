cmake_minimum_required(VERSION 3.20)
project(SokolApp VERSION 1.0.0)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(USE_D3D11 "Use Direct3D11 backend on Windows" OFF)

# Platform detection
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
elseif(APPLE)
    set(PLATFORM_OSX TRUE)
else()
    set(PLATFORM_UNIX TRUE)
endif()

# Find packages
find_package(PkgConfig REQUIRED)

# SDL3
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)

find_package(SDL3 QUIET)
if(NOT SDL3_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG main
        GIT_SHALLOW TRUE
    )

    FetchContent_MakeAvailable(SDL3)
endif()

# Sokol
include(FetchContent)
FetchContent_Declare(
    sokol
    GIT_REPOSITORY https://github.com/floooh/sokol.git
    GIT_TAG master
)
FetchContent_MakeAvailable(sokol)

# Source files
set(SOURCES
    src/main.c
    src/renderer.c
    src/window.c
)

# Platform-specific sources
if(PLATFORM_WINDOWS)
    if(USE_D3D11)
        list(APPEND SOURCES src/backends/d3d11_backend.c)
    else()
        list(APPEND SOURCES src/backends/opengl_backend.c)
    endif()
else()
    # Use OpenGL for both macOS and Linux/Unix
    list(APPEND SOURCES src/backends/opengl_backend.c)
endif()

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    src/backends
    ${sokol_SOURCE_DIR}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
)

# Platform-specific configurations
if(PLATFORM_WINDOWS)
    # Windows - D3D11
    if(USE_D3D11)
        target_compile_definitions(${PROJECT_NAME} PRIVATE
            SOKOL_D3D11
            CIMGUI_USE_D3D11
            WIN32_LEAN_AND_MEAN
        )
        target_link_libraries(${PROJECT_NAME} PRIVATE
            d3d11
            dxgi
            d3dcompiler
        )
    else()
        target_compile_definitions(${PROJECT_NAME} PRIVATE
            SOKOL_GLCORE
            CIMGUI_USE_OPENGL3
        )
        find_package(OpenGL REQUIRED)
        target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
    endif()
elseif(PLATFORM_OSX)
    # macOS - OpenGL (instead of Metal)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        SOKOL_GLCORE
        CIMGUI_USE_OPENGL3
    )
    
    # Find OpenGL for macOS
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
    
    # macOS-specific frameworks for OpenGL
    find_library(COCOA_LIBRARY Cocoa)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${COCOA_LIBRARY})
    
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist
    )
else()
    # Unix/Linux - OpenGL
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        SOKOL_GLCORE
        CIMGUI_USE_OPENGL3
    )
    
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${CMAKE_DL_LIBS}
    )
    
    # Find OpenGL
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
    
    # Find X11 for Linux
    find_package(X11 REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_LIBRARIES})
endif()

# Compiler-specific options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

# Debug configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=1)
endif()

# Create directories
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/assets)

# Copy assets if they exist
if(EXISTS ${CMAKE_SOURCE_DIR}/assets)
    file(COPY ${CMAKE_SOURCE_DIR}/assets/ DESTINATION ${CMAKE_BINARY_DIR}/assets/)
endif()