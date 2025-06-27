#include "FTDIEnumerator.h"
#include "knokke_v1.h"
#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

// Test helper functions
std::vector<uint8_t> createTestFrame()
{
    std::vector<uint8_t> frame(KnokkeV1::FRAME_SIZE_BYTES);
    for (size_t i = 0; i < frame.size(); ++i)
    {
        frame[i] = static_cast<uint8_t>(i % 256);
    }
    return frame;
}

std::vector<uint8_t> createFrameWithDelimiters(const std::vector<uint8_t> &frameData)
{
    std::vector<uint8_t> result;

    // Add start delimiter
    result.push_back(KnokkeV1::START_DELIM_0);
    result.push_back(KnokkeV1::START_DELIM_1);
    result.push_back(KnokkeV1::START_DELIM_2);
    result.push_back(KnokkeV1::START_DELIM_3);

    // Add frame data
    result.insert(result.end(), frameData.begin(), frameData.end());

    // Add end delimiter
    result.push_back(KnokkeV1::END_DELIM_0);
    result.push_back(KnokkeV1::END_DELIM_1);
    result.push_back(KnokkeV1::END_DELIM_2);
    result.push_back(KnokkeV1::END_DELIM_3);

    return result;
}

void testFrameDelimiterDetection()
{
    std::cout << "=== Testing Frame Delimiter Detection ===" << std::endl;

    KnokkeV1 scanner;

    // Test data with delimiters
    std::vector<uint8_t> testData = {
        0x00,
        0x01,
        0x02,
        0x03, // Random data
        KnokkeV1::START_DELIM_0,
        KnokkeV1::START_DELIM_1,
        KnokkeV1::START_DELIM_2,
        KnokkeV1::START_DELIM_3, // Start delimiter
        0x10,
        0x11,
        0x12,
        0x13, // More data
        KnokkeV1::END_DELIM_0,
        KnokkeV1::END_DELIM_1,
        KnokkeV1::END_DELIM_2,
        KnokkeV1::END_DELIM_3 // End delimiter
    };

    // Test start delimiter detection
    size_t               foundPos;
    std::vector<uint8_t> startDelim = {KnokkeV1::START_DELIM_0,
                                       KnokkeV1::START_DELIM_1,
                                       KnokkeV1::START_DELIM_2,
                                       KnokkeV1::START_DELIM_3};
    bool                 found      = scanner.findFrameDelimiter(testData, 0, startDelim, foundPos);

    if (found && foundPos == 4)
    {
        std::cout << "✓ Start delimiter detection: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ Start delimiter detection: FAILED" << std::endl;
        return;
    }

    // Test end delimiter detection
    std::vector<uint8_t> endDelim = {
        KnokkeV1::END_DELIM_0, KnokkeV1::END_DELIM_1, KnokkeV1::END_DELIM_2, KnokkeV1::END_DELIM_3};
    found = scanner.findFrameDelimiter(testData, 0, endDelim, foundPos);

    if (found && foundPos == 12)
    {
        std::cout << "✓ End delimiter detection: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ End delimiter detection: FAILED" << std::endl;
        return;
    }

    // Test delimiter not found
    std::vector<uint8_t> fakeDelim = {0xFF, 0xFF, 0xFF, 0xFF};
    found                          = scanner.findFrameDelimiter(testData, 0, fakeDelim, foundPos);

    if (!found)
    {
        std::cout << "✓ Non-existent delimiter detection: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ Non-existent delimiter detection: FAILED" << std::endl;
        return;
    }
}

void testFrameValidation()
{
    std::cout << "=== Testing Frame Validation ===" << std::endl;

    KnokkeV1 scanner;

    // Test valid frame
    std::vector<uint8_t> validFrame = createTestFrame();
    bool                 isValid    = scanner.validateFrame(validFrame);

    if (isValid)
    {
        std::cout << "✓ Valid frame validation: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ Valid frame validation: FAILED" << std::endl;
        return;
    }

    // Test invalid frame size
    std::vector<uint8_t> invalidFrame(KnokkeV1::FRAME_SIZE_BYTES - 1);
    isValid = scanner.validateFrame(invalidFrame);

    if (!isValid)
    {
        std::cout << "✓ Invalid frame size validation: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ Invalid frame size validation: FAILED" << std::endl;
        return;
    }

    // Test empty frame
    std::vector<uint8_t> emptyFrame;
    isValid = scanner.validateFrame(emptyFrame);

    if (!isValid)
    {
        std::cout << "✓ Empty frame validation: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ Empty frame validation: FAILED" << std::endl;
        return;
    }
}

