cmake_minimum_required(VERSION 3.20)

set(PHOLIO_VERSION "1.0.30")

project(Pholio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable ccache if available to speed up recompilation
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "ccache found, enabling compiler cache")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

# Enable Interprocedural Optimization (LTO) for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT error)
    if(ipo_supported)
        message(STATUS "IPO/LTO supported, enabling for optimized build")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "IPO/LTO not supported: ${error}")
    endif()
endif()

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
if(APPLE)
    add_executable(Pholio MACOSX_BUNDLE
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
        src/core/ReportManager.cpp
        src/core/ReportManager.hpp
        src/core/UpdateManager.cpp
        src/core/UpdateManager.hpp
        src/ui/AppWindow.cpp
        src/ui/AppWindow.hpp
        src/ui/DebugWindow.cpp
        src/ui/DebugWindow.hpp
        src/ui/DashboardWindow.cpp
        src/ui/DashboardWindow.hpp
        src/ui/PreviewWindow.cpp
        src/ui/PreviewWindow.hpp
        src/ui/AboutWindow.cpp
        src/ui/AboutWindow.hpp
        src/ui/ChangelogWindow.cpp
        src/ui/ChangelogWindow.hpp
        src/ui/ReportWindow.cpp
        src/ui/ReportWindow.hpp
        src/ui/SettingsWindow.cpp
        src/ui/SettingsWindow.hpp
        src/ui/LogWindow.cpp
        src/ui/LogWindow.hpp
        src/ui/Texture.cpp
        src/ui/Texture.hpp
        src/ui/ProgressWindow.cpp
        src/ui/ProgressWindow.hpp
    )
    
    set_target_properties(Pholio PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/packaging/macOS/Info.plist
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.pholio.cpp"
        MACOSX_BUNDLE_BUNDLE_NAME "Pholio"
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PHOLIO_VERSION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PHOLIO_VERSION}
    )
else()
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
        src/core/ReportManager.cpp
        src/core/ReportManager.hpp
        src/core/UpdateManager.cpp
        src/core/UpdateManager.hpp
        src/ui/AppWindow.cpp
        src/ui/AppWindow.hpp
        src/ui/DebugWindow.cpp
        src/ui/DebugWindow.hpp
        src/ui/DashboardWindow.cpp
        src/ui/DashboardWindow.hpp
        src/ui/PreviewWindow.cpp
        src/ui/PreviewWindow.hpp
        src/ui/AboutWindow.cpp
        src/ui/AboutWindow.hpp
        src/ui/ChangelogWindow.cpp
        src/ui/ChangelogWindow.hpp
        src/ui/ReportWindow.cpp
        src/ui/ReportWindow.hpp
        src/ui/SettingsWindow.cpp
        src/ui/SettingsWindow.hpp
        src/ui/LogWindow.cpp
        src/ui/LogWindow.hpp
        src/ui/Texture.cpp
        src/ui/Texture.hpp
        src/ui/ProgressWindow.cpp
        src/ui/ProgressWindow.hpp
    )
endif()

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
message(STATUS "Pholio version ${PHOLIO_VERSION} build system initialized")
