cmake_minimum_required(VERSION 3.20)

file(READ "VERSION" PROJECT_VERSION_FILE)
string(STRIP "${PROJECT_VERSION_FILE}" PROJECT_VERSION_STR)

project(Pholio VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure version header
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/Config.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/core/Config.hpp
)

# Find packages
find_package(exiv2 CONFIG REQUIRED)

# Find System FFmpeg (via PkgConfig)
find_package(PkgConfig REQUIRED)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVUTIL REQUIRED libavutil)

find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(Backward CONFIG REQUIRED)
find_package(nfd CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(xxHash CONFIG REQUIRED)

# Executable
add_executable(Pholio 
    src/main.cpp
    src/core/CrashHandler.cpp
    src/core/CrashHandler.hpp
    src/engine/Scanner.cpp
    src/engine/MediaAnalyzer.cpp
    src/engine/StructureAnalyzer.cpp
    src/engine/Verifier.cpp
    src/engine/Sorter.cpp
    src/engine/Worker.cpp
    src/engine/Worker.hpp
    src/core/ConfigManager.cpp
    src/core/ConfigManager.hpp
    src/core/Loggable.hpp
    src/ui/AppWindow.cpp
    src/ui/AppWindow.hpp
    src/ui/DashboardWindow.cpp
    src/ui/DashboardWindow.hpp
    src/ui/AboutWindow.cpp
    src/ui/AboutWindow.hpp
    src/ui/ChangelogWindow.cpp
    src/ui/ChangelogWindow.hpp
    src/ui/SettingsWindow.cpp
    src/ui/SettingsWindow.hpp
    src/ui/LogWindow.cpp
    src/ui/LogWindow.hpp
    src/ui/Texture.cpp
    src/ui/Texture.hpp
    src/ui/ProgressWindow.cpp
    src/ui/ProgressWindow.hpp
)

# Link dependencies
target_link_libraries(Pholio PRIVATE
    Exiv2::exiv2lib
    Backward::Backward
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

# stb is header-only
target_include_directories(Pholio PRIVATE ${Stb_INCLUDE_DIR})

# Add include directories
target_include_directories(Pholio PRIVATE
    ${AVFORMAT_INCLUDE_DIRS}
    ${AVCODEC_INCLUDE_DIRS}
    ${AVUTIL_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)

# Output build system status
message(STATUS "Pholio version ${PROJECT_VERSION_STR} build system initialized")
