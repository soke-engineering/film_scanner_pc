#pragma once

#include "FTD2XXWrapper.h"
#include "slice.h"

#include <atomic>
#include <functional>
#include <mutex>
#include <queue>
#include <stdint.h>
#include <thread>
#include <vector>

class KnokkeV1
{
  public:
    // Frame dimensions
    static constexpr size_t FRAME_WIDTH      = 3840;
    static constexpr size_t FRAME_HEIGHT     = 12;
    static constexpr size_t FRAME_SIZE_BYTES = FRAME_WIDTH * FRAME_HEIGHT;
    static constexpr size_t FRAME_SIZE_WORDS = FRAME_WIDTH * FRAME_HEIGHT; // 16-bit words

    // Frame delimiters
    static constexpr uint8_t START_DELIM_0 = 0xFF;
    static constexpr uint8_t START_DELIM_1 = 0x01;
    static constexpr uint8_t START_DELIM_2 = 0xB1;
    static constexpr uint8_t START_DELIM_3 = 0x6B;

    static constexpr uint8_t END_DELIM_0 = 0xFF;
    static constexpr uint8_t END_DELIM_1 = 0x01;
    static constexpr uint8_t END_DELIM_2 = 0x01;
    static constexpr uint8_t END_DELIM_3 = 0xB5;

    // Callback types for frame notifications
    using FrameCallback      = std::function<void(const std::vector<uint8_t> &frameData)>;
    using Frame16BitCallback = std::function<void(const std::vector<uint16_t> &frameData)>;

    KnokkeV1(void);
    ~KnokkeV1(void);

    // Device management
    bool openDevice(const std::string &serial);
    void closeDevice();
    bool isDeviceOpen() const;

    // Control interface access
    FTD2XXWrapper       *getControlInterface();
    const FTD2XXWrapper *getControlInterface() const;

    // Frame processing
    void   registerFrameCallback(FrameCallback callback);
    void   registerFrame16BitCallback(Frame16BitCallback callback);
    bool   getNextFrame(std::vector<uint8_t> &frameData);
    bool   getNextFrame16Bit(std::vector<uint16_t> &frameData);
    size_t getFrameQueueSize() const;
    size_t getFrame16BitQueueSize() const;

    // Data conversion utilities
    static std::vector<uint16_t> convert8BitTo16Bit(const std::vector<uint8_t> &frame8Bit);
    static std::vector<uint8_t>  convert16BitTo8Bit(const std::vector<uint16_t> &frame16Bit);

    // Statistics
    size_t getTotalFramesReceived() const;
    size_t getTotalBytesProcessed() const;
    void   resetStatistics();

    // Testing methods (public for unit testing)
    void processIncomingData(const std::vector<uint8_t> &data);
    bool findFrameDelimiter(const std::vector<uint8_t> &data,
                            size_t                      startPos,
                            const std::vector<uint8_t> &delimiter,
                            size_t                     &foundPos);
    bool validateFrame(const std::vector<uint8_t> &frameData);

  private:
    // Frame parsing state
    enum class ParseState
    {
        WAITING_FOR_START,
        READING_FRAME_DATA,
        WAITING_FOR_END
    };

    // Data callback for FTD2XXWrapper
    void onDataReceived(size_t bytesAdded);

    // Helper methods for device management
    std::string getDataInterfaceSerial(const std::string &baseSerial) const;
    std::string getControlInterfaceSerial(const std::string &baseSerial) const;

    // Member variables
    FTD2XXWrapper     m_dataWrapper;    // Interface A - Image data
    FTD2XXWrapper     m_controlWrapper; // Interface B - Control
    std::atomic<bool> m_deviceOpen{false};

    // Frame processing
    std::vector<uint8_t> m_frameBuffer;
    ParseState           m_parseState{ParseState::WAITING_FOR_START};
    size_t               m_frameDataIndex{0};

    // Frame queues
    std::queue<std::vector<uint8_t>>  m_frameQueue;
    std::queue<std::vector<uint16_t>> m_frame16BitQueue;
    mutable std::mutex                m_frameQueueMutex;
    mutable std::mutex                m_frame16BitQueueMutex;
    std::condition_variable           m_frameCondition;

    // Callbacks
    FrameCallback      m_frameCallback;
    Frame16BitCallback m_frame16BitCallback;
    std::mutex         m_callbackMutex;

    // Statistics
    std::atomic<size_t> m_totalFramesReceived{0};
    std::atomic<size_t> m_totalBytesProcessed{0};
    std::atomic<size_t> m_framesWithErrors{0};
};
