# Pholio C++ API Documentation

This document provides a technical overview of the core components and interfaces of the Pholio application.

## Namespace: `engine`

The `engine` namespace contains the core processing logic for scanning, analyzing, and sorting media files.

### Class: `Worker`
The `Worker` class manages the background execution thread for media processing tasks. It handles communication between the engine and the UI.

- **`start()`**: Initializes and starts the background worker thread.
- **`stop()`**: Signals the worker thread to stop gracefully.
- **`isRunning()`**: Returns `true` if the worker thread is currently active.
- **`getProgress()`**: Returns a float representing the overall completion percentage.
- **`getStatusMessage()`**: Returns a string describing the current operation (e.g., "Scanning", "Processing...").
- **`getFilesPerSecond()` / `getBytesPerSecond()`**: Provides real-time performance metrics.

### Class: `Scanner`
Responsible for recursively identifying media files and their corresponding sidecar files in a source directory.

- **`scan()`**: Performs the recursive scan and returns a vector of `MediaTask` objects.
- **`isImage(path)` / `isVideo(path)`**: Static helper methods for file type identification based on extensions.

### Class: `MediaAnalyzer`
Handles the extraction of metadata from images and videos.

- **`analyze(metadata)`**: Extracts date-time and format information using `Exiv2` for images and `FFmpeg` for videos.

### Class: `Sorter`
Executes file operations (copy/move) and manages the organization hierarchy.

- **`process(task, mode)`**: Orchestrates the organization of a single `MediaTask`. This includes path generation, file transfer, and verification.

### Class: `Verifier`
Ensures data integrity during and after file operations.

- **`calculateHash(path, level)`**: Calculates a hash or identifier for a file based on the requested `VerificationLevel`.
- **`verify(source, destination, level)`**: Compares source and destination files to ensure they are identical.

---

## Namespace: `core`

The `core` namespace handles application-wide services such as configuration and state management.

### Class: `ConfigManager`
A singleton class that manages the loading, saving, and access of application settings.

- **`getInstance()`**: Returns the singleton instance of the `ConfigManager`.
- **`load()` / `save()`**: Persists application settings to the filesystem in JSON format.
- **`getSettings()`**: Provides access to the `AppSettings` struct containing user preferences.

---

## Namespace: `ui`

The `ui` namespace contains the ImGui-based frontend components.

### Class: `AppWindow`
The main application entry point for the UI. It orchestrates the rendering of all sub-windows and manages the integration with the `engine::Worker`.

- **`update()`**: Updates the state of all UI components.
- **`render()`**: Executes the ImGui rendering commands for the main dockspace and child windows.

### UI Components
- **`SettingsWindow`**: Interface for configuring paths, operation modes, and verification levels.
- **`ProgressWindow`**: Displays real-time metrics, progress bars, and throughput graphs using `ImPlot`.
- **`LogWindow`**: A thread-safe interface for viewing real-time operation logs.
