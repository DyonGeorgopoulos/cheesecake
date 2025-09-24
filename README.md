Cross-Platform Sokol + ImGui + SDL3 Project

This project demonstrates a cross-platform application using:

    Sokol for graphics abstraction
    Dear ImGui (via cimgui) for immediate mode GUI
    SDL3 for windowing and input
    GLAD for OpenGL loading (Unix/Linux only)

Platform-Specific Backends

    Windows: Direct3D 11
    macOS: Metal
    Unix/Linux: OpenGL Core 3.3+

Prerequisites
Windows

    Visual Studio 2019 or later with C++ support
    Windows SDK
    CMake 3.20+

macOS

    Xcode command line tools
    CMake 3.20+
    macOS 10.15+

Linux/Unix

    GCC or Clang with C99 support
    CMake 3.20+
    Development packages for:
        OpenGL
        X11 (libx11-dev, libxrandr-dev, libxinerama-dev, libxcursor-dev, libxi-dev)

Building
Clone and Build
bash

git clone <your-repo-url>
cd <project-directory>
mkdir build
cd build
cmake ..
cmake --build .

Platform-Specific Notes
Windows (Visual Studio)
cmd

mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release

macOS
bash

mkdir build
cd build
cmake .. -G "Xcode"
cmake --build . --config Release

Linux
bash

# Install dependencies (Ubuntu/Debian)
sudo apt-get install build-essential cmake libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev

mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

Project Structure

src/
├── main.c              # Main application entry point
├── window.h/c          # Window management (SDL3)
├── renderer.h/c        # Cross-platform renderer interface
└── backends/
    ├── d3d11_backend.c    # Windows D3D11 implementation
    ├── metal_backend.c    # macOS Metal implementation
    └── opengl_backend.c   # Unix/Linux OpenGL implementation

Features

    Cross-platform window creation and event handling
    Platform-appropriate graphics API usage
    ImGui integration with platform-specific backends
    Automatic dependency fetching via CMake FetchContent
    Proper resource cleanup and error handling

Dependencies

All dependencies are automatically fetched by CMake:

    SDL3: Latest from main branch
    GLAD: OpenGL loader generator (glad2 branch)
    Sokol: Single-header graphics libraries
    cimgui: C bindings for Dear ImGui

Troubleshooting
Build Issues

    Ensure you have the correct platform SDK installed
    Check CMake version (3.20+ required)
    Verify graphics drivers are up to date

Runtime Issues

    Windows: Ensure D3D11 runtime is available
    macOS: Check Metal support (macOS 10.13+)
    Linux: Verify OpenGL 3.3+ driver support

ImGui Display Issues

    Check that the appropriate ImGui backend is being used for your platform
    Verify window size and DPI scaling

License

This project template is provided as-is for educational and development purposes.
Check individual dependency licenses for commercial use.
