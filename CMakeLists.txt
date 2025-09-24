cmake_minimum_required(VERSION 3.20)
project(SokolApp VERSION 1.0.0)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

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
find_package(SDL3 QUIET)
if(NOT SDL3_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG main
        GIT_SHALLOW TRUE
    )
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)  # Disable SDL tests
    FetchContent_MakeAvailable(SDL3)
endif()

# GLAD
include(FetchContent)
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG glad2
)
FetchContent_MakeAvailable(glad)

# Sokol
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
    list(APPEND SOURCES src/backends/d3d11_backend.c)
elseif(PLATFORM_OSX)
    list(APPEND SOURCES src/backends/metal_backend.c)
else()
    list(APPEND SOURCES src/backends/opengl_backend.c)
endif()

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    src/backends
    ${sokol_SOURCE_DIR}
    ${glad_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
)

# Platform-specific configurations
if(PLATFORM_WINDOWS)
    # Windows - D3D11
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
elseif(PLATFORM_OSX)
    # macOS - Metal
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        SOKOL_METAL
        CIMGUI_USE_METAL
    )
    find_library(METAL_LIBRARY Metal)
    find_library(METALKIT_LIBRARY MetalKit)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(QUARTZCORE_LIBRARY QuartzCore)
    
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${METAL_LIBRARY}
        ${METALKIT_LIBRARY}
        ${COCOA_LIBRARY}
        ${QUARTZCORE_LIBRARY}
    )
    
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
        glad
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