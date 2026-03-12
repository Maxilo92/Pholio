#!/usr/bin/env python3
import os
import sys
import subprocess
import shutil
import argparse
from pathlib import Path

def run_command(command, cwd=None, env=None):
    print(f"Running: {' '.join(command)}")
    try:
        subprocess.check_call(command, cwd=cwd, env=env)
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error executing command: {e}")
        return False

def find_vcpkg():
    # Priority list for vcpkg locations
    vcpkg_roots = [
        os.environ.get("VCPKG_ROOT"),
        str(Path.home() / "vcpkg"),
        "/usr/local/opt/vcpkg",
        "/opt/vcpkg"
    ]
    
    for root in vcpkg_roots:
        if root and os.path.exists(Path(root) / "scripts/buildsystems/vcpkg.cmake"):
            return root
    return None

def main():
    parser = argparse.ArgumentParser(description="Build script for Pholio")
    parser.add_argument("--config", choices=["Debug", "Release"], default="Release", help="Build configuration")
    parser.add_argument("--vcpkg-root", help="Path to vcpkg installation")
    parser.add_argument("--clean", action="store_true", help="Remove build directory before building")
    parser.add_argument("--install-dir", help="Custom vcpkg installed directory (useful for paths with spaces)")
    parser.add_argument("-s", "--start", "-start", action="store_true", help="Start the application after successful build")
    
    args = parser.parse_args()

    # 1. Resolve Paths
    project_root = Path(__file__).parent.absolute()
    build_dir = project_root / "build"
    
    vcpkg_root = args.vcpkg_root or find_vcpkg()
    if not vcpkg_root:
        print("Error: vcpkg not found. Please set VCPKG_ROOT environment variable or use --vcpkg-root")
        sys.exit(1)
        
    toolchain_file = Path(vcpkg_root) / "scripts/buildsystems/vcpkg.cmake"
    print(f"Using vcpkg toolchain: {toolchain_file}")

    # 2. Clean
    if args.clean and build_dir.exists():
        print(f"Cleaning build directory: {build_dir}")
        shutil.rmtree(build_dir)

    # 3. Configure
    os.makedirs(build_dir, exist_ok=True)
    
    cmake_configure = [
        "cmake",
        "-B", str(build_dir),
        "-S", str(project_root),
        f"-DCMAKE_BUILD_TYPE={args.config}",
        f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}"
    ]
    
    # Use Ninja generator if available for faster builds
    if shutil.which("ninja"):
        cmake_configure.extend(["-G", "Ninja"])
    
    # Handle spaces in path issue if needed
    if args.install_dir:
        cmake_configure.append(f"-DVCPKG_INSTALLED_DIR={args.install_dir}")
    elif " " in str(project_root):
        # Default to a space-free temp directory if the project path contains spaces
        temp_install = Path("/tmp/vcpkg_installed_pholio")
        print(f"Warning: Project path contains spaces. Using {temp_install} for vcpkg dependencies.")
        cmake_configure.append(f"-DVCPKG_INSTALLED_DIR={temp_install}")

    if not run_command(cmake_configure):
        print("Configuration failed.")
        sys.exit(1)

    # 4. Build
    cmake_build = [
        "cmake",
        "--build", str(build_dir),
        "--config", args.config,
        "--parallel" # Uses all available CPU cores
    ]
    
    if not run_command(cmake_build):
        print("Build failed.")
        sys.exit(1)

    print("\n" + "="*40)
    print(f"Build successful! (Configuration: {args.config})")
    
    binary_path = build_dir / "Pholio"
    if sys.platform == "darwin":
        app_bundle = build_dir / "Pholio.app"
        if app_bundle.exists():
            print(f"App Bundle: {app_bundle}")
            binary_path = app_bundle / "Contents/MacOS/Pholio"
    
    print(f"Executable: {binary_path}")
    print("="*40)

    # 5. Execute if requested
    if args.start:
        print("\nStarting Pholio...")
        if sys.platform == "darwin" and (build_dir / "Pholio.app").exists():
            run_command(["open", str(build_dir / "Pholio.app")])
        else:
            run_command([str(binary_path)])

if __name__ == "__main__":
    main()
