from invoke import task
import os
import sys
import platform
from pathlib import Path


@task
def bootstrap(c):
    """Install all dependencies for the project (OpenCV, Qt6, build tools)"""
    print("🚀 Bootstrapping project dependencies...")

    system = platform.system().lower()

    if system == "linux":
        bootstrap_linux(c)
    elif system == "darwin":  # macOS
        bootstrap_macos(c)
    elif system == "windows":
        bootstrap_windows(c)
    else:
        print(f"❌ Unsupported operating system: {system}")
        sys.exit(1)

    print("✅ Bootstrap complete! You can now run 'invoke build'")


def bootstrap_linux(c):
    """Install dependencies on Linux (Ubuntu/Debian)"""
    print("🐧 Installing dependencies on Linux...")

    # Update package list
    c.run("sudo apt update")

    # Install build tools
    c.run("sudo apt install -y build-essential cmake ninja-build pkg-config")

    # Install Qt6
    c.run("sudo apt install -y qt6-base-dev qt6-tools-dev")

    # Install OpenCV dependencies
    c.run("sudo apt install -y libjpeg-dev libpng-dev libtiff-dev")
    c.run(
        "sudo apt install -y libavcodec-dev libavformat-dev libswscale-dev libv4l-dev"
    )
    c.run("sudo apt install -y libxvidcore-dev libx264-dev")
    c.run("sudo apt install -y libgtk-3-dev")
    c.run("sudo apt install -y libatlas-base-dev gfortran")

    # Install OpenCV (both dev and static libraries)
    print("📦 Installing OpenCV with static libraries...")
    result = c.run(
        "sudo apt install -y libopencv-dev libopencv-contrib-dev", hide=True, warn=True
    )

    if result.failed:
        print("⚠️  OpenCV not available in package manager, building from source...")
        install_opencv_from_source_linux(c)
    else:
        print("✅ OpenCV installed via package manager")
        # Check if the installed version is sufficient
        if not verify_opencv_installation(c):
            print("⚠️  Installed OpenCV version is too old, building from source...")
            install_opencv_from_source_linux(c)

    # Final verification
    verify_opencv_installation(c)


def bootstrap_macos(c):
    """Install dependencies on macOS"""
    print("🍎 Installing dependencies on macOS...")

    # Check if Homebrew is installed
    result = c.run("which brew", hide=True, warn=True)
    if result.failed:
        print("📦 Installing Homebrew...")
        c.run(
            '/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"'
        )

    # Install build tools
    c.run("brew install cmake ninja")

    # Install Qt6
    c.run("brew install qt@6")

    # Install OpenCV (Homebrew OpenCV includes static libraries)
    print("📦 Installing OpenCV with static libraries...")
    c.run("brew install opencv")

    # Verify installation
    verify_opencv_installation(c)


def bootstrap_windows(c):
    """Install dependencies on Windows"""
    print("🪟 Installing dependencies on Windows...")

    # Check if vcpkg is installed
    if not os.path.exists("vcpkg"):
        print("📦 Installing vcpkg...")
        c.run("git clone https://github.com/Microsoft/vcpkg.git")
        c.run("cd vcpkg && .\\bootstrap-vcpkg.bat")
        c.run("cd vcpkg && .\\vcpkg integrate install")

    # Install OpenCV with static libraries
    print("📦 Installing OpenCV with static libraries...")
    c.run("cd vcpkg && .\\vcpkg install opencv:x64-windows-static")

    # Note: Qt6 needs to be installed manually on Windows
    print("⚠️  Please install Qt6 manually from https://www.qt.io/download")
    print("   Make sure to add Qt6 to your PATH or set Qt6_DIR in CMake")

    # Verify installation
    verify_opencv_installation(c)


def install_opencv_from_source_linux(c):
    """Build OpenCV from source on Linux"""
    print("🔨 Building OpenCV from source...")

    # Create temporary directory
    c.run("mkdir -p /tmp/opencv_build")

    with c.cd("/tmp/opencv_build"):
        # Clone OpenCV
        c.run("git clone https://github.com/opencv/opencv.git")
        c.run("cd opencv && git checkout 4.8.0")

        # Build OpenCV
        c.run("cd opencv && mkdir build && cd build")
        c.run(
            "cd opencv/build && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_SHARED_LIBS=ON -DBUILD_opencv_world=ON -DOPENCV_ENABLE_NONFREE=ON -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_opencv_python=OFF .."
        )
        c.run("cd opencv/build && make -j$(nproc)")
        c.run("cd opencv/build && sudo make install")
        c.run("sudo ldconfig")

    # Cleanup
    c.run("rm -rf /tmp/opencv_build")


