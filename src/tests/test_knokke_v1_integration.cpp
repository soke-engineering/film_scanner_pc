#include "FTDIEnumerator.h"
#include "knokke_v1.h"
#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

// Integration test that demonstrates the complete knokke_v1 driver functionality
int main()
{
    std::cout << "=== KnokkeV1 Integration Test ===" << std::endl;

    // Find available Knokke V1 devices (SE prefix)
    auto                     allSerials = FTDIEnumerator::getSerialNumbers();
    std::vector<std::string> knokkeSerials;

    for (const auto &serial : allSerials)
    {
        if (serial.substr(0, 2) == "SE")
        {
            knokkeSerials.push_back(serial);
        }
    }

    if (knokkeSerials.empty())
    {
        std::cout << "No Knokke V1 devices found. This test requires a physical Knokke V1 scanner."
                  << std::endl;
        std::cout << "Please connect a Knokke V1 scanner (devices with SE prefix)." << std::endl;
        return 1;
    }

    // Get unique base serial numbers (without A/B suffix)
    auto uniqueSerials = FTDIEnumerator::getUniqueSerialNumbers();

    // Filter for only SE devices
    std::vector<std::string> knokkeUniqueSerials;
    for (const auto &serial : uniqueSerials)
    {
        if (serial.substr(0, 2) == "SE")
        {
            knokkeUniqueSerials.push_back(serial);
        }
    }

    if (knokkeUniqueSerials.empty())
    {
        std::cout << "No Knokke V1 devices found. This test requires a physical Knokke V1 scanner."
                  << std::endl;
        std::cout << "Please connect a Knokke V1 scanner (devices with SE prefix)." << std::endl;
        return 1;
    }

    std::cout << "Found " << knokkeSerials.size() << " FTDI device(s):" << std::endl;
    for (size_t i = 0; i < knokkeSerials.size(); ++i)
    {
        std::cout << "  " << (i + 1) << ". " << knokkeSerials[i] << std::endl;
    }

    // Use the first available Knokke V1 device (base serial without A/B suffix)
    std::string selectedSerial = knokkeUniqueSerials.front();
    std::cout << "\nUsing device: " << selectedSerial << std::endl;

    // Create scanner instance
    KnokkeV1 scanner;

    // Set up frame callback
    std::atomic<size_t> framesReceived{0};
    std::atomic<size_t> totalBytesInFrames{0};

    scanner.registerFrameCallback(
        [&framesReceived, &totalBytesInFrames](const std::vector<uint8_t> &frameData)
        {
            framesReceived.fetch_add(1);
            totalBytesInFrames.fetch_add(frameData.size());

            std::cout << "Frame " << framesReceived.load() << " received: " << frameData.size()
                      << " bytes" << std::endl;

            // Print first few bytes of the frame for debugging
            std::cout << "  First 16 bytes: ";
            for (size_t i = 0; i < std::min(size_t(16), frameData.size()); ++i)
            {
                std::cout << std::hex << std::setw(2) << std::setfill('0')
                          << static_cast<int>(frameData[i]) << " ";
            }
            std::cout << std::dec << std::endl;
        });

    // Open device
    std::cout << "\nOpening device..." << std::endl;
    if (!scanner.openDevice(selectedSerial))
    {
        std::cerr << "Failed to open device!" << std::endl;
        return 1;
    }

    std::cout << "Device opened successfully!" << std::endl;
    std::cout << "Waiting for frame data..." << std::endl;

    // Run for a fixed period (e.g., 5 seconds) instead of waiting for Ctrl+C
    constexpr int run_seconds    = 5;
    auto          startTime      = std::chrono::steady_clock::now();
    size_t        lastFrameCount = 0;

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime);

        size_t currentFrameCount   = framesReceived.load();
        size_t frameQueueSize      = scanner.getFrameQueueSize();
        size_t totalBytesProcessed = scanner.getTotalBytesProcessed();

        // Calculate frame rate
        double frameRate = 0.0;
        if (elapsed.count() > 0)
        {
            frameRate = static_cast<double>(currentFrameCount) / elapsed.count();
        }

        // Calculate bytes per second
        double bytesPerSecond = 0.0;
        if (elapsed.count() > 0)
        {
            bytesPerSecond = static_cast<double>(totalBytesProcessed) / elapsed.count();
        }

        // Print status
        std::cout << "[" << std::setw(3) << elapsed.count() << "s] "
                  << "Frames: " << std::setw(4) << currentFrameCount << " | Queue: " << std::setw(2)
                  << frameQueueSize << " | Rate: " << std::fixed << std::setprecision(1)
                  << std::setw(5) << frameRate << " fps"
                  << " | Data: " << std::setw(8) << static_cast<int>(bytesPerSecond / 1024)
                  << " KB/s"
                  << " | Total: " << std::setw(8) << (totalBytesProcessed / 1024) << " KB"
                  << std::endl;

        // Check for new frames in queue
        std::vector<uint8_t> frame;
        while (scanner.getNextFrame(frame))
        {
            // Process frame here if needed
            // For this test, we're just monitoring via callback
        }

        // Stop after run_seconds
        if (elapsed.count() >= run_seconds)
        {
            std::cout << "\nTest duration reached (" << run_seconds << " seconds). Stopping..."
                      << std::endl;
            break;
        }

        lastFrameCount = currentFrameCount;
    }

    // Close device
    std::cout << "\nClosing device..." << std::endl;
    scanner.closeDevice();

    // Print final statistics
    std::cout << "\n=== Final Statistics ===" << std::endl;
    std::cout << "Total frames received: " << framesReceived.load() << std::endl;
    std::cout << "Total bytes in frames: " << totalBytesInFrames.load() << std::endl;
    std::cout << "Total bytes processed: " << scanner.getTotalBytesProcessed() << std::endl;
    std::cout << "Frame queue size: " << scanner.getFrameQueueSize() << std::endl;

    if (framesReceived.load() > 0)
    {
        std::cout << "Average frame size: " << (totalBytesInFrames.load() / framesReceived.load())
                  << " bytes" << std::endl;
        std::cout << "Expected frame size: " << KnokkeV1::FRAME_SIZE_BYTES << " bytes" << std::endl;
    }

    std::cout << "\nIntegration test completed." << std::endl;
    return 0;
}