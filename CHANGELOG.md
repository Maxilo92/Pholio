# Changelog

All notable changes to this project will be documented in this file.

## [1.5.0] - 2026-03-13

### Added
- **Per-Plugin Activation:** Plugins can now be enabled/disabled individually and the state is persisted in settings (`disabledPlugins`).

### Changed
- **Plugin Runtime Behavior:** Disabled plugins are skipped in processing, cannot be activated from menus, and do not render plugin windows.
- **Plugin Management UI:** Added per-plugin toggle controls in `Plugins -> Manage` with immediate persistence.
- **I18n Coverage:** Localized Plugin window controls/messages for German/English and added corresponding menu status labels.
- **Versioning:** Bumped semantic version to `1.5.0`.

## [1.4.0] - 2026-03-13

### Added
- **Internationalization Core:** Added a central UI i18n module with runtime language switching (`en`/`de`) and safe English fallback for missing keys.
- **Language Preference:** Added persistent `uiLanguage` setting in app config and integrated it into settings save/load flow.

### Changed
- **Localized App Shell:** Main menus, update UI, status bar and close-during-sort prompts now render in German or English.
- **Localized Settings:** Settings window tabs, labels, tooltips, presets and plugin-related controls now support German/English UI text.
- **Roadmap Progress:** Marked the "Internationalisierung (I18n)" roadmap item as completed.
- **Versioning:** Bumped semantic version to `1.4.0`.

## [1.3.0] - 2026-03-13

### Added
- **Plugin Metadata:** Plugins can now optionally export `pholio_plugin_version` and `pholio_plugin_author`, displayed in UI menus/info.
- **Plugins Menubar Entries:** Loaded plugins now appear directly under `Plugins` in the menubar with per-plugin actions.

### Changed
- **Plugin UX:** Clicking a plugin menu entry opens its window/config if available; plugins without UI now present friendly metadata info.
- **Versioning:** Bumped semantic version to `1.3.0`.

## [1.2.0] - 2026-03-13

### Added
- **Plugin System Baseline:** Added a plugin API (`pholio_plugin_*` symbols), dynamic plugin loading from a configurable directory, and safe per-file plugin decisions (skip / target override).
- **Pipeline Hooking:** Integrated plugin execution into the sorting pipeline after target path generation and before copy/move.
- **Plugin Settings:** Added plugin enable/disable switch and plugin directory configuration in the Engine settings.

### Changed
- **Roadmap Progress:** Marked the "Plugin-System" roadmap item as completed.
- **Versioning:** Bumped semantic version to `1.2.0`.

## [1.1.0] - 2026-03-13

### Added
- **Structure Migration Modes:** Added full "Umbau / Merge / Weiterführen" behavior with existing-target pattern detection and optional restructuring of already archived files.
- **Format Conversion Pipeline:** Added on-the-fly conversion options (image/video) with configurable output formats and safe fallback behavior when conversion tools are unavailable.
- **Custom Filename Templates:** Added user-defined filename templates with placeholders (e.g. date/time, original name, extension) and filename sanitization.

### Changed
- **Roadmap Progress:** Marked completed roadmap entries for structure migration, format conversion, duplicate comparison, and custom templates.
- **Versioning:** Bumped application version to stable semantic version `1.1.0`.

## [1.0.30] - 2026-03-13

### Added
- **Roadmap:** Created ROADMAP.md to track project milestones and future development phases.

### Changed
- **Release Status:** Switched from alpha to stable release version `1.0.30`.
- **Duplicate Handling UX:** Added side-by-side duplicate comparison with explicit actions and an "apply to all remaining duplicates" option.
- **Shutdown Safety:** Closing during sorting now pauses processing until the user chooses to continue or stop-and-exit safely.
- **Update Notifications:** Update checks now only notify when a release is actually newer than the installed version.

## [1.0.29-alpha] - 2026-03-13

### Fixed
- **Restart Reliability:** Hardened normal restart handoff so the current app only closes after the new instance confirms startup.
- **Update Relaunch Stability:** Added explicit startup acknowledgment and fallback behavior for update/restart relaunch on macOS.

## [1.0.18-alpha] - 2026-03-12

### Added
- **Detailed Processing Summary:** The background worker now provides a comprehensive summary including total data size, processing time, and average speed (fps and MB/s).

## [1.0.17-alpha] - 2026-03-12

### Added
- **Direct Update Installation:** Added ability to download and install updates directly from the app.
- **Safety Measures:** Update installation is blocked while the background worker is active to prevent data corruption.
- **Flexible Restart:** Choose between "Update Now" (immediate restart) and "Update at Restart".
- **macOS App Bundle Support:** Robust update script for macOS to safely replace the .app bundle.
- **Internal Debugging:** Added a new "Internal Debug" window under the View menu.
- **API Monitoring:** Real-time logging of GitHub API calls with response inspection.
- **Performance Graph:** Live FPS and frame-time tracking with visual graph.
- **Manual Update Check:** Added "Check for Updates" button in Help and About windows.
- **Enhanced Update Logic:** Now detects pre-releases and alpha versions correctly.

## [1.0.16-alpha] - 2026-03-12

### Added
- **Summary Popup:** Added a modal dialog that shows the final count of successful and failed files.
- **Stability Protection:** Implemented a protective delay during startup to prevent immediate closure on macOS.

## [1.0.15-alpha] - 2026-03-12

### Changed
- **Logging precision:** Millisecond resolution added to log timestamps.
- **Reporting Accuracy:** Improved worker metrics to distinguish between successful and failed operations.

## [1.0.14-alpha] - 2026-03-12

### Changed
- **Storage Warning:** Added background calculation of source directory size.
- **Capacity Check:** Automatic warning if target drive space is insufficient for source files.

## [1.0.13-alpha] - 2026-03-12

### Added
- **Disk Space Indicator:** Visual progress bar in settings showing drive usage.
- **Update Check:** Automatic check for new versions on GitHub during startup.

## [1.0.12-alpha] - 2026-03-12

### Changed
- **Preview System:** Image preview moved to a dedicated, dockable window.
- **UI Menu:** Added "Image Preview" toggle to the View menu.

## [1.0.11-alpha] - 2026-03-12

### Fixed
- **Stability:** Fixed crash caused by outdated settings files.
- **macOS Compatibility:** Improved window focus and event handling.

## [1.0.10-alpha] - 2026-03-12

### Fixed
- **Report System:** Fixed JSON serialization for report types.
- **Local Storage:** Reports are now stored in a local folder within the project.

## [1.0.9-alpha] - 2026-03-12

### Changed
- **Logging Path:** Logs are now stored in a local `logs/` directory.

## [1.0.8-alpha] - 2026-03-12

### Changed
- **Build System:** Switched to Ninja generator and enabled multi-core parallel builds.
- **Compiler Cache:** Integrated ccache for faster recompilation.
- **Optimizations:** Enabled Interprocedural Optimization (LTO) for release builds.
