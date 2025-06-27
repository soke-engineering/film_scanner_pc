#include "knokke_v1.h"
#include <cassert>
#include <iostream>
#include <vector>

// Test helper functions
std::vector<uint8_t> createTestFrame8Bit()
{
    std::vector<uint8_t> frame(KnokkeV1::FRAME_SIZE_BYTES);
    for (size_t i = 0; i < frame.size(); ++i)
    {
        frame[i] = static_cast<uint8_t>(i % 256);
    }
    return frame;
}

std::vector<uint16_t> createTestFrame16Bit()
{
    std::vector<uint16_t> frame(KnokkeV1::FRAME_SIZE_WORDS);
    for (size_t i = 0; i < frame.size(); ++i)
    {
        // Create 16-bit values with 8-bit data in upper byte
        frame[i] = static_cast<uint16_t>((i % 256) << 8);
    }
    return frame;
}

void testDataConversion()
{
    std::cout << "=== Testing Data Conversion ===" << std::endl;

    // Test 8-bit to 16-bit conversion
    std::vector<uint8_t>  test8Bit       = {0x00, 0x7F, 0xFF, 0x80, 0x01};
    std::vector<uint16_t> converted16Bit = KnokkeV1::convert8BitTo16Bit(test8Bit);

    if (converted16Bit.size() == test8Bit.size())
    {
        std::cout << "✓ 8-bit to 16-bit conversion size: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ 8-bit to 16-bit conversion size: FAILED" << std::endl;
        return;
    }

    // Check specific values
    bool conversionCorrect = true;
    if (converted16Bit[0] != 0x0000)
        conversionCorrect = false; // 0x00 -> 0x0000
    if (converted16Bit[1] != 0x7F00)
        conversionCorrect = false; // 0x7F -> 0x7F00
    if (converted16Bit[2] != 0xFF00)
        conversionCorrect = false; // 0xFF -> 0xFF00
    if (converted16Bit[3] != 0x8000)
        conversionCorrect = false; // 0x80 -> 0x8000
    if (converted16Bit[4] != 0x0100)
        conversionCorrect = false; // 0x01 -> 0x0100

    if (conversionCorrect)
    {
        std::cout << "✓ 8-bit to 16-bit conversion values: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ 8-bit to 16-bit conversion values: FAILED" << std::endl;
        return;
    }

    // Test 16-bit to 8-bit conversion
    std::vector<uint8_t> convertedBack8Bit = KnokkeV1::convert16BitTo8Bit(converted16Bit);

    if (convertedBack8Bit.size() == test8Bit.size())
    {
        std::cout << "✓ 16-bit to 8-bit conversion size: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ 16-bit to 8-bit conversion size: FAILED" << std::endl;
        return;
    }

    // Check that conversion back matches original
    bool roundTripCorrect = (convertedBack8Bit == test8Bit);
    if (roundTripCorrect)
    {
        std::cout << "✓ Round-trip conversion: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ Round-trip conversion: FAILED" << std::endl;
        return;
    }

    // Test with full frame size
    std::vector<uint8_t>  fullFrame8Bit     = createTestFrame8Bit();
    std::vector<uint16_t> fullFrame16Bit    = KnokkeV1::convert8BitTo16Bit(fullFrame8Bit);
    std::vector<uint8_t>  fullFrameBack8Bit = KnokkeV1::convert16BitTo8Bit(fullFrame16Bit);

    if (fullFrame16Bit.size() == KnokkeV1::FRAME_SIZE_WORDS &&
        fullFrameBack8Bit.size() == KnokkeV1::FRAME_SIZE_BYTES &&
        fullFrameBack8Bit == fullFrame8Bit)
    {
        std::cout << "✓ Full frame conversion: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ Full frame conversion: FAILED" << std::endl;
        return;
    }
}

