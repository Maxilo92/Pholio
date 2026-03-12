# Changelog

All notable changes to this project will be documented in this file.

## [1.0.14-alpha] - 2026-03-12

### Changed
- **Enhanced Disk Space Check:** The storage indicator in Settings now automatically calculates the total size of the source directory in the background. It provides a specific warning if the source files will not fit on the target drive.

## [1.0.13-alpha] - 2026-03-12

### Added
- **Disk Space Warning:** Added an intelligent storage indicator in the Settings window. It shows used/total space of the target drive with color-coded alerts (Green/Yellow/Red) based on availability.
- **Auto Update Check:** Pholio now checks for new releases on GitHub at startup. A non-intrusive banner appears if a newer version is found, providing a direct link to the release page.

## [1.0.12-alpha] - 2026-03-12

### Changed
- **Image Preview:** The real-time image preview has been moved from the Dashboard into its own dedicated "Image Preview" window.
- **UI:** Added a new "Image Preview" toggle to the View menu.
- **Docking:** The new Image Preview window is now integrated into the default workspace layout.

## [1.0.11-alpha] - 2026-03-12

### Fixed
- **Stability:** Fixed a crash/immediate exit caused by outdated `settings.json` files.
- **Robustness:** Configuration loading now gracefully handles missing or corrupted keys.
- **macOS Compatibility:** Improved window focus and event handling on macOS.
- **UI:** Deactivated ImGui viewports on macOS to prevent window management conflicts.

## [1.0.10-alpha] - 2026-03-12

### Added
- **Duplicate Handling Settings:** Added options to the settings window for deciding what to do when a file already exists (Skip, Overwrite, Rename).
- **Duplicate Logic:** The sorter engine now supports automatic renaming (e.g., `image (1).jpg`) and skipping of duplicates based on user preferences.
- **Ask on Duplicate:** Added a toggle to (eventually) prompt for each duplicate, currently marking them for manual action in the logs.

## [1.0.10-alpha] - 2026-03-12

### Fixed
- **Report Saving:** Fixed a critical bug where reports could not be saved because of missing JSON serialization for the report type enum.
- **Report Path:** Reports are now stored in a local `reports/` directory within the project root.

## [1.0.9-alpha] - 2026-03-12

### Changed
- **Logging Path:** Log files are now stored in a local `logs/` directory within the project root instead of the global configuration directory.

## [1.0.8-alpha] - 2026-03-12

### Changed
- **Build Process Optimized:**
  - Added `--parallel` to the `build.py` script to enable multi-core compilation.
  - Added automatic detection and usage of the `Ninja` generator if available.
  - Integrated `ccache` support into `CMakeLists.txt` for significantly faster recompilation.
  - Enabled Link-Time Optimization (IPO/LTO) for `Release` builds.

## [1.0.7-alpha] - 2026-03-11

### Added
- **macOS Bundle Support:** Pholio is now built as a proper macOS `.app` bundle with its own `Info.plist` and standard application structure.
- **User Reporting System:** Added a new reporting window (accessible via Help menu) to submit bug reports and feature requests. Reports are stored locally as JSON files.
- **Google Photos Support:** Automatically detects `.supplemental-metadata.json` files from Google Takeout and merges their photo taken time and GPS coordinates into the image files.

## [1.0.6-alpha] - 2026-03-11

### Fixed
- **Versioning:** Synchronized version numbers between the macOS native "About" dialog and the internal "About" window.
- **Layout Persistence:** Fixed window layout not being saved across sessions by moving `imgui.ini` to the persistent config directory.

## [1.0.5-alpha] - 2026-03-11

### Fixed
- **UI:** Implemented the previously non-functional "Exit" menu item to correctly close the application.

## [1.0.4-alpha] - 2026-03-11

### Added
- **Window State Persistence:** The application now remembers which windows (Dashboard, Settings, Logs, etc.) were open in the last session and restores them on startup.

## [1.0.3-alpha] - 2026-03-11

### Fixed
- **Stability:** Fixed a critical `SIGABRT` crash that occurred when starting the worker process multiple times without a proper reset.

## [1.0.2-alpha] - 2026-03-11

### Added
- **Sorting Speed Metric:** Added real-time display of data processing speed in MB/s or GB/s to the Dashboard.

## [1.0.1-alpha] - 2026-03-11

### Changed
- **Default Folder Structure:** Updated default organization pattern to `yyyy/mm-Monthname/dd/` (e.g., `2024/03-March/11/`).

## [1.0.0-alpha] - 2026-03-11

### Added
- **Initial Release:** officially rebranded to **Pholio**.
- **Build & Run Support:** Added flags to `build.py` for automatic execution.
- **Crash Reporting:** Implemented `backward-cpp` crash handler.
- **Image Preview:** Added real-time preview of processed photos.
- **Custom Folder Structure:** Added support for `strftime` patterns in settings.
- **Organization Presets:** Added quick-selection buttons for common structures.
- **Disk Space Metrics:** Dashboard shows total volume processed.
- **Pre-Start Validation:** Verifies disk space before starting.
