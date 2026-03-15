# Changelog

All notable changes to this project will be documented in this file.

## [1.12.12] - 2026-03-15

### Fixed
- **Release Packaging CI (macOS/Linux):** Fehlende FFmpeg/PkgConfig-Abhaengigkeiten in den Release-Jobs werden jetzt vor dem Configure-Schritt installiert, damit `libavformat` in CMake korrekt gefunden wird.
  - macOS: `brew install pkg-config ffmpeg`
  - Linux: `apt install pkg-config libavformat-dev libavcodec-dev libavutil-dev`

### Changed
- **Versioning:** Bumped semantic version to `1.12.12`.

## [1.12.11] - 2026-03-15

### Fixed
- **Galerie-Vorschau Stabilitaet (macOS):** Decoder-Reihenfolge verbessert. Wenn `stb_image` fehlschlaegt, wird jetzt zuerst der native ImageIO-Decoder verwendet (damit auch PNG/HEIC robust funktionieren), danach Exiv2-Preview als weiterer Fallback.
- **Diagnose:** Bei fehlgeschlagener Vorschau wird der `stb_image`-Fehlergrund ins stderr geloggt.

### Changed
- **Versioning:** Bumped semantic version to `1.12.11`.

## [1.12.10] - 2026-03-15

### Fixed
- **Galerie-Vorschau fuer RAW/HEIC:** Wenn `stb_image` Dateien wie `DNG`, `CR2` oder `HEIC` nicht direkt dekodieren kann, nutzt die Vorschau jetzt einen Fallback:
  - Exiv2 eingebettete Preview (RAW-Dateien),
  - unter macOS zusaetzlich native ImageIO-Dekodierung (u. a. HEIC).
- **macOS Linking:** `CoreFoundation`, `CoreGraphics` und `ImageIO` werden fuer den App-Target verlinkt, damit der HEIC-Fallback verlässlich funktioniert.

### Changed
- **Versioning:** Bumped semantic version to `1.12.10`.

## [1.12.8] - 2026-03-15

### Added
- **Galerie/Scanner Bildformate:** Erweiterte Unterstuetzung fuer weitere RAW-Bildformate in der Bild-Erkennung (u. a. `CR3`, `NRW`, `SR2`, `SRW`, `ORF`, `RW2`, `RAF`, `PEF`, `ERF`, `3FR`, `IIQ`, `KDC`, `DCR`, `MRW`, `X3F`, `RAW`).

### Changed
- **Versioning:** Bumped semantic version to `1.12.8`.

## [1.12.7] - 2026-03-15

### Fixed
- **run.sh Rebuild-Flow:** Bei CMake-Generator-Konflikten (z. B. `Unix Makefiles` vs `Ninja`) setzt `run.sh` jetzt automatisch nur den Build-Cache (`CMakeCache.txt`, `CMakeFiles/`) zurueck und konfiguriert danach sauber neu, statt den Rebuild abzubrechen.

### Changed
- **Versioning:** Bumped semantic version to `1.12.7`.

## [1.12.6] - 2026-03-15

### Fixed
- **CMake/vcpkg Build-Stabilitaet (macOS):** Build-Setup erkennt jetzt automatisch das lokale vcpkg-Toolchain-Skript (`$VCPKG_ROOT` oder `$HOME/vcpkg`) sowie das repository-lokale `vcpkg_installed` und den passenden Triplet (`x64-osx`), damit fehlende Paketkonfigurationen nicht zum Configure-Abbruch fuehren.
- **PkgConfig unter vcpkg-Toolchain:** Bei Unix-Builds wird fuer FFmpeg-Aufloesung explizit ein funktionierendes System-`pkg-config` gesetzt, um fehlerhafte vcpkg-Tool-Referenzen zu vermeiden.
- **vcpkg Manifest:** `ffmpeg` wird nur noch auf Windows ueber vcpkg gezogen (`platform: windows`), wodurch macOS nicht mehr an externen ffmpeg-Port-Buildfehlern scheitert.

### Changed
- **Versioning:** Bumped semantic version to `1.12.6`.

## [1.12.5] - 2026-03-15

### Fixed
- **Move + Metadata-Merge:** Wenn Supplemental-Metadaten erfolgreich in das Zielbild gemerged wurden, wird die zugehoerige Quell-Metadatendatei im `Move`-Modus jetzt ebenfalls entfernt.
- **Status-Korrektur:** Der Status-Zusatz `(Merged)` wird nur noch gesetzt, wenn der Merge tatsaechlich erfolgreich war.

### Changed
- **Versioning:** Bumped semantic version to `1.12.5`.

## [1.12.4] - 2026-03-14

### Changed
- **Galerie/Preview-Verhalten:** Galerie zeigt jetzt die Liste, waehrend ausgewaehlte Bilder im bestehenden `Image Preview`-Fenster gerendert werden.
- **UX-Hinweise:** Galerie zeigt klaren Hinweis auf das Ziel-Fenster und bietet einen Button zum Oeffnen der Bildvorschau, falls sie geschlossen ist.
- **Versioning:** Bumped semantic version to `1.12.4`.

## [1.12.3] - 2026-03-14

