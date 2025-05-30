#include "FTD2XXWrapper.h"
#include "FTDIEnumerator.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

int main()
{
    std::cout << "=== FTD2XXWrapper Integration Test ===" << std::endl;

    // Find a device to test with
    auto serials = FTDIEnumerator::getSerialNumbers();

    if (serials.empty())
    {
        std::cerr << "No FTDI devices found for FTD2XXWrapper test." << std::endl;
        return 1;
    }

    std::string serial = serials.front();
    std::cout << "Testing FTD2XXWrapper with device serial: " << serial << std::endl;

    // Test wrapper functionality
    FTD2XXWrapper wrapper;

    // Test opening device
    std::cout << "Opening device..." << std::endl;
    bool opened = wrapper.openBySerial(serial);
    if (!opened)
    {
        std::cerr << "Failed to open device with serial: " << serial << std::endl;
        return 1;
    }
    std::cout << "Device opened successfully." << std::endl;

    // Test callback registration
    std::atomic<size_t> totalBytesReceived{0};
    std::atomic<int>    callbackCount{0};

    wrapper.registerCallback(
        [&totalBytesReceived, &callbackCount](size_t bytesAdded)
        {
            totalBytesReceived += bytesAdded;
            callbackCount++;
            std::cout << "Callback " << callbackCount.load() << ": " << bytesAdded
                      << " bytes received (total: " << totalBytesReceived.load() << ")"
                      << std::endl;
        });
    std::cout << "Callback registered." << std::endl;

    // Wait for potential data
    std::cout << "Waiting for data (2 seconds)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Test data retrieval
    std::vector<uint8_t> data;
    int                  dataChunks = 0;
    while (wrapper.popData(data))
    {
        dataChunks++;
        std::cout << "Retrieved data chunk " << dataChunks << " with " << data.size() << " bytes"
                  << std::endl;
        if (dataChunks >= 10)
            break; // Prevent infinite loop
    }

    if (dataChunks == 0)
    {
        std::cout << "No data chunks retrieved (this is normal if no data was sent to the device)"
                  << std::endl;
    }

    // Test closing
    std::cout << "Closing device..." << std::endl;
    wrapper.close();
    std::cout << "Device closed." << std::endl;

    std::cout << "Summary:" << std::endl;
    std::cout << "  - Device opened: " << (opened ? "YES" : "NO") << std::endl;
    std::cout << "  - Callbacks triggered: " << callbackCount.load() << std::endl;
    std::cout << "  - Total bytes received: " << totalBytesReceived.load() << std::endl;
    std::cout << "  - Data chunks retrieved: " << dataChunks << std::endl;

    std::cout << "FTD2XXWrapper integration test completed successfully." << std::endl;
    return 0;
}