#include "FTDIEnumerator.h"
#include "knokke_v1.h"
#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <signal.h>
#include <thread>
#include <vector>

// Global flag for graceful shutdown
std::atomic<bool> g_running{true};

// Signal handler for graceful shutdown
void signalHandler(int signal)
{
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    g_running.store(false);
}

int main()
{
    std::cout << "=== KnokkeV1 Scanner Example (8-bit & 16-bit) ===" << std::endl;

    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Find available FTDI devices
    auto serials = FTDIEnumerator::getSerialNumbers();

    if (serials.empty())
    {
        std::cout << "No FTDI devices found." << std::endl;
        std::cout << "Please connect a KnokkeV1 scanner or compatible FTDI device." << std::endl;
        return 1;
    }

    std::cout << "Found " << serials.size() << " FTDI device(s):" << std::endl;
    for (size_t i = 0; i < serials.size(); ++i)
    {
        std::cout << "  " << (i + 1) << ". " << serials[i] << std::endl;
    }

    // Use the first available device
    std::string selectedSerial = serials.front();
    std::cout << "\nUsing device: " << selectedSerial << std::endl;

    // Create scanner instance
    KnokkeV1 scanner;

    // Set up frame callbacks for both 8-bit and 16-bit
    std::atomic<size_t> frames8BitReceived{0};
    std::atomic<size_t> frames16BitReceived{0};
    std::atomic<size_t> totalBytesIn8BitFrames{0};
    std::atomic<size_t> totalBytesIn16BitFrames{0};

    // 8-bit frame callback
    scanner.registerFrameCallback(
        [&frames8BitReceived, &totalBytesIn8BitFrames](const std::vector<uint8_t> &frameData)
        {
            frames8BitReceived.fetch_add(1);
            totalBytesIn8BitFrames.fetch_add(frameData.size());

            std::cout << "8-bit Frame " << frames8BitReceived.load()
                      << " received: " << frameData.size() << " bytes" << std::endl;

            // Example: Print frame statistics
            if (frameData.size() == KnokkeV1::FRAME_SIZE_BYTES)
            {
                std::cout << "  8-bit Frame dimensions: " << KnokkeV1::FRAME_WIDTH << "x"
                          << KnokkeV1::FRAME_HEIGHT << std::endl;

                // Calculate some basic statistics
                uint8_t  minVal = 255, maxVal = 0;
                uint32_t sum = 0;
                for (uint8_t pixel : frameData)
                {
                    minVal = std::min(minVal, pixel);
                    maxVal = std::max(maxVal, pixel);
                    sum += pixel;
                }
                double avgVal = static_cast<double>(sum) / frameData.size();

                std::cout << "  8-bit Pixel range: " << static_cast<int>(minVal) << " - "
                          << static_cast<int>(maxVal) << " (avg: " << std::fixed
                          << std::setprecision(1) << avgVal << ")" << std::endl;
            }
        });

    // 16-bit frame callback
    scanner.registerFrame16BitCallback(
        [&frames16BitReceived, &totalBytesIn16BitFrames](const std::vector<uint16_t> &frameData)
        {
            frames16BitReceived.fetch_add(1);
            totalBytesIn16BitFrames.fetch_add(frameData.size() * sizeof(uint16_t));

            std::cout << "16-bit Frame " << frames16BitReceived.load()
                      << " received: " << frameData.size() << " words ("
                      << (frameData.size() * sizeof(uint16_t)) << " bytes)" << std::endl;

            // Example: Print frame statistics
            if (frameData.size() == KnokkeV1::FRAME_SIZE_WORDS)
            {
                std::cout << "  16-bit Frame dimensions: " << KnokkeV1::FRAME_WIDTH << "x"
                          << KnokkeV1::FRAME_HEIGHT << std::endl;

                // Calculate some basic statistics
                uint16_t minVal = 65535, maxVal = 0;
                uint32_t sum = 0;
                for (uint16_t pixel : frameData)
                {
                    minVal = std::min(minVal, pixel);
                    maxVal = std::max(maxVal, pixel);
                    sum += pixel;
                }
                double avgVal = static_cast<double>(sum) / frameData.size();

                std::cout << "  16-bit Pixel range: " << minVal << " - " << maxVal
                          << " (avg: " << std::fixed << std::setprecision(1) << avgVal << ")"
                          << std::endl;

                // Show that 16-bit values have 8-bit data in upper byte
                std::cout << "  16-bit format: 8-bit data in upper byte (0x" << std::hex
                          << (maxVal & 0xFF00) << "00)" << std::dec << std::endl;
            }
        });

    // Open device
    std::cout << "\nOpening device..." << std::endl;
    if (!scanner.openDevice(selectedSerial))
    {
        std::cerr << "Failed to open device!" << std::endl;
        return 1;
    }

    std::cout << "Device opened successfully!" << std::endl;

    // Demonstrate control interface access
    FTD2XXWrapper *controlInterface = scanner.getControlInterface();
    if (controlInterface)
    {
        std::cout << "Control interface available" << std::endl;
        std::cout << "You can now use the control interface to send commands to the scanner"
                  << std::endl;

        // Example: Set up a callback for control interface responses
        controlInterface->registerCallback(
            [](size_t bytesAdded)
            { std::cout << "Control interface received " << bytesAdded << " bytes" << std::endl; });

        // Example: Send a command (this would be implemented based on the scanner's protocol)
        // std::vector<uint8_t> command = {0x01, 0x02, 0x03}; // Example command
        // controlInterface->writeData(command);
    }
    else
    {
        std::cout << "Warning: Control interface not available" << std::endl;
    }

    std::cout << "Waiting for frame data..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;

    // Main processing loop
    auto startTime = std::chrono::steady_clock::now();

    while (g_running.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Process any frames in both queues
        std::vector<uint8_t> frame8Bit;
        while (scanner.getNextFrame(frame8Bit))
        {
            // Additional 8-bit frame processing can be done here
            // The callback above handles most of the processing
        }

        std::vector<uint16_t> frame16Bit;
        while (scanner.getNextFrame16Bit(frame16Bit))
        {
            // Additional 16-bit frame processing can be done here
            // The callback above handles most of the processing
        }

        // Print status every 5 seconds
        static auto lastStatusTime = startTime;
        auto        currentTime    = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastStatusTime)
                .count() >= 5)
        {
            auto elapsed =
                std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime);
            size_t current8BitFrameCount  = frames8BitReceived.load();
            size_t current16BitFrameCount = frames16BitReceived.load();
            size_t frame8BitQueueSize     = scanner.getFrameQueueSize();
            size_t frame16BitQueueSize    = scanner.getFrame16BitQueueSize();
            size_t totalBytesProcessed    = scanner.getTotalBytesProcessed();

            double frameRate8Bit  = 0.0;
            double frameRate16Bit = 0.0;
            if (elapsed.count() > 0)
            {
                frameRate8Bit  = static_cast<double>(current8BitFrameCount) / elapsed.count();
                frameRate16Bit = static_cast<double>(current16BitFrameCount) / elapsed.count();
            }

            std::cout << "\n[" << elapsed.count() << "s] Status:" << std::endl;
            std::cout << "  8-bit:  Frames=" << current8BitFrameCount
                      << " | Queue=" << frame8BitQueueSize << " | Rate=" << std::fixed
                      << std::setprecision(1) << frameRate8Bit << " fps" << std::endl;
            std::cout << "  16-bit: Frames=" << current16BitFrameCount
                      << " | Queue=" << frame16BitQueueSize << " | Rate=" << std::fixed
                      << std::setprecision(1) << frameRate16Bit << " fps" << std::endl;
            std::cout << "  Total:  Data=" << (totalBytesProcessed / 1024) << " KB" << std::endl;

            lastStatusTime = currentTime;
        }
    }

    // Close device
    std::cout << "\nClosing device..." << std::endl;
    scanner.closeDevice();

    // Print final statistics
    std::cout << "\n=== Final Statistics ===" << std::endl;
    std::cout << "8-bit frames received: " << frames8BitReceived.load() << std::endl;
    std::cout << "16-bit frames received: " << frames16BitReceived.load() << std::endl;
    std::cout << "Total bytes in 8-bit frames: " << totalBytesIn8BitFrames.load() << std::endl;
    std::cout << "Total bytes in 16-bit frames: " << totalBytesIn16BitFrames.load() << std::endl;
    std::cout << "Total bytes processed: " << scanner.getTotalBytesProcessed() << std::endl;
    std::cout << "8-bit frame queue size: " << scanner.getFrameQueueSize() << std::endl;
    std::cout << "16-bit frame queue size: " << scanner.getFrame16BitQueueSize() << std::endl;

    if (frames8BitReceived.load() > 0)
    {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - startTime);
        double avgFrameRate8Bit = static_cast<double>(frames8BitReceived.load()) / elapsed.count();
        std::cout << "Average 8-bit frame rate: " << std::fixed << std::setprecision(1)
                  << avgFrameRate8Bit << " fps" << std::endl;
        std::cout << "Average 8-bit frame size: "
                  << (totalBytesIn8BitFrames.load() / frames8BitReceived.load()) << " bytes"
                  << std::endl;
    }

    if (frames16BitReceived.load() > 0)
    {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - startTime);
        double avgFrameRate16Bit =
            static_cast<double>(frames16BitReceived.load()) / elapsed.count();
        std::cout << "Average 16-bit frame rate: " << std::fixed << std::setprecision(1)
                  << avgFrameRate16Bit << " fps" << std::endl;
        std::cout << "Average 16-bit frame size: "
                  << (totalBytesIn16BitFrames.load() / frames16BitReceived.load()) << " bytes"
                  << std::endl;
    }

    std::cout << "\nExample completed." << std::endl;
    return 0;
}