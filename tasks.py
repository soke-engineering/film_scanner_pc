from invoke import task
import os
import sys
from pathlib import Path


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
            "find build -name 'korova' -type f 2>/dev/null || echo 'No korova executable found'"
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
