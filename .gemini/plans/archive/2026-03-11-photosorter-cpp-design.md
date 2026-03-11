# Design-Dokument: PhotoSorter C++

## 1. Problem Statement
PhotoSorter C++ is designed to solve the common problem of disorganized digital media archives. Modern cameras and smartphones generate thousands of files with disparate naming conventions and deep, inconsistent folder structures. This application automates the process of organizing these files into a consistent, date-based hierarchy (e.g., `Year/Month/Day/`) while ensuring absolute data integrity through its "No-Loss Policy."

The application specifically targets advanced users and photographers on Linux and macOS, providing a desktop-class experience with real-time feedback and high-performance multithreaded scanning. By integrating industry-standard metadata and video libraries (`Exiv2`, `FFmpeg`), it ensures robust handling of all common media formats and their associated RAW sidecar files.

## 2. Requirements

### Functional Requirements
- **Metadata Analysis:** Robust extraction of image (EXIF) and video metadata using `Exiv2` and `FFmpeg` (libavformat/libavcodec).
- **Automated Sorting:** Flexible date-based directory structure generation (e.g., `Year/Month/Day/`).
- **Migration Logic:** Support for three distinct modes when encountering existing archives:
    1. **Umbau:** Full reorganization of target directory.
    2. **Merge:** Insertion of new files into existing structures.
    3. **Continue:** Adopt and extend the detected legacy structure.
- **Sidecar Handling:** Automatic pairing of RAW images with their corresponding sidecar files (e.g., `.xmp`).
- **Duplication Management:** Configurable behavior for duplicate files (skip, rename, replace).
- **Checksum Verification:** Three levels of post-copy verification (Full XXHash, Partial Hash, Size Only), configurable in settings.

### Non-Functional Requirements
- **No-Loss Policy:** Zero-tolerance for data loss during move/copy operations.
- **Performance:** Multithreaded scanning and I/O for high-throughput processing.
- **Observability:** Real-time log buffer, progress bars, and performance metrics (MB/s, files/s) using `ImPlot`.
- **Portability:** Primarily targeting Linux/macOS with `AppImage` and `DMG` packaging.
- **Build System:** CMake-based project utilizing `vcpkg` for dependency management.

### Constraints
- Use of C++20 for modern features and safety.
- Dependency on specific industry libraries (`Exiv2`, `FFmpeg`, `stb_image`, `ImGui`, `ImPlot`).

## 3. Approach

### Selected Architecture: Threaded Task Engine
The selected approach uses a single background thread to perform all metadata analysis, I/O operations, and verification. This background thread interacts with the UI thread via thread-safe atomic variables and a mutex-protected log buffer.

- **Background Worker:** Manages a queue of file tasks. Each task consists of a single file and its potential sidecars. The worker performs scanning, then iterates through the tasks one by one to analyze metadata, calculate target paths, perform the I/O operation (copy or move), and verify the result using the user's selected hashing mode.
- **UI Interaction:** The ImGui rendering loop runs in the main thread and polls the worker's status. It updates progress bars, ETA calculations, and the live log window in real-time. This model is efficient and avoids complex asynchronous message passing, which is ideal for a high-throughput desktop application.

### Alternatives Considered
- **Reactive Event Engine:** Decoupling the engine and UI via a message-based system was considered for flexibility. However, it was rejected for this project due to the added complexity and overhead. Given that the core operation is a linear, high-throughput I/O task, the threaded task model is simpler to implement and more performant.
- **Multithreaded I/O:** We considered using multiple background threads for I/O operations. This was rejected because, while potentially faster for SSDs, it significantly increases the risk of file system contention and potential data corruption. A single background thread for I/O ensures atomic-like behavior and simplifies the implementation of the "No-Loss Policy."

## 4. Architecture

### Core Components
1. **Engine (Worker Thread):**
    - **Scanner:** Recursively scans source and target directories, building a list of files and their potential sidecar pairs.
    - **MediaAnalyzer:** Extracts metadata using `Exiv2` (images) and `libavformat`/`libavcodec` (videos). Provides a fallback mechanism if metadata is missing (e.g., file modification date).
    - **StructureAnalyzer:** Detects and analyzes existing directory structures in the target folder to determine the best migration strategy.
    - **Verifier:** Calculates and compares file checksums (XXHash, MD5, or Size-only) before and after file operations.
    - **Sorter:** Executes atomic-like copy/move operations based on the results from the `MediaAnalyzer` and `Verifier`.
