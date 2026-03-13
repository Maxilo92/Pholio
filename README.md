# Pholio C++ (Release)

[![Release](https://img.shields.io/github/v/release/Maxilo92/Pholio?include_prereleases)](https://github.com/Maxilo92/Pholio/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**Pholio C++** is a high-performance, desktop-class media organization tool designed for advanced users and photographers. This is the **stable branch**, providing the most reliable versions of Pholio.

## 🚀 Download Ready-to-Run
For most users, we recommend downloading the pre-compiled application:
1. Go to the [Releases](https://github.com/Maxilo92/Pholio/releases) page.
2. Download the latest `.zip` for your operating system (e.g., `Pholio-macOS-vX.X.X.zip`).
3. Extract and run.

## Key Features
- **No-Loss Policy:** Source files are only deleted (in Move mode) after successful checksum verification.
- **Intelligent Storage Warning:** Calculates source size in the background and warns if target space is insufficient.
- **Real-time Image Preview:** See processed photos in a dedicated window during organization.
- **Date-Based Organization:** Hierarchies like `Year/Month/Day/` based on EXIF/Metadata.
- **Automatic Updates:** Pholio checks for new GitHub releases on startup.

## Development
If you are looking for the latest features or want to contribute, please check the [Development Branch](https://github.com/Maxilo92/Pholio/tree/development).

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
