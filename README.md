# Pholio C++ (Development)

[![Build Status](https://img.shields.io/github/actions/workflow/status/Maxilo92/Pholio/build.yml?branch=development)](https://github.com/Maxilo92/Pholio/actions)
[![Latest Dev Version](https://img.shields.io/github/v/tag/Maxilo92/Pholio?label=dev-version)](https://github.com/Maxilo92/Pholio/tree/development)

This is the **development branch** for Pholio C++. Here you will find the latest features, experimental updates, and the full source code for building.

> [!IMPORTANT]
> This branch may be unstable. For regular use, please use the [Release Branch](https://github.com/Maxilo92/Pholio/tree/release) or download the latest binary from [Releases](https://github.com/Maxilo92/Pholio/releases).

## Tech Stack
- **Language:** C++20
- **GUI:** Dear ImGui (Docking), GLFW, OpenGL3
- **Dependencies:** vcpkg (exiv2, nlohmann-json, etc.)
- **Build System:** CMake + Ninja/Make

## Building from Source

### Prerequisites
- **CMake** (3.20+)
- **vcpkg** (C++ package manager)
- **C++20 Compiler** (Clang 13+, GCC 11+, or MSVC 2022+)

### Instructions
1. **Clone & Setup:**
   ```bash
   git clone -b development https://github.com/Maxilo92/Pholio.git
   cd Pholio
   ```
2. **Build and run with included launcher:**
   ```bash
   ./run.sh
   ```

## Plugin System (Baseline)

Pholio can load runtime plugins from the configured plugin directory (`Settings -> Engine`).

- macOS: `.dylib`
- Linux: `.so`
- Windows: `.dll`

Each plugin must export:

- `int pholio_plugin_api_version();` (must return `2`)
- `const char* pholio_plugin_name();`
- `const char* pholio_plugin_version();` (optional, shown in plugin menu/info)
- `const char* pholio_plugin_author();` (optional, shown in plugin menu/info)
- `bool pholio_plugin_process(const PholioPluginTask*, PholioPluginDecision*);` (optional if UI-only plugin)
- `void pholio_plugin_render_window(const PholioPluginUiApi*, bool* open);` (optional, for custom plugin windows)

UI plugins now render through host-provided C callbacks (`PholioPluginUiApi`), so they no longer link against ImGui directly.

The C ABI structs are defined in `src/plugins/PluginAPI.hpp`.

Plugin workflow in-app:

- Configure plugin settings in `Settings -> Plugins`
- Use `Plugins` window to search/add/manage plugins
- Open plugin-specific windows/config from `Plugins` (menubar). Plugins without UI show metadata info there.
- Trigger immediate reload via `Reload Plugins` (window) or `Reload Plugins Now` (settings)

### Sample Plugins

This repository includes sample plugins under `src/plugins/samples/`:

- `SkipSmallFilesPlugin` (skips files smaller than 1MB)
- `DatePrefixPlugin` (prepends `YYYY-MM-DD/` to target path)
- `VideosOnlyPlugin` (skips non-video files)
- `HelloWindowPlugin` (adds an example plugin-rendered ImGui window)

When building, sample binaries are generated in `build/sample-plugins/` (if `PHOLIO_BUILD_SAMPLE_PLUGINS=ON`).

## Contributing
1. Fork the repo.
2. Create your feature branch (`git checkout -b feature/amazing-feature`).
3. Commit your changes (`git commit -m 'Add amazing feature'`).
4. Push to the branch (`git push origin feature/amazing-feature`).
5. Open a Pull Request against the `development` branch.

## License
Distributed under the MIT License. See `LICENSE` for more information.