2. **Frontend (Main Thread):**
    - **ImGui Interface:** Modern docking-based UI with windows for settings, progress, live logging, and final summaries.
    - **Live-Log Window:** A thread-safe, mutex-protected buffer for real-time log messages.
    - **Progress & Metrics:** Real-time visualization using `ImPlot` for throughput (MB/s) and file counts.
    - **Preview:** Displays the currently processed image or a video frame using `stb_image` and `FFmpeg`.
3. **Shared State:**
    - **`ConfigManager`:** Manages persistent application settings in JSON format.
    - **Atomic Progress Variables:** `std::atomic<uint64_t>` for tracking file counts and byte progress.
    - **Safe-Cancel Signal:** An atomic boolean used to signal the worker thread to stop gracefully.

### Data Flow
Source files are scanned and stored in a shared task queue. The `MediaAnalyzer` determines the target path for each file. The `Sorter` performs the file operation, followed by the `Verifier`'s integrity check. The results of each step are recorded in the shared log buffer, and atomic progress counters are updated. The main thread's ImGui loop polls these variables to update the UI every frame.

## 5. Agent Team
1. **Projektleitung (Architect):** Responsible for the overall system architecture, project management, and high-level design decisions. Ensures the consistency of the design and the adherence to the "No-Loss Policy."
2. **Kern-Entwickler (Coder):** The primary implementation agent for the engine and business logic. Handles metadata analysis, directory structure detection, and multithreaded file operations.
3. **Integrations-Lead (API Designer):** Responsible for defining clean interfaces between the engine and the UI. Manages the `ConfigManager` and external library integrations (Exiv2, FFmpeg).
4. **Qualitätssicherung (Tester):** Focuses on unit and integration testing. Responsible for verifying the `Verifier` module and ensuring the "No-Loss Policy" is empirically tested.
5. **Performance-Experte (Performance Engineer):** Optimizes multithreaded scanning and I/O operations. Responsible for real-time metrics and ImPlot visualization.
6. **Sicherheits-Experte (Security Engineer):** Audits the file operation logic for safety and potential data corruption risks. Ensures atomic-like operations and robust error handling.
7. **DevOps-Ingenieur (DevOps Engineer):** Manages the CMake build system, vcpkg dependencies, and the packaging for Linux (AppImage) and macOS (DMG).
8. **Technischer-Autor (Technical Writer):** Documents the codebase, provides user documentation, and ensures clear API descriptions.

## 6. Risk Assessment & Mitigation
- **Data Loss During Operations:** Risk of file corruption or deletion during I/O operations. Mitigation: Strict "Copy-Verify-Delete" sequence, where the source file is only deleted (if move mode is selected) *after* a successful checksum verification of the target file.
- **Library Compatibility:** Risk of issues with `Exiv2` or `FFmpeg` on Linux/macOS. Mitigation: Careful testing on both platforms and using `vcpkg` for consistent builds.
- **Multithreading Contention:** Potential for race conditions between the UI and worker thread. Mitigation: Use of `std::atomic` for counters and a robust `std::mutex` strategy for log and data buffers.
- **Resource Exhaustion:** Large media collections could exhaust system resources (RAM, CPU, I/O bandwidth). Mitigation: Throttled file scanning and optimized I/O using buffer management and asynchronous file operations where appropriate.
- **Disk Full/Error Handling:** Sudden I/O failure during operation. Mitigation: Robust exception handling and atomic-like state transitions that allow for a safe resume or graceful stop.
- **Unsupported Metadata:** Risk of files with non-standard EXIF or video metadata. Mitigation: Fallback mechanism to file system dates or filename-based analysis as a last resort.

## 7. Success Criteria
- **Functional Completeness:** The application can successfully scan, analyze, and sort images and videos based on metadata into a date-based directory structure.
- **Robustness:** The "No-Loss Policy" is strictly enforced, with no data lost during move or copy operations across various file types and sizes.
- **Integrity Verification:** All three checksum verification modes (Full, Partial, Size-only) are correctly implemented and can be selected by the user.
- **Cross-Platform Compatibility:** The application builds and runs as expected on Linux and macOS, with functional `AppImage` and `DMG` packages.
- **User Interface Responsiveness:** The ImGui interface remains responsive during long-running background tasks, with accurate progress bars, live logging, and real-time metrics.
- **Error Resilience:** The application can handle standard error cases (disk full, file system permissions, invalid metadata) without crashing.
- **Performance:** Scanning and sorting operations are multithreaded and efficient, meeting performance expectations for large media collections.
- **Documentation Quality:** Clear and complete technical and user documentation exists for all core modules.
