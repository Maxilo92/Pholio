# Changelog

All notable changes to this project will be documented in this file.

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
