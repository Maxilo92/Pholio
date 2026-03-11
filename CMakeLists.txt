cmake_minimum_required(VERSION 3.20)

file(READ "VERSION" PROJECT_VERSION_FILE)
string(STRIP "${PROJECT_VERSION_FILE}" PROJECT_VERSION_STR)

project(PhotoSorter VERSION ${PROJECT_VERSION_STR} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(exiv2 CONFIG REQUIRED)

# Find System FFmpeg (via PkgConfig - much faster on macOS/Linux)
find_package(PkgConfig REQUIRED)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVUTIL REQUIRED libavutil)

find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(nfd CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(xxHash CONFIG REQUIRED)

# Executable
add_executable(PhotoSorter 
    src/main.cpp
    src/engine/Scanner.cpp
    src/engine/MediaAnalyzer.cpp
    src/engine/StructureAnalyzer.cpp
    src/engine/Verifier.cpp
    src/engine/Sorter.cpp
    src/engine/Worker.cpp
    src/engine/Worker.hpp
    src/core/ConfigManager.cpp
    src/core/ConfigManager.hpp
    src/ui/AppWindow.cpp
    src/ui/AppWindow.hpp
    src/ui/DashboardWindow.cpp
    src/ui/DashboardWindow.hpp
    src/ui/AboutWindow.cpp
    src/ui/AboutWindow.hpp
    src/ui/SettingsWindow.cpp
    src/ui/SettingsWindow.hpp
    src/ui/LogWindow.cpp
    src/ui/LogWindow.hpp
    src/ui/ProgressWindow.cpp
    src/ui/ProgressWindow.hpp
)

# Link dependencies
target_link_libraries(PhotoSorter PRIVATE
    Exiv2::exiv2lib
    ${AVFORMAT_LIBRARIES}
    ${AVCODEC_LIBRARIES}
    ${AVUTIL_LIBRARIES}
    imgui::imgui
    glfw
    OpenGL::GL
    implot::implot
    nfd::nfd
    nlohmann_json::nlohmann_json
    xxHash::xxhash
)

# stb is header-only and doesn't always provide a target in older vcpkg versions
target_include_directories(PhotoSorter PRIVATE ${Stb_INCLUDE_DIR})

# Add include directories for FFmpeg
target_include_directories(PhotoSorter PRIVATE
    ${AVFORMAT_INCLUDE_DIRS}
    ${AVCODEC_INCLUDE_DIRS}
    ${AVUTIL_INCLUDE_DIRS}
)

# Output build system status
message(STATUS "PhotoSorter version ${PROJECT_VERSION_STR} build system initialized")
