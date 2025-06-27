#include "knokke_v1.h"
#include <algorithm>
#include <chrono>
#include <iostream>

KnokkeV1::KnokkeV1(void)
    : m_frameBuffer(), m_parseState(ParseState::WAITING_FOR_START), m_frameDataIndex(0)
{
    // Pre-allocate frame buffer
    m_frameBuffer.reserve(FRAME_SIZE_BYTES + 8); // Extra space for delimiters

    // Register data callback with data interface FTD2XXWrapper
    m_dataWrapper.registerCallback([this](size_t bytesAdded) { this->onDataReceived(bytesAdded); });
}

KnokkeV1::~KnokkeV1(void) { closeDevice(); }

std::string KnokkeV1::getDataInterfaceSerial(const std::string &baseSerial) const
{
    // Interface A for image data
    return baseSerial + "A";
}

std::string KnokkeV1::getControlInterfaceSerial(const std::string &baseSerial) const
{
    // Interface B for control
    return baseSerial + "B";
}

bool KnokkeV1::openDevice(const std::string &serial)
{
    if (m_deviceOpen.load())
    {
        std::cerr << "Device already open" << std::endl;
        return false;
    }

    // Generate serial numbers for both interfaces
    std::string dataSerial    = getDataInterfaceSerial(serial);
    std::string controlSerial = getControlInterfaceSerial(serial);

    std::cout << "Opening KnokkeV1 device:" << std::endl;
    std::cout << "  Data interface (A): " << dataSerial << std::endl;
    std::cout << "  Control interface (B): " << controlSerial << std::endl;

    // Open data interface
    bool dataSuccess = m_dataWrapper.openBySerial(dataSerial);
    if (!dataSuccess)
    {
        std::cerr << "Failed to open data interface with serial: " << dataSerial << std::endl;
        return false;
    }

    // Open control interface
    bool controlSuccess = m_controlWrapper.openBySerial(controlSerial);
    if (!controlSuccess)
    {
        std::cerr << "Failed to open control interface with serial: " << controlSerial << std::endl;
        // Close data interface if control interface fails
        m_dataWrapper.close();
        return false;
    }

    m_deviceOpen.store(true);
    std::cout << "KnokkeV1 device opened successfully" << std::endl;

    return true;
}

void KnokkeV1::closeDevice()
{
    if (m_deviceOpen.load())
    {
        m_dataWrapper.close();
        m_controlWrapper.close();
        m_deviceOpen.store(false);

        // Reset parsing state
        m_parseState     = ParseState::WAITING_FOR_START;
        m_frameDataIndex = 0;
        m_frameBuffer.clear();

        // Clear frame queues
        {
            std::lock_guard<std::mutex>      lock(m_frameQueueMutex);
            std::queue<std::vector<uint8_t>> empty;
            std::swap(m_frameQueue, empty);
        }
        {
            std::lock_guard<std::mutex>       lock(m_frame16BitQueueMutex);
            std::queue<std::vector<uint16_t>> empty;
            std::swap(m_frame16BitQueue, empty);
        }

        std::cout << "KnokkeV1 device closed" << std::endl;
    }
}

bool KnokkeV1::isDeviceOpen() const { return m_deviceOpen.load(); }

FTD2XXWrapper *KnokkeV1::getControlInterface()
{
    return m_deviceOpen.load() ? &m_controlWrapper : nullptr;
}

const FTD2XXWrapper *KnokkeV1::getControlInterface() const
{
    return m_deviceOpen.load() ? &m_controlWrapper : nullptr;
}

void KnokkeV1::registerFrameCallback(FrameCallback callback)
{
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_frameCallback = std::move(callback);
}

void KnokkeV1::registerFrame16BitCallback(Frame16BitCallback callback)
{
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_frame16BitCallback = std::move(callback);
}

bool KnokkeV1::getNextFrame(std::vector<uint8_t> &frameData)
{
    std::unique_lock<std::mutex> lock(m_frameQueueMutex);

    if (m_frameQueue.empty())
    {
        return false;
    }

    frameData = std::move(m_frameQueue.front());
    m_frameQueue.pop();
    return true;
}

bool KnokkeV1::getNextFrame16Bit(std::vector<uint16_t> &frameData)
{
    std::unique_lock<std::mutex> lock(m_frame16BitQueueMutex);

    if (m_frame16BitQueue.empty())
    {
        return false;
    }

    frameData = std::move(m_frame16BitQueue.front());
    m_frame16BitQueue.pop();
    return true;
}

size_t KnokkeV1::getFrameQueueSize() const
{
    std::lock_guard<std::mutex> lock(m_frameQueueMutex);
    return m_frameQueue.size();
}

size_t KnokkeV1::getFrame16BitQueueSize() const
{
    std::lock_guard<std::mutex> lock(m_frame16BitQueueMutex);
    return m_frame16BitQueue.size();
}

std::vector<uint16_t> KnokkeV1::convert8BitTo16Bit(const std::vector<uint8_t> &frame8Bit)
{
    std::vector<uint16_t> frame16Bit;
    frame16Bit.reserve(frame8Bit.size());

    for (uint8_t pixel : frame8Bit)
    {
        // Convert 8-bit to 16-bit by placing the 8-bit value in the upper byte
        // This gives us the full 0-65535 range with the 8-bit data in the upper 8 bits
        uint16_t pixel16Bit = static_cast<uint16_t>(pixel) << 8;
        frame16Bit.push_back(pixel16Bit);
    }

    return frame16Bit;
}