void test16BitFrameParsing()
{
    std::cout << "=== Testing 16-bit Frame Parsing ===" << std::endl;

    KnokkeV1 scanner;

    // Create test frame with delimiters
    std::vector<uint8_t> originalFrame = createTestFrame8Bit();
    std::vector<uint8_t> frameWithDelimiters;

    // Add start delimiter
    frameWithDelimiters.push_back(KnokkeV1::START_DELIM_0);
    frameWithDelimiters.push_back(KnokkeV1::START_DELIM_1);
    frameWithDelimiters.push_back(KnokkeV1::START_DELIM_2);
    frameWithDelimiters.push_back(KnokkeV1::START_DELIM_3);

    // Add frame data
    frameWithDelimiters.insert(
        frameWithDelimiters.end(), originalFrame.begin(), originalFrame.end());

    // Add end delimiter
    frameWithDelimiters.push_back(KnokkeV1::END_DELIM_0);
    frameWithDelimiters.push_back(KnokkeV1::END_DELIM_1);
    frameWithDelimiters.push_back(KnokkeV1::END_DELIM_2);
    frameWithDelimiters.push_back(KnokkeV1::END_DELIM_3);

    // Process the frame data
    scanner.processIncomingData(frameWithDelimiters);

    // Check if 16-bit frame was parsed and queued
    std::vector<uint16_t> received16BitFrame;
    bool                  frame16BitReceived = scanner.getNextFrame16Bit(received16BitFrame);

    if (frame16BitReceived && received16BitFrame.size() == KnokkeV1::FRAME_SIZE_WORDS)
    {
        // Verify frame data matches expected conversion
        std::vector<uint16_t> expected16BitFrame = KnokkeV1::convert8BitTo16Bit(originalFrame);
        bool                  dataMatches        = (received16BitFrame == expected16BitFrame);

        if (dataMatches)
        {
            std::cout << "✓ 16-bit frame parsing: PASSED" << std::endl;
        }
        else
        {
            std::cout << "✗ 16-bit frame parsing: FAILED (data mismatch)" << std::endl;
            return;
        }
    }
    else
    {
        std::cout << "✗ 16-bit frame parsing: FAILED (no frame received)" << std::endl;
        return;
    }

    // Test statistics
    if (scanner.getTotalFramesReceived() == 1)
    {
        std::cout << "✓ 16-bit frame statistics: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ 16-bit frame statistics: FAILED" << std::endl;
        return;
    }
}

void test16BitFrameCallback()
{
    std::cout << "=== Testing 16-bit Frame Callback ===" << std::endl;

    KnokkeV1              scanner;
    std::atomic<bool>     callbackCalled{false};
    std::vector<uint16_t> callbackFrame;

    // Register 16-bit callback
    scanner.registerFrame16BitCallback(
        [&callbackCalled, &callbackFrame](const std::vector<uint16_t> &frameData)
        {
            callbackCalled.store(true);
            callbackFrame = frameData;
        });

    // Create and process test frame
    std::vector<uint8_t> originalFrame = createTestFrame8Bit();
    std::vector<uint8_t> frameWithDelimiters;

    // Add start delimiter
    frameWithDelimiters.push_back(KnokkeV1::START_DELIM_0);
    frameWithDelimiters.push_back(KnokkeV1::START_DELIM_1);
    frameWithDelimiters.push_back(KnokkeV1::START_DELIM_2);
    frameWithDelimiters.push_back(KnokkeV1::START_DELIM_3);

    // Add frame data
    frameWithDelimiters.insert(
        frameWithDelimiters.end(), originalFrame.begin(), originalFrame.end());

    // Add end delimiter
    frameWithDelimiters.push_back(KnokkeV1::END_DELIM_0);
    frameWithDelimiters.push_back(KnokkeV1::END_DELIM_1);
    frameWithDelimiters.push_back(KnokkeV1::END_DELIM_2);
    frameWithDelimiters.push_back(KnokkeV1::END_DELIM_3);

    scanner.processIncomingData(frameWithDelimiters);

    // Wait a bit for callback to be called
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::vector<uint16_t> expected16BitFrame = KnokkeV1::convert8BitTo16Bit(originalFrame);

    if (callbackCalled.load() && callbackFrame == expected16BitFrame)
    {
        std::cout << "✓ 16-bit frame callback: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ 16-bit frame callback: FAILED" << std::endl;
        return;
    }
}

