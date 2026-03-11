# PhotoSorter C++

**PhotoSorter C++** is a high-performance, desktop-class media organization tool designed for advanced users and photographers on Linux and macOS. It automates the process of organizing large collections of photos and videos into a consistent, date-based hierarchy while ensuring absolute data integrity through its strict "No-Loss Policy."

## Key Features

- **No-Loss Policy:** Zero-tolerance for data loss. Source files are only deleted (in Move mode) after a successful checksum verification of the target file.
- **Robust Metadata Analysis:** Reliable extraction of image (EXIF) and video metadata using industry-standard libraries (`Exiv2`, `FFmpeg`).
- **Date-Based Organization:** Automatically sorts media into structured hierarchies (e.g., `Year/Month/Day/`).
- **Three Operation Modes:**
    - **Copy:** Preserves source files and creates organized copies in the target directory.
    - **Move:** Safely migrates files to the target directory, deleting source files only after verified transfer.
    - **Dry Run:** Simulates the entire process without performing any file operations.
- **Multi-Level Verification:** Configurable integrity checks:
    - **Full:** Complete checksum comparison (using XXHash).
    - **Partial:** Verifies file size and the first 1MB of data.
    - **Size Only:** Basic verification of file size.
- **Sidecar Support:** Automatically detects and pairs RAW images with their corresponding sidecar files (e.g., `.xmp`, `.json`).
- **Modern UI:** Responsive, docking-based interface using `Dear ImGui` and `ImPlot` for real-time progress metrics and live logging.

## Tech Stack

- **Language:** C++20
- **GUI:** Dear ImGui (Docking branch), Glfw + OpenGL3
- **Visualization:** ImPlot
- **Metadata:** Exiv2 (Images), FFmpeg (Video)
- **Dependency Management:** vcpkg
- **Build System:** CMake

## Getting Started

### Prerequisites

- **CMake** (3.20 or higher)
- **vcpkg** (C++ package manager)
- **C++20 compatible compiler** (GCC 11+, Clang 13+, or MSVC 2022+)
- **OpenGL** development libraries

### Building the Project

1. **Clone the repository:**
   ```bash
   git clone <repository-url>
   cd "Verwaltung V5"
   ```

2. **Install dependencies via vcpkg:**
   ```bash
   vcpkg install
   ```

3. **Configure and build using CMake:**
   ```bash
   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
   cmake --build build
   ```

4. **Run the application:**
   ```bash
   ./build/PhotoSorter
   ```

## Usage

1. **Source & Target:** Select your source directory containing disorganized media and your desired target directory for the organized archive.
2. **Configuration:** Choose between **Copy** or **Move** mode and select your preferred **Verification Level**.
3. **Execution:** Click **Start** to begin the process. Monitor real-time progress, throughput (MB/s), and detailed logs in the application windows.
4. **Completion:** Review the summary window once the process finishes to ensure all files were successfully organized.

## Documentation

Detailed API and architectural documentation can be found in the `docs/` directory.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