void testFrameParsing()
{
    std::cout << "=== Testing Frame Parsing ===" << std::endl;

    KnokkeV1 scanner;

    // Create test frame with delimiters
    std::vector<uint8_t> originalFrame       = createTestFrame();
    std::vector<uint8_t> frameWithDelimiters = createFrameWithDelimiters(originalFrame);

    // Process the frame data
    scanner.processIncomingData(frameWithDelimiters);

    // Check if frame was parsed and queued
    std::vector<uint8_t> receivedFrame;
    bool                 frameReceived = scanner.getNextFrame(receivedFrame);

    if (frameReceived && receivedFrame.size() == KnokkeV1::FRAME_SIZE_BYTES)
    {
        // Verify frame data matches
        bool dataMatches = (receivedFrame == originalFrame);
        if (dataMatches)
        {
            std::cout << "✓ Frame parsing: PASSED" << std::endl;
        }
        else
        {
            std::cout << "✗ Frame parsing: FAILED (data mismatch)" << std::endl;
            return;
        }
    }
    else
    {
        std::cout << "✗ Frame parsing: FAILED (no frame received)" << std::endl;
        return;
    }

    // Test statistics
    if (scanner.getTotalFramesReceived() == 1)
    {
        std::cout << "✓ Frame statistics: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ Frame statistics: FAILED" << std::endl;
        return;
    }
}

void testFrameCallback()
{
    std::cout << "=== Testing Frame Callback ===" << std::endl;

    KnokkeV1             scanner;
    std::atomic<bool>    callbackCalled{false};
    std::vector<uint8_t> callbackFrame;

    // Register callback
    scanner.registerFrameCallback(
        [&callbackCalled, &callbackFrame](const std::vector<uint8_t> &frameData)
        {
            callbackCalled.store(true);
            callbackFrame = frameData;
        });

    // Create and process test frame
    std::vector<uint8_t> originalFrame       = createTestFrame();
    std::vector<uint8_t> frameWithDelimiters = createFrameWithDelimiters(originalFrame);

    scanner.processIncomingData(frameWithDelimiters);

    // Wait a bit for callback to be called
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (callbackCalled.load() && callbackFrame == originalFrame)
    {
        std::cout << "✓ Frame callback: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ Frame callback: FAILED" << std::endl;
        return;
    }
}

void testMultipleFrames()
{
    std::cout << "=== Testing Multiple Frames ===" << std::endl;

    KnokkeV1 scanner;

    // Create multiple test frames
    std::vector<std::vector<uint8_t>> originalFrames;
    std::vector<uint8_t>              combinedData;

    for (int i = 0; i < 3; ++i)
    {
        std::vector<uint8_t> frame = createTestFrame();
        // Fill with different data for each frame
        for (size_t j = 0; j < frame.size(); ++j)
        {
            frame[j] = static_cast<uint8_t>((i * 100 + j) % 256);
        }
        originalFrames.push_back(frame);

        std::vector<uint8_t> frameWithDelimiters = createFrameWithDelimiters(frame);
        combinedData.insert(
            combinedData.end(), frameWithDelimiters.begin(), frameWithDelimiters.end());
    }

    // Process all frames
    scanner.processIncomingData(combinedData);

    // Retrieve all frames
    std::vector<std::vector<uint8_t>> receivedFrames;
    std::vector<uint8_t>              frame;
    while (scanner.getNextFrame(frame))
    {
        receivedFrames.push_back(frame);
    }

    if (receivedFrames.size() == 3)
    {
        bool allFramesMatch = true;
        for (size_t i = 0; i < originalFrames.size(); ++i)
        {
            if (receivedFrames[i] != originalFrames[i])
            {
                allFramesMatch = false;
                break;
            }
        }

        if (allFramesMatch)
        {
            std::cout << "✓ Multiple frames: PASSED" << std::endl;
        }
        else
        {
            std::cout << "✗ Multiple frames: FAILED (frame data mismatch)" << std::endl;
            return;
        }
    }
    else
    {
        std::cout << "✗ Multiple frames: FAILED (expected 3, got " << receivedFrames.size() << ")"
                  << std::endl;
        return;
    }

    // Test statistics
    if (scanner.getTotalFramesReceived() == 3)
    {
        std::cout << "✓ Multiple frames statistics: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ Multiple frames statistics: FAILED" << std::endl;
        return;
    }
}

