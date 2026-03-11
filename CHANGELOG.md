# Changelog

All notable changes to this project will be documented in this file.

## [0.5.0] - 2026-03-11

### Added
- Rebranded the application to **VerwaltungV5**.
- Implemented **Dynamic Versioning**: Version is now centrally managed in `VERSION` file and propagated via CMake-generated `core/Config.hpp`.
- Unified project name and version across all UI elements, console output, and window titles.
- Platform-appropriate configuration paths now use the dynamic project name.

### Improved
- Cleaned up include paths and build system structure.
- Enhanced `AboutWindow` to dynamically display project metadata.

## [0.3.0] - 2026-03-11
...
### Added
- **Restart & Rebuild Controls**: Added "Restart" (Ctrl+R) and "Rebuild & Restart" (Ctrl+Shift+R) buttons to the File menu.
- **Run Wrapper**: New `run.sh` script that manages the application lifecycle, enabling automatic rebuilding and restarting based on exit codes.

## [0.4.1] - 2026-03-11

### Added
- **Persistent Window Layout**: The application now remembers your custom window arrangements and sizes across restarts (saved in `imgui.ini`).
- **Initial Layout Logic**: The default layout is now only applied on the very first start or when manually triggered via "Reset Layout".

## [0.4.0] - 2026-03-11

### Added
- **Modern Dark Style**: Implemented a professional, darker theme with rounded corners and consistent spacing for a high-quality feel.
- **Smart Status Bar**: New status bar at the bottom displaying version, status (Idle/Running), processing speed, and current operation mode (Copy/Move).
- **Advanced Log Filtering**: Added checkboxes to filter logs by level (Info, Warning, Error, Success) and a text search field.
- **Improved Docking Layout**: Automatic sensible window arrangement on first launch or via "Reset Layout".
- **Table-based UI Layouts**: Folder selection and settings are now organized in clean, scalable tables for better alignment and readability.

### Improved
- **Dashboard UI**: Larger, color-coded Start/Stop buttons and better progress visualization with detailed status columns.
- **Settings UI**: Grouped settings into logical categories ("Directories", "Engine Behavior") for better focus.
- **Responsive Layout**: Ensured all UI elements scale sensibly when windows are resized within the dockspace.

## [0.3.0] - 2026-03-11

### Added
- New **DashboardWindow** as a central hub for starting/stopping processes and selecting folders.
- Native folder selection dialogs using **nfd-extended** (Finder on macOS, Explorer on Windows).
- **Persistent File Logging**: Logs are now saved to timestamped files in the configuration directory.
- **AboutWindow** with version, dependency, and license information.
- "Open in Finder/Explorer" context menus for folder paths.
- "Copy Message" context menu for log entries.
- Informative tooltips for all configuration options in Settings.
- "Reset Layout" option in the View menu to restore default window docking.

### Improved
- Expanded Menu Bar with File, View, and Help menus.
- Better window management and visibility toggles.
- Dashboard now displays progress, speed, and estimated time remaining (ETA).

## [0.2.0] - 2026-03-11
...
### Added
- Complete PhotoSorter Engine (Scanner, MediaAnalyzer, Sorter, Verifier, StructureAnalyzer, Worker).
- Modern ImGui Docking UI with Settings, Progress, and Log windows.
- Performance monitoring with ImPlot visualization.
- Multi-level checksum verification (None, Size, Partial, Full).
- RAW sidecar file support (.xmp).
- Thread-safe configuration management with JSON persistence.
- Cross-platform build system support (Linux/macOS).

### Fixed
- Thread-safety issues in LogWindow and StructureAnalyzer (using localtime_r/localtime_s).
- Race conditions in ConfigManager settings access via mutex protection.
- UI settings synchronization with background worker.

## [0.1.0] - 2026-03-11
...