### Added
- **Galerie-Kontextmenü:** Rechtsklick auf Galerie-Einträge öffnet ein Kontextmenü mit Aktionen für Datei öffnen, Ordner öffnen und Pfad kopieren.

### Changed
- **Versioning:** Bumped semantic version to `1.12.3`.

## [1.12.2] - 2026-03-14

### Added
- **Galerie-Fenster:** Neues `Gallery`-Fenster zum rekursiven Anzeigen aller Bilder eines gewählten Ordners inklusive beliebig tiefer Unterordner.

### Changed
- **UI Integration:** Galerie in `View`-Menü, Docking-Layouts und persistente Fenstersichtbarkeit (`showGallery`) integriert.
- **Versioning:** Bumped semantic version to `1.12.2`.

## [1.12.0] - 2026-03-13

### Changed
- **Phase Completion:** Closed Phase 3 by marking all Release Candidate objectives as completed and moving active status to Phase 4 in `ROADMAP.md`.
- **Versioning:** Bumped semantic version to `1.12.0`.

## [1.11.0] - 2026-03-13

### Added
- **Packaging Workflow:** Added `.github/workflows/release-packaging.yml` to build and package release artifacts for macOS, Windows, and Linux.
- **CPack Integration:** Added install/CPack configuration to CMake so CI can generate `DMG` (macOS), `MSI` (Windows), and Linux `TGZ` artifacts.
- **Packaging Documentation:** Added `docs/PACKAGING_PIPELINE.md` with triggers, artifact outputs, and pipeline structure.

### Changed
- **Roadmap Progress:** Marked the RC-track item "Installer-Pipeline vervollständigen" as completed.
- **Versioning:** Bumped semantic version to `1.11.0`.

## [1.10.0] - 2026-03-13

### Added
- **Signing CI Placeholder:** Added `.github/workflows/release-signing-placeholder.yml` with dedicated macOS and Windows signing placeholder jobs.
- **Secret Validation:** Added strict CI checks for required signing/notarization secrets before placeholder signing steps run.
- **Signing Documentation:** Added `docs/SIGNING_WORKFLOW.md` documenting required secrets and next implementation steps.

### Changed
- **Roadmap Progress:** Marked the Phase 3 item "Signing-Workflow vorbereiten" as completed.
- **Versioning:** Bumped semantic version to `1.10.0`.

## [1.9.0] - 2026-03-13

### Added
- **Unit Test Harness:** Added CTest integration and a dedicated test binary (`PholioEngineTests`) in CMake.
- **Core Unit Tests:** Added initial tests for `StructureAnalyzer::generatePath` (`tests/StructureAnalyzerTests.cpp`).
- **Performance Benchmarking:** Added benchmark binary (`PholioBenchmarks`) for reproducible `StructureAnalyzer.generatePath` throughput measurements.
- **Benchmark Documentation:** Added `docs/PERFORMANCE_BENCHMARKS.md` with dataset definition, metrics, execution commands, baseline, and RC target values.

### Changed
- **Roadmap Progress:** Checked off the Phase 3 RC item "Performance-Benchmarks definieren" and documented active Phase-3 progress.
- **Versioning:** Bumped semantic version to `1.9.0`.

## [1.8.0] - 2026-03-13

### Added
- **Windows CI Validation:** Added GitHub Actions workflow (`windows-msvc.yml`) to configure and build on `windows-latest` with MSVC.

### Changed
- **Windows Build Porting:** Updated CMake FFmpeg resolution to support Windows/MSVC (vcpkg config targets) while keeping pkg-config flow for Unix-like systems.
- **Dependency Manifest:** Added `ffmpeg` to `vcpkg.json` for reproducible cross-platform builds.
- **Roadmap Progress:** Marked the "Windows Support" roadmap item as completed.
- **Versioning:** Bumped semantic version to `1.8.0`.

## [1.7.0] - 2026-03-13

### Added
- **Diagnostics Export:** Added automated diagnostics bundle export (logs, crash reports, settings, metadata) via `ReportManager::exportDiagnosticsBundle`.
- **Report UI Actions:** Added `EXPORT DIAGNOSTICS` and `Open Reports Folder` actions to the report window.

### Changed
- **Report Localization:** Localized report window fields/messages for German/English.
- **Roadmap Progress:** Marked the "Fehler-Reporting" roadmap item as completed.
- **Versioning:** Bumped semantic version to `1.7.0`.

## [1.6.0] - 2026-03-13

### Added
- **Layout Presets:** Added multiple dock layout presets (Default, Media Focus, Monitoring Focus) in `View`.
- **Icon Labels:** Added clearer icon-style labels for key view/plugin navigation entries.

### Changed
- **UI/UX Milestone:** Completed and checked off the roadmap item "UI/UX Refinement".
- **Versioning:** Bumped semantic version to `1.6.0`.

## [1.5.1] - 2026-03-13

### Added
- **Theme Setting:** Added persisted `uiTheme` setting (`dark`/`light`) to app configuration.

### Changed
- **Live Theme Application:** App style now updates based on the configured theme at runtime.
- **Settings UX:** Added localized theme selector (Dark/Light) in settings.
- **Roadmap Progress:** Advanced the UI/UX refinement milestone by implementing Dark/Light mode support.
- **Versioning:** Bumped semantic version to `1.5.1`.

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
