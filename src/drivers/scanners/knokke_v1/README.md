# KnokkeV1 Scanner Driver

The KnokkeV1 driver provides a complete implementation for interfacing with the KnokkeV1 film scanner via FTDI USB communication. It handles frame parsing, delimiter detection, and provides a callback mechanism for real-time frame processing.

## Features

- **Dual Interface Support**: Supports both data (A) and control (B) interfaces of the FT2232H
- **Frame Parsing**: Automatically detects and parses frames using start/end delimiters
- **Thread-Safe**: Multi-threaded design with proper synchronization
- **Callback System**: Real-time frame notifications via callbacks
- **Frame Queue**: Thread-safe frame queue for buffering
- **Statistics**: Comprehensive statistics tracking
- **POSIX Compliant**: No Qt dependencies, pure C++17 implementation

## Hardware Architecture

The KnokkeV1 scanner uses an FT2232H chip with two interfaces:
- **Interface A**: Image data stream (3840×12 Bayer frames)
- **Interface B**: Control interface for scanner commands and status

The driver automatically manages both interfaces using the base serial number with "A" and "B" suffixes.

## Frame Format

Each frame consists of:
- **Start Delimiter**: `0xFF 0x01 0xB1 0x6B`
- **Frame Data**: 3840 × 12 pixels of Bayer data (46,080 bytes)
- **End Delimiter**: `0xFF 0x01 0x01 0xB5`

### Frame Dimensions
- Width: 3840 pixels
- Height: 12 lines
- Total frame size: 46,080 bytes
- Data format: Bayer pattern (1 byte per pixel)

## Usage

### Basic Usage

```cpp
#include "knokke_v1.h"
#include "FTDIEnumerator.h"

int main() {
    // Find available devices
    auto serials = FTDIEnumerator::getSerialNumbers();
    if (serials.empty()) {
        std::cerr << "No FTDI devices found" << std::endl;
        return 1;
    }
    
    // Create scanner instance
    KnokkeV1 scanner;
    
    // Register frame callback
    scanner.registerFrameCallback([](const std::vector<uint8_t>& frameData) {
        std::cout << "Frame received: " << frameData.size() << " bytes" << std::endl;
        // Process frame data here
    });
    
    // Open device (automatically opens both interfaces)
    if (!scanner.openDevice(serials.front())) {
        std::cerr << "Failed to open device" << std::endl;
        return 1;
    }
    
    // Access control interface
    FTD2XXWrapper* controlInterface = scanner.getControlInterface();
    if (controlInterface) {
        // Send commands to the scanner
        // controlInterface->writeData(command);
    }
    
    // Main processing loop
    while (true) {
        std::vector<uint8_t> frame;
        if (scanner.getNextFrame(frame)) {
            // Process frame from queue
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Clean up
    scanner.closeDevice();
    return 0;
}
```

### Advanced Usage with Control Interface

```cpp
KnokkeV1 scanner;

// Set up frame callback
std::atomic<size_t> frameCount{0};
scanner.registerFrameCallback([&frameCount](const std::vector<uint8_t>& frameData) {
    frameCount.fetch_add(1);
    std::cout << "Frame " << frameCount.load() << " received" << std::endl;
});

// Open device
if (!scanner.openDevice(serial)) {
    return 1;
}

// Access control interface
FTD2XXWrapper* control = scanner.getControlInterface();
if (control) {
    // Set up control interface callback
    control->registerCallback([](size_t bytesReceived) {
        std::cout << "Control response: " << bytesReceived << " bytes" << std::endl;
    });
    
    // Example: Send scanner commands
    // std::vector<uint8_t> startCommand = {0x01, 0x00}; // Start scanning
    // control->writeData(startCommand);
    
    // std::vector<uint8_t> stopCommand = {0x02, 0x00}; // Stop scanning
    // control->writeData(stopCommand);
}

// Monitor statistics
while (true) {
    std::cout << "Frames: " << scanner.getTotalFramesReceived()
              << " | Queue: " << scanner.getFrameQueueSize()
              << " | Bytes: " << scanner.getTotalBytesProcessed() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
```

