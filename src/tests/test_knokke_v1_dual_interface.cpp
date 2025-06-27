#include "knokke_v1.h"
#include <cassert>
#include <iostream>

// Test the dual interface functionality without requiring physical devices
int main()
{
    std::cout << "=== KnokkeV1 Dual Interface Test ===" << std::endl;

    KnokkeV1 scanner;

    // Test 1: Control interface access when device is closed
    std::cout << "Test 1: Control interface access when closed" << std::endl;
    if (scanner.getControlInterface() == nullptr)
    {
        std::cout << "✓ Control interface returns nullptr when device is closed" << std::endl;
    }
    else
    {
        std::cout << "✗ Control interface should return nullptr when device is closed" << std::endl;
        return 1;
    }

    // Test 2: Const version of control interface access
    std::cout << "\nTest 2: Const control interface access when closed" << std::endl;
    const KnokkeV1 &constScanner = scanner;
    if (constScanner.getControlInterface() == nullptr)
    {
        std::cout << "✓ Const control interface returns nullptr when device is closed" << std::endl;
    }
    else
    {
        std::cout << "✗ Const control interface should return nullptr when device is closed"
                  << std::endl;
        return 1;
    }

    // Test 3: Device state when closed
    std::cout << "\nTest 3: Device state when closed" << std::endl;
    if (!scanner.isDeviceOpen())
    {
        std::cout << "✓ Device reports as closed initially" << std::endl;
    }
    else
    {
        std::cout << "✗ Device should report as closed initially" << std::endl;
        return 1;
    }

    // Test 4: Try to open with a non-existent device (should fail gracefully)
    std::cout << "\nTest 4: Attempt to open non-existent device" << std::endl;
    bool opened = scanner.openDevice("NONEXISTENT123");
    if (!opened)
    {
        std::cout << "✓ Failed to open non-existent device (expected)" << std::endl;
    }
    else
    {
        std::cout << "✗ Should not be able to open non-existent device" << std::endl;
        return 1;
    }

    // Test 5: Control interface should still be nullptr after failed open
    std::cout << "\nTest 5: Control interface after failed open" << std::endl;
    if (scanner.getControlInterface() == nullptr)
    {
        std::cout << "✓ Control interface still nullptr after failed open" << std::endl;
    }
    else
    {
        std::cout << "✗ Control interface should be nullptr after failed open" << std::endl;
        return 1;
    }

    // Test 6: Device should still be closed after failed open
    std::cout << "\nTest 6: Device state after failed open" << std::endl;
    if (!scanner.isDeviceOpen())
    {
        std::cout << "✓ Device still reports as closed after failed open" << std::endl;
    }
    else
    {
        std::cout << "✗ Device should still report as closed after failed open" << std::endl;
        return 1;
    }

    // Test 7: Close device when already closed (should not crash)
    std::cout << "\nTest 7: Close device when already closed" << std::endl;
    scanner.closeDevice();
    std::cout << "✓ Close device when already closed completed without error" << std::endl;

    // Test 8: Control interface should still be nullptr after close
    std::cout << "\nTest 8: Control interface after close" << std::endl;
    if (scanner.getControlInterface() == nullptr)
    {
        std::cout << "✓ Control interface still nullptr after close" << std::endl;
    }
    else
    {
        std::cout << "✗ Control interface should be nullptr after close" << std::endl;
        return 1;
    }

    // Test 9: Device should still be closed after close
    std::cout << "\nTest 9: Device state after close" << std::endl;
    if (!scanner.isDeviceOpen())
    {
        std::cout << "✓ Device still reports as closed after close" << std::endl;
    }
    else
    {
        std::cout << "✗ Device should still report as closed after close" << std::endl;
        return 1;
    }

    // Test 10: Frame queue should be empty
    std::cout << "\nTest 10: Frame queue state" << std::endl;
    if (scanner.getFrameQueueSize() == 0)
    {
        std::cout << "✓ Frame queue is empty" << std::endl;
    }
    else
    {
        std::cout << "✗ Frame queue should be empty" << std::endl;
        return 1;
    }

    // Test 11: Statistics should be zero
    std::cout << "\nTest 11: Statistics state" << std::endl;
    if (scanner.getTotalFramesReceived() == 0 && scanner.getTotalBytesProcessed() == 0)
    {
        std::cout << "✓ Statistics are zero" << std::endl;
    }
    else
    {
        std::cout << "✗ Statistics should be zero" << std::endl;
        return 1;
    }

    // Test 12: Try to get frame when queue is empty
    std::cout << "\nTest 12: Get frame from empty queue" << std::endl;
    std::vector<uint8_t> frame;
    if (!scanner.getNextFrame(frame))
    {
        std::cout << "✓ getNextFrame returns false when queue is empty" << std::endl;
    }
    else
    {
        std::cout << "✗ getNextFrame should return false when queue is empty" << std::endl;
        return 1;
    }

    std::cout << "\n=== All Dual Interface Tests Passed ===" << std::endl;
    std::cout << "\nNote: This test verifies the dual interface architecture" << std::endl;
    std::cout << "without requiring physical hardware. The driver is designed" << std::endl;
    std::cout << "to handle both data (A) and control (B) interfaces of the" << std::endl;
    std::cout << "FT2232H chip automatically." << std::endl;

    return 0;
}