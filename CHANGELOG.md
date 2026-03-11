# Changelog

All notable changes to this project will be documented in this file.

## [1.0.0-alpha] - 2026-03-11

### Added
- **Project Rebranding:** Officially transitioned from "VerwaltungV5" to **Pholio**.
- **Image Conversion Support:** Integrated **ImageMagick (Magick++)** for future automated image format conversion (e.g., HEIC to JPEG).
- **Crash Reporting:** Implemented a robust crash handler using `backward-cpp` that saves detailed stack traces to `~/.config/Pholio/crashes/`.
- **Image Preview:** Added a real-time preview of the photos currently being processed in the Dashboard.
- **Dedicated Error Popups:** Critical errors (like "Disk Full") now trigger a modal alert for better visibility.

### Improved
- **Version Management:** Centralized versioning in `VERSION` file, now at 1.0.0-alpha.
- **Settings Sync:** Real-time synchronization between Dashboard and Settings windows with dirty-flag protection.
- **Disk Space Metrics:** Dashboard now shows total data volume processed in GB.
- **Pre-Start Validation:** Application now verifies folder existence and disk space *before* starting the process.

## [0.6.0] - 2026-03-11
- Added robust crash handler with automatic stack trace reporting.

## [0.5.1] - 2026-03-11
- Added image preview toggle and real-time processing preview.

## [0.5.0] - 2026-03-11
- Rebranded to VerwaltungV5 and implemented dynamic versioning.

## [0.3.0] - 2026-03-11
- Initial public-ready UI with Dashboard, native dialogs, and logging.