## API Reference

### Constructor/Destructor
```cpp
KnokkeV1();
~KnokkeV1();
```

### Device Management
```cpp
bool openDevice(const std::string& serial);
void closeDevice();
bool isDeviceOpen() const;
```

### Control Interface Access
```cpp
FTD2XXWrapper* getControlInterface();
const FTD2XXWrapper* getControlInterface() const;
```

### Frame Processing
```cpp
void registerFrameCallback(FrameCallback callback);
bool getNextFrame(std::vector<uint8_t>& frameData);
size_t getFrameQueueSize() const;
```

### Statistics
```cpp
size_t getTotalFramesReceived() const;
size_t getTotalBytesProcessed() const;
void resetStatistics();
```

### Testing Methods (Public for unit testing)
```cpp
void processIncomingData(const std::vector<uint8_t>& data);
bool findFrameDelimiter(const std::vector<uint8_t>& data, size_t startPos, 
                       const std::vector<uint8_t>& delimiter, size_t& foundPos);
bool validateFrame(const std::vector<uint8_t>& frameData);
```

## Constants

```cpp
static constexpr size_t FRAME_WIDTH = 3840;
static constexpr size_t FRAME_HEIGHT = 12;
static constexpr size_t FRAME_SIZE_BYTES = FRAME_WIDTH * FRAME_HEIGHT;

// Start delimiter
static constexpr uint8_t START_DELIM_0 = 0xFF;
static constexpr uint8_t START_DELIM_1 = 0x01;
static constexpr uint8_t START_DELIM_2 = 0xB1;
static constexpr uint8_t START_DELIM_3 = 0x6B;

// End delimiter
static constexpr uint8_t END_DELIM_0 = 0xFF;
static constexpr uint8_t END_DELIM_1 = 0x01;
static constexpr uint8_t END_DELIM_2 = 0x01;
static constexpr uint8_t END_DELIM_3 = 0xB5;
```

## Interface Management

The driver automatically manages the dual interfaces of the FT2232H:

- **Data Interface (A)**: Handles incoming frame data with automatic parsing
- **Control Interface (B)**: Provides direct access for sending commands and receiving responses

When you call `openDevice(serial)`, the driver:
1. Opens interface A as `{serial}A` for data
2. Opens interface B as `{serial}B` for control
3. Sets up frame parsing on the data interface
4. Provides access to the control interface via `getControlInterface()`

## Testing

### Unit Tests
Run the unit test suite:
```bash
cd build
make test_knokke_v1
./src/tests/test_knokke_v1
```

### Integration Tests
Run the integration test with a physical device:
```bash
cd build
make test_knokke_v1_integration
./src/tests/test_knokke_v1_integration
```

### Example Program
Run the example program:
```bash
cd build
make knokke_v1_example
./src/examples/knokke_v1_example
```

## Test Coverage

The test suite covers:
- Frame delimiter detection
- Frame validation
- Frame parsing and queueing
- Callback functionality
- Multiple frame processing
- Device management
- Frame queue management
- Control interface access
- Error handling

## Thread Safety

The driver is designed to be thread-safe:
- Frame queue access is protected by mutex
- Callback registration is thread-safe
- Statistics are atomic
- Device state is atomic
- Control interface access is thread-safe

## Performance Considerations

- Frame buffer is pre-allocated to avoid memory allocations during parsing
- Delimiter search uses efficient byte-by-byte comparison
- Frame queue uses move semantics to avoid unnecessary copying
- Callback is called from the parsing thread (consider using a separate thread for heavy processing)
- Both interfaces operate independently for optimal performance

## Error Handling

- Invalid frame sizes are detected and discarded
- Missing delimiters are handled gracefully
- Device connection errors are reported for both interfaces
- Statistics track error counts
- Control interface failures are handled gracefully

## Dependencies

- C++17 or later
- FTDI D2XX library
- POSIX threads
- Standard C++ library

## Building

The driver is built as part of the main project:
```bash
mkdir build
cd build
cmake ..
make
```

## License

See the main project LICENSE file for details. 