void testDualQueues()
{
    std::cout << "=== Testing Dual Queues ===" << std::endl;

    KnokkeV1 scanner;

    // Test empty queues
    if (scanner.getFrameQueueSize() == 0 && scanner.getFrame16BitQueueSize() == 0)
    {
        std::cout << "✓ Empty queues: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ Empty queues: FAILED" << std::endl;
        return;
    }

    // Add frames to both queues
    for (int i = 0; i < 3; ++i)
    {
        std::vector<uint8_t> frame = createTestFrame8Bit();
        std::vector<uint8_t> frameWithDelimiters;

        // Add start delimiter
        frameWithDelimiters.push_back(KnokkeV1::START_DELIM_0);
        frameWithDelimiters.push_back(KnokkeV1::START_DELIM_1);
        frameWithDelimiters.push_back(KnokkeV1::START_DELIM_2);
        frameWithDelimiters.push_back(KnokkeV1::START_DELIM_3);

        // Add frame data
        frameWithDelimiters.insert(frameWithDelimiters.end(), frame.begin(), frame.end());

        // Add end delimiter
        frameWithDelimiters.push_back(KnokkeV1::END_DELIM_0);
        frameWithDelimiters.push_back(KnokkeV1::END_DELIM_1);
        frameWithDelimiters.push_back(KnokkeV1::END_DELIM_2);
        frameWithDelimiters.push_back(KnokkeV1::END_DELIM_3);

        scanner.processIncomingData(frameWithDelimiters);
    }

    // Check queue sizes
    if (scanner.getFrameQueueSize() == 3 && scanner.getFrame16BitQueueSize() == 3)
    {
        std::cout << "✓ Queue sizes after adding frames: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ Queue sizes after adding frames: FAILED" << std::endl;
        return;
    }

    // Retrieve frames from both queues
    int frames8BitRetrieved  = 0;
    int frames16BitRetrieved = 0;

    std::vector<uint8_t> frame8Bit;
    while (scanner.getNextFrame(frame8Bit))
    {
        frames8BitRetrieved++;
    }

    std::vector<uint16_t> frame16Bit;
    while (scanner.getNextFrame16Bit(frame16Bit))
    {
        frames16BitRetrieved++;
    }

    if (frames8BitRetrieved == 3 && frames16BitRetrieved == 3 && scanner.getFrameQueueSize() == 0 &&
        scanner.getFrame16BitQueueSize() == 0)
    {
        std::cout << "✓ Frame retrieval from both queues: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ Frame retrieval from both queues: FAILED" << std::endl;
        return;
    }
}

void testSimultaneousCallbacks()
{
    std::cout << "=== Testing Simultaneous Callbacks ===" << std::endl;

    KnokkeV1         scanner;
    std::atomic<int> callback8BitCount{0};
    std::atomic<int> callback16BitCount{0};

    // Register both callbacks
    scanner.registerFrameCallback([&callback8BitCount](const std::vector<uint8_t> &frameData)
                                  { callback8BitCount.fetch_add(1); });

    scanner.registerFrame16BitCallback([&callback16BitCount](const std::vector<uint16_t> &frameData)
                                       { callback16BitCount.fetch_add(1); });

    // Create and process test frame
    std::vector<uint8_t> originalFrame = createTestFrame8Bit();
    std::vector<uint8_t> frameWithDelimiters;

    // Add start delimiter
    frameWithDelimiters.push_back(KnokkeV1::START_DELIM_0);
    frameWithDelimiters.push_back(KnokkeV1::START_DELIM_1);
    frameWithDelimiters.push_back(KnokkeV1::START_DELIM_2);
    frameWithDelimiters.push_back(KnokkeV1::START_DELIM_3);

    // Add frame data
    frameWithDelimiters.insert(
        frameWithDelimiters.end(), originalFrame.begin(), originalFrame.end());

    // Add end delimiter
    frameWithDelimiters.push_back(KnokkeV1::END_DELIM_0);
    frameWithDelimiters.push_back(KnokkeV1::END_DELIM_1);
    frameWithDelimiters.push_back(KnokkeV1::END_DELIM_2);
    frameWithDelimiters.push_back(KnokkeV1::END_DELIM_3);

    scanner.processIncomingData(frameWithDelimiters);

    // Wait a bit for callbacks to be called
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (callback8BitCount.load() == 1 && callback16BitCount.load() == 1)
    {
        std::cout << "✓ Simultaneous callbacks: PASSED" << std::endl;
    }
    else
    {
        std::cout << "✗ Simultaneous callbacks: FAILED" << std::endl;
        return;
    }
}

int main()
{
    std::cout << "=== KnokkeV1 16-bit Functionality Test Suite ===" << std::endl;

    try
    {
        testDataConversion();
        test16BitFrameParsing();
        test16BitFrameCallback();
        testDualQueues();
        testSimultaneousCallbacks();

        std::cout << "\n=== All 16-bit Tests Completed Successfully ===" << std::endl;
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