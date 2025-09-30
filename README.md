# film_scanner_pc

A film scanner application.

## Dependencies

- Qt6 (Core, Widgets)
- OpenCV 4.5.0 or higher (configured for static linking)

## Quick Start

### Prerequisites

The easiest way to get started is to use the bootstrap function:

```bash
# Install all dependencies automatically (including static OpenCV libraries)
invoke bootstrap

# Build the project
invoke build

# Run the application
invoke run
```

### Manual Installation

If you prefer to install dependencies manually:

#### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build qt6-base-dev qt6-tools-dev libopencv-dev libopencv-contrib-dev
```

#### macOS
```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies (Homebrew OpenCV includes static libraries)
brew install cmake ninja qt@6 opencv
```

#### Windows
```bash
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
.\vcpkg install opencv:x64-windows-static

# Install Qt6 manually from https://www.qt.io/download
```

## Available Commands

```bash
# Dependency management
invoke bootstrap          # Install all dependencies (first time setup)
invoke check-opencv       # Check OpenCV installation and static libraries

# Project management
invoke build              # Build the project
invoke run                # Run the application
invoke test               # Run tests
invoke clean              # Clean build artifacts
invoke rebuild            # Clean, configure, and build
```

## Features

- Film scanning capabilities
- Real-time preview
- Multiple export formats (TIFF, JPEG, PNG, RAW)
- Adjustable color settings
- Cross-platform support (Linux, macOS, Windows)
- OpenCV integration for image processing
- Static linking for self-contained executables

## License

See [LICENSE](LICENSE) file for details.