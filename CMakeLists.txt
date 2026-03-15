cmake_minimum_required(VERSION 3.20)

set(PHOLIO_VERSION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
if(EXISTS "${PHOLIO_VERSION_FILE}")
    file(READ "${PHOLIO_VERSION_FILE}" PHOLIO_VERSION)
    string(STRIP "${PHOLIO_VERSION}" PHOLIO_VERSION)
else()
    message(FATAL_ERROR "VERSION file not found: ${PHOLIO_VERSION_FILE}")
endif()

if(NOT PHOLIO_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "Invalid VERSION '${PHOLIO_VERSION}'. Expected x.y.z")
endif()

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_ROOT} AND EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Path to vcpkg toolchain file")
        message(STATUS "Using vcpkg toolchain from VCPKG_ROOT: ${CMAKE_TOOLCHAIN_FILE}")
    elseif(DEFINED ENV{HOME} AND EXISTS "$ENV{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "$ENV{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Path to vcpkg toolchain file")
        message(STATUS "Using vcpkg toolchain from HOME fallback: ${CMAKE_TOOLCHAIN_FILE}")
    endif()
endif()

if(NOT DEFINED VCPKG_INSTALLED_DIR AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed")
    set(VCPKG_INSTALLED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed"
        CACHE PATH "Use repository-local vcpkg_installed directory")
endif()

if(NOT DEFINED VCPKG_TARGET_TRIPLET AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-osx")
    set(VCPKG_TARGET_TRIPLET "x64-osx" CACHE STRING "Default vcpkg target triplet")
endif()

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
find_package(Exiv2 CONFIG QUIET)
if(NOT TARGET Exiv2::exiv2lib)
    find_package(exiv2 CONFIG REQUIRED)
endif()

if(WIN32)
    # Prefer vcpkg CMake config targets on Windows/MSVC.
    find_package(FFMPEG QUIET)
    if(TARGET FFMPEG::avformat AND TARGET FFMPEG::avcodec AND TARGET FFMPEG::avutil)
        set(PHOLIO_FFMPEG_LIBS
            FFMPEG::avformat
            FFMPEG::avcodec
            FFMPEG::avutil
        )
    else()
        find_package(unofficial-ffmpeg CONFIG REQUIRED)
        set(PHOLIO_FFMPEG_LIBS
            unofficial::ffmpeg::avformat
            unofficial::ffmpeg::avcodec
            unofficial::ffmpeg::avutil
        )
    endif()
else()
    # Use system FFmpeg (via PkgConfig) on Unix-like platforms.
    if(CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
        find_program(SYSTEM_PKG_CONFIG_EXECUTABLE NAMES pkg-config pkgconf)
        if(SYSTEM_PKG_CONFIG_EXECUTABLE)
            set(PKG_CONFIG_EXECUTABLE "${SYSTEM_PKG_CONFIG_EXECUTABLE}" CACHE FILEPATH "pkg-config executable" FORCE)
        endif()
    endif()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(AVFORMAT REQUIRED IMPORTED_TARGET libavformat)
    pkg_check_modules(AVCODEC REQUIRED IMPORTED_TARGET libavcodec)
    pkg_check_modules(AVUTIL REQUIRED IMPORTED_TARGET libavutil)
    set(PHOLIO_FFMPEG_LIBS
        PkgConfig::AVFORMAT
        PkgConfig::AVCODEC
        PkgConfig::AVUTIL
    )
endif()

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
        src/engine/Converter.cpp
        src/engine/Verifier.cpp
        src/engine/Sorter.cpp
        src/engine/Worker.cpp
        src/engine/Worker.hpp
        src/plugins/PluginManager.cpp
        src/plugins/PluginManager.hpp
        src/plugins/PluginAPI.hpp
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
        src/ui/GalleryWindow.cpp
        src/ui/GalleryWindow.hpp
        src/ui/AboutWindow.cpp
        src/ui/AboutWindow.hpp
        src/ui/ChangelogWindow.cpp
        src/ui/ChangelogWindow.hpp
        src/ui/ReportWindow.cpp
        src/ui/ReportWindow.hpp
        src/ui/PluginWindow.cpp
        src/ui/PluginWindow.hpp
        src/ui/I18n.cpp
        src/ui/I18n.hpp
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
        src/engine/Converter.cpp
        src/engine/Verifier.cpp
        src/engine/Sorter.cpp
        src/engine/Worker.cpp
        src/engine/Worker.hpp
        src/plugins/PluginManager.cpp
        src/plugins/PluginManager.hpp
        src/plugins/PluginAPI.hpp
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
        src/ui/GalleryWindow.cpp
        src/ui/GalleryWindow.hpp
        src/ui/AboutWindow.cpp
        src/ui/AboutWindow.hpp
        src/ui/ChangelogWindow.cpp
        src/ui/ChangelogWindow.hpp
        src/ui/ReportWindow.cpp
        src/ui/ReportWindow.hpp
        src/ui/PluginWindow.cpp
        src/ui/PluginWindow.hpp
        src/ui/I18n.cpp
        src/ui/I18n.hpp
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
    ${PHOLIO_FFMPEG_LIBS}
    imgui::imgui
    glfw
    OpenGL::GL
    implot::implot
    nfd::nfd
    nlohmann_json::nlohmann_json
    xxHash::xxhash
    ${CMAKE_DL_LIBS}
)

if(APPLE)
    target_link_libraries(Pholio PRIVATE
        "-framework CoreFoundation"
        "-framework CoreGraphics"
        "-framework ImageIO"
    )
endif()

# stb is header-only
target_include_directories(Pholio PRIVATE ${Stb_INCLUDE_DIR})

# Add include directories
target_include_directories(Pholio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)

if(NOT WIN32)
    target_include_directories(Pholio PRIVATE
        ${AVFORMAT_INCLUDE_DIRS}
        ${AVCODEC_INCLUDE_DIRS}
        ${AVUTIL_INCLUDE_DIRS}
    )
endif()

# Output build system status
message(STATUS "Pholio version ${PHOLIO_VERSION} build system initialized")

option(PHOLIO_BUILD_SAMPLE_PLUGINS "Build sample plugins" ON)
if(PHOLIO_BUILD_SAMPLE_PLUGINS)
    add_library(PholioPluginSkipSmall MODULE src/plugins/samples/SkipSmallFilesPlugin.cpp)
    add_library(PholioPluginDatePrefix MODULE src/plugins/samples/DatePrefixPlugin.cpp)
    add_library(PholioPluginVideosOnly MODULE src/plugins/samples/VideosOnlyPlugin.cpp)
    add_library(PholioPluginHelloWindow MODULE src/plugins/samples/HelloWindowPlugin.cpp)

    foreach(pluginTarget IN ITEMS PholioPluginSkipSmall PholioPluginDatePrefix PholioPluginVideosOnly PholioPluginHelloWindow)
        target_include_directories(${pluginTarget} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins)
        set_target_properties(${pluginTarget} PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED ON
            PREFIX ""
            OUTPUT_NAME "${pluginTarget}"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample-plugins"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sample-plugins"
        )
    endforeach()
endif()

option(PHOLIO_BUILD_TESTS "Build unit tests" ON)
if(PHOLIO_BUILD_TESTS)
    enable_testing()

    add_executable(PholioEngineTests
        tests/StructureAnalyzerTests.cpp
        src/engine/StructureAnalyzer.cpp
    )

    target_include_directories(PholioEngineTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    add_test(
        NAME pholio.engine.structure_analyzer
        COMMAND PholioEngineTests
    )
endif()

option(PHOLIO_BUILD_BENCHMARKS "Build performance benchmarks" ON)
if(PHOLIO_BUILD_BENCHMARKS)
    add_executable(PholioBenchmarks
        tests/benchmarks/StructureAnalyzerBenchmark.cpp
        src/engine/StructureAnalyzer.cpp
    )

    target_include_directories(PholioBenchmarks PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
endif()

include(GNUInstallDirs)
install(TARGETS Pholio
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set(CPACK_PACKAGE_NAME "Pholio")
set(CPACK_PACKAGE_VENDOR "Pholio")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Pholio media organizer")
set(CPACK_PACKAGE_VERSION "${PHOLIO_VERSION}")

if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
elseif(WIN32)
    set(CPACK_GENERATOR "WIX")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)
