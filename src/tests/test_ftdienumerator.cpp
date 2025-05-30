#include "FTDIEnumerator.h"
#include <iostream>

int main()
{
    std::cout << "=== FTDIEnumerator Integration Test ===" << std::endl;

    // Test device count
    int deviceCount = FTDIEnumerator::getDeviceCount();
    std::cout << "Device count: " << deviceCount << std::endl;

    if (deviceCount <= 0)
    {
        std::cerr << "No FTDI devices found for FTDIEnumerator test." << std::endl;
        if (deviceCount < 0)
        {
            std::cerr << "Error: " << FTDIEnumerator::getLastError() << std::endl;
        }
        return 1;
    }

    // Test device enumeration
    auto devices = FTDIEnumerator::enumerateDevices();
    std::cout << "Found " << devices.size() << " FTDI device(s):" << std::endl;

    for (const auto &dev : devices)
    {
        std::cout << "  Index: " << dev.index << std::endl;
        std::cout << "  Serial: " << dev.serialNumber << std::endl;
        std::cout << "  Description: " << dev.description << std::endl;
        std::cout << "  Device ID: 0x" << std::hex << dev.deviceId << std::dec << std::endl;
        std::cout << "  Location ID: 0x" << std::hex << dev.locId << std::dec << std::endl;
        std::cout << "  Flags: 0x" << std::hex << dev.flags << std::dec << std::endl;
        std::cout << "  Type: " << dev.type << " (" << FTDIEnumerator::deviceTypeToString(dev.type)
                  << ")" << std::endl;
        std::cout << "  ---" << std::endl;
    }

    // Test serial number retrieval
    auto serials = FTDIEnumerator::getSerialNumbers();
    std::cout << "Serial numbers: ";
    for (const auto &serial : serials)
    {
        std::cout << serial << " ";
    }
    std::cout << std::endl;

    std::cout << "FTDIEnumerator integration test completed successfully." << std::endl;
    return 0;
}