def verify_opencv_installation(c):
    """Verify that OpenCV is properly installed"""
    print("🔍 Verifying OpenCV installation...")

    # Try to find OpenCV with pkg-config
    result = c.run("pkg-config --modversion opencv4", hide=True, warn=True)
    if result.failed:
        result = c.run("pkg-config --modversion opencv", hide=True, warn=True)

    if result.failed:
        print("❌ OpenCV not found by pkg-config")
        print("   Please ensure OpenCV is properly installed")
        return False

    version = result.stdout.strip()
    print(f"✅ Found OpenCV version: {version}")

    # Check if version is sufficient
    if version < "4.5.0":
        print(f"❌ OpenCV version {version} is too old. Required: 4.5.0 or higher")
        return False

    # Check for static libraries
    if not check_static_libraries(c):
        print("⚠️  Static libraries not found, but shared libraries are available")
        print("   The project will be configured to use static linking if available")

    print("✅ OpenCV installation verified successfully!")
    return True


def check_static_libraries(c):
    """Check if OpenCV static libraries are available"""
    print("🔍 Checking for OpenCV static libraries...")

    # Common static library locations
    static_lib_paths = [
        "/usr/lib/x86_64-linux-gnu/libopencv_*.a",
        "/usr/local/lib/libopencv_*.a",
        "/opt/homebrew/lib/libopencv_*.a",
    ]

    for pattern in static_lib_paths:
        result = c.run(f"ls {pattern}", hide=True, warn=True)
        if not result.failed and result.stdout.strip():
            print(f"✅ Found static libraries in: {pattern}")
            return True

    print("❌ No static libraries found")
    return False


@task
def check_opencv(c):
    """Check OpenCV installation and version"""
    print("🔍 Checking OpenCV installation...")

    if verify_opencv_installation(c):
        print("✅ OpenCV is properly installed and configured")
        check_static_libraries(c)
    else:
        print("❌ OpenCV installation check failed")
        print("   Run 'invoke bootstrap' to install dependencies")


@task
def clean(c):
    """Clean build artifacts"""
    print("🧹 Cleaning build artifacts...")
    c.run("rm -rf build/")
    c.run("rm -f CMakeCache.txt cmake_install.cmake")
    c.run("rm -rf korova/")  # Remove old build dir if it exists
    print("✅ Clean complete")


@task
def configure(c):
    """Configure the project with CMake"""
    print("⚙️  Configuring project...")

    # Create build directory
    Path("build").mkdir(exist_ok=True)

    # Set Qt6 path if needed (adjust this path to match your Qt installation)
    qt_paths = [
        f"{os.environ.get('HOME')}/Qt/6.5.0/gcc_64/lib/cmake/Qt6",
        f"{os.environ.get('HOME')}/Qt/6.9.0/gcc_64/lib/cmake/Qt6",  # Your current version
        "/usr/lib/x86_64-linux-gnu/cmake/Qt6",
        "/opt/Qt/6.5.0/gcc_64/lib/cmake/Qt6",
    ]

    qt6_dir = None
    for path in qt_paths:
        if os.path.exists(path):
            qt6_dir = path
            break

    if qt6_dir:
        print(f"📦 Found Qt6 at: {qt6_dir}")
        with c.cd("build"):
            c.run(f"cmake -DQt6_DIR={qt6_dir} ..")
    else:
        print("⚠️  Qt6 not found in common locations, trying default cmake...")
        with c.cd("build"):
            c.run("cmake ..")

    print("✅ Configure complete")


def print_binary_size(c):
    """Print the size of the built executable"""
    executable_paths = [
        "build/korova",
        "build/src/app/korova",
        "build/Debug/korova",
        "build/Release/korova",
        "build/src/app/korova.app/Contents/MacOS/korova",  # macOS .app bundle path
    ]

    executable = None
    for path in executable_paths:
        if os.path.exists(path):
            executable = path
            break

    if executable:
        # Get file size in bytes
        size_bytes = os.path.getsize(executable)

        # Convert to human readable format
        if size_bytes >= 1024 * 1024 * 1024:  # GB
            size_str = f"{size_bytes / (1024 * 1024 * 1024):.2f} GB"
        elif size_bytes >= 1024 * 1024:  # MB
            size_str = f"{size_bytes / (1024 * 1024):.2f} MB"
        elif size_bytes >= 1024:  # KB
            size_str = f"{size_bytes / 1024:.2f} KB"
        else:
            size_str = f"{size_bytes} bytes"

        print(f"📦 Executable size: {size_str} ({size_bytes:,} bytes)")
        print(f"📍 Location: {executable}")
    else:
        print("⚠️  Could not find executable to measure size")


@task
def build(c, run=False, clean_first=False):
    """Build the project

    Args:
        run: Run the executable after building
        clean_first: Clean before building
    """
    if clean_first:
        clean(c)
        configure(c)

    print("🔨 Building project...")

    # Ensure build directory exists and is configured
    if not os.path.exists("build/Makefile") and not os.path.exists("build/build.ninja"):
        print("📋 No build files found, configuring first...")
        configure(c)

    # Build
    with c.cd("build"):
        # Try ninja first, fall back to make
        try:
            c.run("ninja")
        except:
            c.run("make -j$(nproc)")

    print("✅ Build complete")

    # Print binary size
    print_binary_size(c)

    if run:
        run_app(c)


