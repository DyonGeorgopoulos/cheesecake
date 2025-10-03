cmake_minimum_required(VERSION 3.20)
project(Cheesecake VERSION 1.0.0)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Force output directories to be consistent across platforms
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR})

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

# Flecs
include(FetchContent)
FetchContent_Declare(
    flecs
    GIT_REPOSITORY https://github.com/SanderMertens/flecs.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(flecs)

# Sokol
FetchContent_Declare(
    sokol
    GIT_REPOSITORY https://github.com/floooh/sokol.git
    GIT_TAG d56a553235252b7b5e1b2e9f7bce5e71caf8838e
)
FetchContent_MakeAvailable(sokol)

# Sokol GP
FetchContent_Declare(
    sokol_gp
    GIT_REPOSITORY https://github.com/edubart/sokol_gp.git
    GIT_TAG master
)
FetchContent_MakeAvailable(sokol_gp)

# Sokol Tools (for shader compilation)
FetchContent_Declare(
    sokol-tools-bin
    GIT_REPOSITORY https://github.com/floooh/sokol-tools-bin.git
    GIT_TAG 3388e3b2c04b532a55b072d79e77c9fc44c3ca50
)
FetchContent_MakeAvailable(sokol-tools-bin)

# Copy sokol_gp.h to sokol directory for easy inclusion
add_custom_command(
    OUTPUT ${sokol_SOURCE_DIR}/sokol_gp.h
    COMMAND ${CMAKE_COMMAND} -E copy
        ${sokol_gp_SOURCE_DIR}/sokol_gp.h
        ${sokol_SOURCE_DIR}/sokol_gp.h
    DEPENDS ${sokol_gp_SOURCE_DIR}/sokol_gp.h
    COMMENT "Copying sokol_gp.h to sokol directory"
)

# Create a custom target that depends on the copied file
add_custom_target(copy_sokol_gp ALL
    DEPENDS ${sokol_SOURCE_DIR}/sokol_gp.h
)

# Determine the correct sokol-shdc executable based on platform
if(WIN32)
    set(SOKOL_SHDC "${sokol-tools-bin_SOURCE_DIR}/bin/win32/sokol-shdc.exe")
elseif(APPLE)
    set(SOKOL_SHDC "${sokol-tools-bin_SOURCE_DIR}/bin/osx/sokol-shdc")
elseif(UNIX)
    set(SOKOL_SHDC "${sokol-tools-bin_SOURCE_DIR}/bin/linux/sokol-shdc")
endif()

# Function to compile shaders
function(compile_shader INPUT_FILE OUTPUT_FILE)
    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${SOKOL_SHDC}
            --input ${INPUT_FILE}
            --output ${OUTPUT_FILE}
            --slang glsl410:glsl300es:hlsl4:metal_macos:wgsl
        DEPENDS ${INPUT_FILE}
        COMMENT "Compiling shader ${INPUT_FILE}"
        VERBATIM
    )
endfunction()

# Compile shaders
compile_shader(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shader.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shader.glsl.h
)

compile_shader(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/font.shader.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/font.shader.glsl.h
)

# Source files
set(SOURCES
    src/main.c
    src/renderer.c
    src/window.c
    src/font_rendering.c
    src/sokol.c
    src/components/rendering.c
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

# Make sure shaders are compiled before building
add_custom_target(compile_shaders ALL
    DEPENDS 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/shader.glsl.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/font.shader.glsl.h
)

add_dependencies(${PROJECT_NAME} compile_shaders)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    src/backends
    src/components
    src/systems
    ${sokol_SOURCE_DIR}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    Freetype::Freetype
    flecs::flecs_static
)

# Platform-specific configurations
if(PLATFORM_WINDOWS)
    # Windows - D3D11
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        SOKOL_D3D11
        WIN32_LEAN_AND_MEAN
        SDL_MAIN_USE_CALLBACKS
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
        d3d11
        dxgi
        d3dcompiler
    )
    
    # Set working directory for Visual Studio
    set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
elseif(PLATFORM_OSX)
    # macOS - OpenGL (instead of Metal)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        SOKOL_GLCORE
        SDL_MAIN_USE_CALLBACKS
    )
    
    # Find OpenGL for macOS
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
    
    # macOS-specific frameworks for OpenGL
    find_library(COCOA_LIBRARY Cocoa)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${COCOA_LIBRARY})
    
    # Disable bundle creation to get a regular executable
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE FALSE
    )
else()
    # Unix/Linux - OpenGL
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        SOKOL_GLCORE
        SDL_MAIN_USE_CALLBACKS
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
    target_compile_options(${PROJECT_NAME} PRIVATE)
endif()

# Debug configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=1)
endif()

# Copy assets to build directory (runs at build time, not configure time)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/assets
    COMMENT "Copying assets to build directory"
)