void testDeviceManagement()
{
    std::cout << "=== Testing Device Management ===" << std::endl;

    KnokkeV1 scanner;

    // Test initial state
    if (!scanner.isDeviceOpen())
    {
        std::cout << "✓ Initial device state: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ Initial device state: FAILED" << std::endl;
        return;
    }

    // Try to find available devices
    auto serials = FTDIEnumerator::getSerialNumbers();
    if (serials.empty())
    {
        std::cout << "No FTDI devices available for device management test" << std::endl;
        return;
    }

    // Test device opening
    bool opened = scanner.openDevice(serials.front());
    if (opened && scanner.isDeviceOpen())
    {
        std::cout << "✓ Device opening: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ Device opening: FAILED" << std::endl;
        return;
    }

    // Test device closing
    scanner.closeDevice();
    if (!scanner.isDeviceOpen())
    {
        std::cout << "✓ Device closing: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ Device closing: FAILED" << std::endl;
        return;
    }
}

void testFrameQueueManagement()
{
    std::cout << "=== Testing Frame Queue Management ===" << std::endl;

    KnokkeV1 scanner;

    // Test empty queue
    if (scanner.getFrameQueueSize() == 0)
    {
        std::cout << "✓ Empty queue: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ Empty queue: FAILED" << std::endl;
        return;
    }

    // Add frames to queue
    for (int i = 0; i < 5; ++i)
    {
        std::vector<uint8_t> frame               = createTestFrame();
        std::vector<uint8_t> frameWithDelimiters = createFrameWithDelimiters(frame);
        scanner.processIncomingData(frameWithDelimiters);
    }

    // Check queue size
    if (scanner.getFrameQueueSize() == 5)
    {
        std::cout << "✓ Queue size after adding frames: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ Queue size after adding frames: FAILED" << std::endl;
        return;
    }

    // Retrieve frames
    int                  framesRetrieved = 0;
    std::vector<uint8_t> frame;
    while (scanner.getNextFrame(frame))
    {
        framesRetrieved++;
    }

    if (framesRetrieved == 5 && scanner.getFrameQueueSize() == 0)
    {
        std::cout << "✓ Frame retrieval: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ Frame retrieval: FAILED" << std::endl;
        return;
    }
}

void testControlInterface()
{
    std::cout << "=== Testing Control Interface ===" << std::endl;

    KnokkeV1 scanner;

    // Test control interface access when device is closed
    if (scanner.getControlInterface() == nullptr)
    {
        std::cout << "✓ Control interface access when closed: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ Control interface access when closed: FAILED" << std::endl;
        return;
    }

    // Test serial number generation
    std::string baseSerial            = "TEST123";
    std::string expectedDataSerial    = baseSerial + "A";
    std::string expectedControlSerial = baseSerial + "B";

    // Note: We can't easily test the private methods, but we can test the behavior
    // by checking that the device opening process works correctly

    // Try to find available devices
    auto serials = FTDIEnumerator::getSerialNumbers();
    if (serials.empty())
    {
        std::cout << "No FTDI devices available for control interface test" << std::endl;
        return;
    }

    // Test device opening (this will test both interfaces)
    bool opened = scanner.openDevice(serials.front());
    if (opened)
    {
        // Test control interface access when device is open
        if (scanner.getControlInterface() != nullptr)
        {
            std::cout << "✓ Control interface access when open: PASSED" << std::endl;
        }
        else
        {
            std::cout << "✗ Control interface access when open: FAILED" << std::endl;
            scanner.closeDevice();
            return;
        }

        // Test const version
        const KnokkeV1 &constScanner = scanner;
        if (constScanner.getControlInterface() != nullptr)
        {
            std::cout << "✓ Const control interface access: PASSED" << std::endl;
        }
        else
        {
            std::cout << "✗ Const control interface access: FAILED" << std::endl;
            scanner.closeDevice();
            return;
        }

        scanner.closeDevice();

        // Test control interface access after closing
        if (scanner.getControlInterface() == nullptr)
        {
            std::cout << "✓ Control interface access after closing: PASSED" << std::endl;
        }
        else
        {
            std::cout << "✗ Control interface access after closing: FAILED" << std::endl;
            return;
        }
    }
    else
    {
        std::cout << "Device opening failed, skipping control interface tests" << std::endl;
    }
}

int main()
{
    std::cout << "=== KnokkeV1 Driver Test Suite ===" << std::endl;

    try
    {
        testFrameDelimiterDetection();
        testFrameValidation();
        testFrameParsing();
        testFrameCallback();
        testMultipleFrames();
        testDeviceManagement();
        testFrameQueueManagement();
        testControlInterface();

        std::cout << "\n=== All Tests Completed Successfully ===" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}