std::vector<uint8_t> KnokkeV1::convert16BitTo8Bit(const std::vector<uint16_t> &frame16Bit)
{
    std::vector<uint8_t> frame8Bit;
    frame8Bit.reserve(frame16Bit.size());

    for (uint16_t pixel : frame16Bit)
    {
        // Convert 16-bit back to 8-bit by taking the upper byte
        uint8_t pixel8Bit = static_cast<uint8_t>((pixel >> 8) & 0xFF);
        frame8Bit.push_back(pixel8Bit);
    }

    return frame8Bit;
}

size_t KnokkeV1::getTotalFramesReceived() const { return m_totalFramesReceived.load(); }

size_t KnokkeV1::getTotalBytesProcessed() const { return m_totalBytesProcessed.load(); }

void KnokkeV1::resetStatistics()
{
    m_totalFramesReceived.store(0);
    m_totalBytesProcessed.store(0);
    m_framesWithErrors.store(0);
}

void KnokkeV1::onDataReceived(size_t bytesAdded)
{
    (void)bytesAdded; // Suppress unused parameter warning
    std::vector<uint8_t> data;
    while (m_dataWrapper.popData(data))
    {
        m_totalBytesProcessed.fetch_add(data.size());
        processIncomingData(data);
    }
}

void KnokkeV1::processIncomingData(const std::vector<uint8_t> &data)
{
    size_t dataIndex = 0;

    while (dataIndex < data.size())
    {
        switch (m_parseState)
        {
        case ParseState::WAITING_FOR_START:
        {
            // Look for start delimiter
            size_t               startPos;
            std::vector<uint8_t> startDelim = {
                START_DELIM_0, START_DELIM_1, START_DELIM_2, START_DELIM_3};

            if (findFrameDelimiter(data, dataIndex, startDelim, startPos))
            {
                // Found start delimiter, begin reading frame data
                m_parseState     = ParseState::READING_FRAME_DATA;
                m_frameDataIndex = 0;
                m_frameBuffer.clear();
                m_frameBuffer.reserve(FRAME_SIZE_BYTES);

                // Skip past the delimiter
                dataIndex = startPos + 4;
            }
            else
            {
                // No start delimiter found, move to next byte
                dataIndex++;
            }
            break;
        }

        case ParseState::READING_FRAME_DATA:
        {
            // Read frame data until we have a complete frame or find end delimiter
            while (dataIndex < data.size() && m_frameDataIndex < FRAME_SIZE_BYTES)
            {
                m_frameBuffer.push_back(data[dataIndex]);
                m_frameDataIndex++;
                dataIndex++;
            }

            // Check if we have a complete frame
            if (m_frameDataIndex >= FRAME_SIZE_BYTES)
            {
                m_parseState = ParseState::WAITING_FOR_END;
            }
            break;
        }

        case ParseState::WAITING_FOR_END:
        {
            // Look for end delimiter
            size_t               endPos;
            std::vector<uint8_t> endDelim = {END_DELIM_0, END_DELIM_1, END_DELIM_2, END_DELIM_3};

            if (findFrameDelimiter(data, dataIndex, endDelim, endPos))
            {
                // Found end delimiter, validate and enqueue frame
                if (validateFrame(m_frameBuffer))
                {
                    // Create copies of the frame data for both queues
                    std::vector<uint8_t>  frameData(m_frameBuffer.begin(), m_frameBuffer.end());
                    std::vector<uint16_t> frame16BitData = convert8BitTo16Bit(frameData);

                    // Notify callbacks first (before moving data)
                    {
                        std::lock_guard<std::mutex> lock(m_callbackMutex);
                        if (m_frameCallback)
                        {
                            m_frameCallback(m_frameBuffer);
                        }
                        if (m_frame16BitCallback)
                        {
                            m_frame16BitCallback(frame16BitData);
                        }
                    }

                    // Add to 8-bit queue
                    {
                        std::lock_guard<std::mutex> lock(m_frameQueueMutex);
                        m_frameQueue.push(std::move(frameData));
                    }

                    // Add to 16-bit queue
                    {
                        std::lock_guard<std::mutex> lock(m_frame16BitQueueMutex);
                        m_frame16BitQueue.push(std::move(frame16BitData));
                    }

                    m_totalFramesReceived.fetch_add(1);
                    m_frameCondition.notify_one();
                }
                else
                {
                    m_framesWithErrors.fetch_add(1);
                    std::cerr << "Invalid frame received, discarding" << std::endl;
                }

                // Reset for next frame
                m_parseState     = ParseState::WAITING_FOR_START;
                m_frameDataIndex = 0;
                m_frameBuffer.clear();

                // Skip past the end delimiter
                dataIndex = endPos + 4;
            }
            else
            {
                // No end delimiter found, continue reading
                dataIndex++;
            }
            break;
        }
        }
    }
}

bool KnokkeV1::findFrameDelimiter(const std::vector<uint8_t> &data,
                                  size_t                      startPos,
                                  const std::vector<uint8_t> &delimiter,
                                  size_t                     &foundPos)
{
    if (delimiter.size() == 0 || startPos >= data.size())
    {
        return false;
    }

    for (size_t i = startPos; i <= data.size() - delimiter.size(); ++i)
    {
        bool found = true;
        for (size_t j = 0; j < delimiter.size(); ++j)
        {
            if (data[i + j] != delimiter[j])
            {
                found = false;
                break;
            }
        }
        if (found)
        {
            foundPos = i;
            return true;
        }
    }

    return false;
}

bool KnokkeV1::validateFrame(const std::vector<uint8_t> &frameData)
{
    // Basic validation: check frame size
    if (frameData.size() != FRAME_SIZE_BYTES)
    {
        std::cerr << "Frame size mismatch: expected " << FRAME_SIZE_BYTES << ", got "
                  << frameData.size() << std::endl;
        return false;
    }

    // Additional validation could be added here:
    // - Check for reasonable pixel values
    // - Check for frame checksums if available
    // - Check for frame sequence numbers if available

    return true;
}