@task
def run(c):
    """Run the built application"""
    run_app(c)


def run_app(c):
    """Helper function to run the application"""
    executable_paths = [
        "build/korova",
        "build/src/app/korova",
        "build/Debug/korova",
        "build/Release/korova",
        "build/src/app/korova.app/Contents/MacOS/korova",  # macOS .app bundle path
    ]

    executable = None
    for path in executable_paths:
        if os.path.exists(path):
            executable = path
            break

    if executable:
        print(f"🚀 Running {executable}...")
        c.run(executable)
    else:
        print("❌ Executable not found. Available files in build/:")
        c.run(
            "find build -type f -executable 2>/dev/null || echo 'No executables found'"
        )


@task
def rebuild(c, run=False):
    """Clean, configure, and build"""
    clean(c)
    configure(c)
    build(c, run=run)


@task
def debug(c):
    """Build and run with debugging"""
    print("🐛 Building for debug...")
    Path("build").mkdir(exist_ok=True)

    with c.cd("build"):
        c.run("cmake -DCMAKE_BUILD_TYPE=Debug ..")
        try:
            c.run("ninja")
        except:
            c.run("make -j$(nproc)")

    print("🚀 Running with gdb...")
    c.run("gdb build/korova")


@task
def install_deps(c):
    """Install development dependencies (Ubuntu/Debian)"""
    print("📦 Installing development dependencies...")
    c.run("sudo apt update")
    c.run(
        "sudo apt install -y build-essential cmake ninja-build qt6-base-dev qt6-tools-dev"
    )
    print("✅ Dependencies installed")


@task
def test(c, name=None):
    """Build and run all tests, or a specific test by name (use --name)."""
    print("🧪 Building tests...")
    build(c)

    # Find test binaries in common locations
    test_paths = [
        "build/tests",
        "build/src/tests",
        "build/Debug/tests",
        "build/Release/tests",
        "build/src/tests/Debug",
        "build/src/tests/Release",
    ]

    test_dir = None
    for path in test_paths:
        if os.path.exists(path):
            test_dir = path
            break

    if not test_dir:
        print("❌ Test binaries not found. Available files in build/:")
        c.run(
            "find build -type f -executable 2>/dev/null || echo 'No executables found'"
        )
        return

    with c.cd(test_dir):
        if name:
            print(f"🔎 Running test: {name}")
            c.run(f"./{name}")
        else:
            print("🔎 Running all tests...")
            c.run("ctest --output-on-failure")
    print("✅ Test(s) complete")


@task
def lint(c, fix=False):
    """Lint and optionally fix C/C++/CMake files using clang-format

    Args:
        fix: If True, fix formatting issues automatically
    """
    print("🔍 Linting code with clang-format...")

    # Check if clang-format is available
    result = c.run("which clang-format", hide=True, warn=True)
    if result.failed:
        print("❌ clang-format not found. Please install it:")
        print("   macOS: brew install clang-format")
        print("   Ubuntu: sudo apt install clang-format")
        return

    # Find source files in src directory only, excluding third-party code
    exclude_dirs = [
        "d2xx",  # Third-party FTDI library
        "autogen",  # Qt auto-generated files
        "CMakeFiles",
    ]

    # Build find command with exclusions - use proper syntax
    exclude_args = " ".join([f"-not -path '*/{dir}/*'" for dir in exclude_dirs])

    # Find source files in src directory only (excluding third-party code)
    source_files_cmd = f"find ./src -name '*.c' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' {exclude_args}"
    result = c.run(source_files_cmd, hide=True, warn=True)
    source_files = []
    if not result.failed and result.stdout.strip():
        source_files = [
            f.strip() for f in result.stdout.strip().split("\n") if f.strip()
        ]

    # Note: CMakeLists.txt files are excluded from clang-format as it doesn't handle CMake syntax properly
    # CMake files should be formatted manually or with cmake-format if needed

    # Only use source files for clang-format
    all_files = sorted(source_files)

    if not all_files:
        print("⚠️  No C/C++ files found to lint")
        return

    print(f"📁 Found {len(all_files)} C/C++ files to lint:")
    for file in all_files:
        print(f"   {file}")

    if fix:
        print("🔧 Fixing formatting issues...")
        for file in all_files:
            print(f"   Formatting: {file}")
            c.run(f"clang-format -i {file}")
        print("✅ Formatting fixes applied")
    else:
        print("🔍 Checking formatting (use --fix to apply fixes)...")
        issues_found = False
        for file in all_files:
            # Check if file needs formatting
            result = c.run(
                f"clang-format --dry-run --Werror {file}", hide=True, warn=True
            )
            if result.failed:
                print(f"❌ {file} needs formatting")
                issues_found = True

        if not issues_found:
            print("✅ All files are properly formatted")
        else:
            print("⚠️  Some files need formatting. Run 'invoke lint --fix' to fix them")
