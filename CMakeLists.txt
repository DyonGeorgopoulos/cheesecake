cmake_minimum_required(VERSION 3.20)
project(SokolApp VERSION 1.0.0)
set(CMAKE_C_STANDARD 11)
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
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(SDL3)
endif()

# FreeType
find_package(Freetype QUIET)
if(NOT Freetype_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        freetype
        GIT_REPOSITORY https://github.com/freetype/freetype.git
        GIT_TAG master
        GIT_SHALLOW TRUE
    )
    
    # FreeType build options
    set(FT_DISABLE_ZLIB ON CACHE BOOL "Disable zlib support")
    set(FT_DISABLE_BZIP2 ON CACHE BOOL "Disable bzip2 support")
    set(FT_DISABLE_PNG ON CACHE BOOL "Disable PNG support")
    set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "Disable HarfBuzz support")
    set(FT_DISABLE_BROTLI ON CACHE BOOL "Disable Brotli support")
    
    FetchContent_MakeAvailable(freetype)
    
    # Create alias for consistency
    add_library(Freetype::Freetype ALIAS freetype)
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
    src/font_rendering.c
)

# Platform-specific sources
if(PLATFORM_WINDOWS)
    list(APPEND SOURCES src/backends/d3d11_backend.c)
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
    Freetype::Freetype
)

# Platform-specific configurations
if(PLATFORM_WINDOWS)
    # Windows - D3D11
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        SOKOL_D3D11
        WIN32_LEAN_AND_MEAN
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
        d3d11
        dxgi
        d3dcompiler
    )
elseif(PLATFORM_OSX)
    # macOS - OpenGL (instead of Metal)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        SOKOL_GLCORE
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
# Create directories and copy assets
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/assets/fonts)

# Copy all fonts from source to build directory
file(GLOB FONT_FILES "${CMAKE_SOURCE_DIR}/assets/fonts/*")
foreach(FONT_FILE ${FONT_FILES})
    file(COPY ${FONT_FILE} DESTINATION ${CMAKE_BINARY_DIR}/assets/fonts/)
endforeach(

)
# Copy assets if they exist
if(EXISTS ${CMAKE_SOURCE_DIR}/assets)
    file(COPY ${CMAKE_SOURCE_DIR}/assets/ DESTINATION ${CMAKE_BINARY_DIR}/assets/)
endif()