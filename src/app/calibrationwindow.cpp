#include "calibrationwindow.h"
#include "../drivers/scanners/Knokke.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

CalibrationWindow::CalibrationWindow(QWidget *parent)
    : QWidget(parent), m_previewLabel(nullptr), m_previewTimer(nullptr),
      m_knokke(std::make_unique<Knokke>()), m_frameCounter(0), m_redSlider(nullptr),
      m_greenSlider(nullptr), m_blueSlider(nullptr), m_redValueLabel(nullptr),
      m_greenValueLabel(nullptr), m_blueValueLabel(nullptr), m_exposureSlider(nullptr),
      m_gainSlider(nullptr), m_exposureValueLabel(nullptr), m_gainValueLabel(nullptr),
      m_redMinLabel(nullptr), m_redMaxLabel(nullptr), m_redAvgLabel(nullptr),
      m_greenMinLabel(nullptr), m_greenMaxLabel(nullptr), m_greenAvgLabel(nullptr),
      m_blueMinLabel(nullptr), m_blueMaxLabel(nullptr), m_blueAvgLabel(nullptr),
      m_saveButton(nullptr), m_motorLeftButton(nullptr), m_motorRightButton(nullptr),
      m_sharpnessLabel(nullptr), m_sliderUpdateTimer(nullptr), m_sliderUpdatePending(false),
      m_pendingExposure(100), m_pendingGain(0)
{
    setWindowTitle("Scanner Calibration Preview");
    setFixedSize(1920, 400); // Increased height to accommodate all controls including motor buttons

    // Ensure it opens as a separate window
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint |
                   Qt::WindowMinimizeButtonHint);
    setAttribute(Qt::WA_DeleteOnClose, true);

    setupUI();

    // Setup debouncing timer for slider updates
    m_sliderUpdateTimer = new QTimer(this);
    m_sliderUpdateTimer->setSingleShot(true);
    m_sliderUpdateTimer->setInterval(100); // 100ms debounce
    connect(m_sliderUpdateTimer, &QTimer::timeout, this, &CalibrationWindow::onSliderUpdateTimeout);

    // Defer scanner setup until window is shown
    QTimer::singleShot(100, this, &CalibrationWindow::setupScanner);
}

CalibrationWindow::~CalibrationWindow() { stopPreview(); }

void CalibrationWindow::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(10); // Add spacing between all widgets

    // Preview label
    m_previewLabel = new QLabel(this);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet("QLabel { border: 1px solid gray; background-color: black; }");
    m_previewLabel->setFixedSize(1900, 120); // Fixed size to prevent overlap
    m_previewLabel->setText("Initializing scanner preview...");
    m_previewLabel->setStyleSheet("QLabel { color: white; font-size: 14px; }");

    layout->addWidget(m_previewLabel);

    // Backlight control sliders
    QHBoxLayout *sliderLayout = new QHBoxLayout();

    // Red slider
    QLabel *redLabel = new QLabel("R:", this);
    redLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
    m_redSlider = new QSlider(Qt::Horizontal, this);
    m_redSlider->setRange(0, 65535);
    m_redSlider->setValue(0);
    m_redValueLabel = new QLabel("0", this);
    m_redValueLabel->setStyleSheet("QLabel { color: red; font-weight: bold; min-width: 60px; }");
    connect(m_redSlider, &QSlider::valueChanged, this, &CalibrationWindow::onRedSliderChanged);

    // Green slider
    QLabel *greenLabel = new QLabel("G:", this);
    greenLabel->setStyleSheet("QLabel { color: green; font-weight: bold; }");
    m_greenSlider = new QSlider(Qt::Horizontal, this);
    m_greenSlider->setRange(0, 65535);
    m_greenSlider->setValue(0);
    m_greenValueLabel = new QLabel("0", this);
    m_greenValueLabel->setStyleSheet(
        "QLabel { color: green; font-weight: bold; min-width: 60px; }");
    connect(m_greenSlider, &QSlider::valueChanged, this, &CalibrationWindow::onGreenSliderChanged);

    // Blue slider
    QLabel *blueLabel = new QLabel("B:", this);
    blueLabel->setStyleSheet("QLabel { color: blue; font-weight: bold; }");
    m_blueSlider = new QSlider(Qt::Horizontal, this);
    m_blueSlider->setRange(0, 65535);
    m_blueSlider->setValue(0);
    m_blueValueLabel = new QLabel("0", this);
    m_blueValueLabel->setStyleSheet("QLabel { color: blue; font-weight: bold; min-width: 60px; }");
    connect(m_blueSlider, &QSlider::valueChanged, this, &CalibrationWindow::onBlueSliderChanged);

    sliderLayout->addWidget(redLabel);
    sliderLayout->addWidget(m_redSlider);
    sliderLayout->addWidget(m_redValueLabel);
    sliderLayout->addWidget(greenLabel);
    sliderLayout->addWidget(m_greenSlider);
    sliderLayout->addWidget(m_greenValueLabel);
    sliderLayout->addWidget(blueLabel);
    sliderLayout->addWidget(m_blueSlider);
    sliderLayout->addWidget(m_blueValueLabel);

    // Exposure and gain control sliders
    QHBoxLayout *exposureGainLayout = new QHBoxLayout();

    // Exposure slider
    QLabel *exposureLabel = new QLabel("Exposure:", this);
    exposureLabel->setStyleSheet("QLabel { color: orange; font-weight: bold; }");
    m_exposureSlider = new QSlider(Qt::Horizontal, this);
    m_exposureSlider->setRange(100, 2000); // 100us to 2000us
    m_exposureSlider->setValue(100);
    m_exposureValueLabel = new QLabel("100μs", this);
    m_exposureValueLabel->setStyleSheet(
        "QLabel { color: orange; font-weight: bold; min-width: 80px; }");
    connect(m_exposureSlider,
            &QSlider::valueChanged,
            this,
            &CalibrationWindow::onExposureSliderChanged);

    // Gain slider
    QLabel *gainLabel = new QLabel("Gain:", this);
    gainLabel->setStyleSheet("QLabel { color: purple; font-weight: bold; }");
    m_gainSlider = new QSlider(Qt::Horizontal, this);
    m_gainSlider->setRange(0, 30); // 0 to 30 dB
    m_gainSlider->setValue(0);
    m_gainValueLabel = new QLabel("0dB", this);
    m_gainValueLabel->setStyleSheet(
        "QLabel { color: purple; font-weight: bold; min-width: 40px; }");
    connect(m_gainSlider, &QSlider::valueChanged, this, &CalibrationWindow::onGainSliderChanged);

    exposureGainLayout->addWidget(exposureLabel);
    exposureGainLayout->addWidget(m_exposureSlider);
    exposureGainLayout->addWidget(m_exposureValueLabel);
    exposureGainLayout->addWidget(gainLabel);
    exposureGainLayout->addWidget(m_gainSlider);
    exposureGainLayout->addWidget(m_gainValueLabel);

    // Add both slider layouts below the preview
    layout->addLayout(sliderLayout);
    layout->addLayout(exposureGainLayout);

    // RGB channel min/max values display
    QHBoxLayout *rgbMinMaxLayout = new QHBoxLayout();

    // Red channel min/max
    QLabel *redMinLabel = new QLabel("R Min:", this);
    redMinLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
    m_redMinLabel = new QLabel("0", this);
    m_redMinLabel->setStyleSheet("QLabel { color: red; font-weight: bold; min-width: 50px; }");

    QLabel *redMaxLabel = new QLabel("R Max:", this);
    redMaxLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
    m_redMaxLabel = new QLabel("0", this);
    m_redMaxLabel->setStyleSheet("QLabel { color: red; font-weight: bold; min-width: 50px; }");

    // Green channel min/max
    QLabel *greenMinLabel = new QLabel("G Min:", this);
    greenMinLabel->setStyleSheet("QLabel { color: green; font-weight: bold; }");
    m_greenMinLabel = new QLabel("0", this);
    m_greenMinLabel->setStyleSheet("QLabel { color: green; font-weight: bold; min-width: 50px; }");

    QLabel *greenMaxLabel = new QLabel("G Max:", this);
    greenMaxLabel->setStyleSheet("QLabel { color: green; font-weight: bold; }");
    m_greenMaxLabel = new QLabel("0", this);
    m_greenMaxLabel->setStyleSheet("QLabel { color: green; font-weight: bold; min-width: 50px; }");

    // Blue channel min/max
    QLabel *blueMinLabel = new QLabel("B Min:", this);
    blueMinLabel->setStyleSheet("QLabel { color: blue; font-weight: bold; }");
    m_blueMinLabel = new QLabel("0", this);
    m_blueMinLabel->setStyleSheet("QLabel { color: blue; font-weight: bold; min-width: 50px; }");

    QLabel *blueMaxLabel = new QLabel("B Max:", this);
    blueMaxLabel->setStyleSheet("QLabel { color: blue; font-weight: bold; }");
    m_blueMaxLabel = new QLabel("0", this);
    m_blueMaxLabel->setStyleSheet("QLabel { color: blue; font-weight: bold; min-width: 50px; }");

    // Red channel average
    QLabel *redAvgLabel = new QLabel("R Avg:", this);
    redAvgLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
    m_redAvgLabel = new QLabel("0", this);
    m_redAvgLabel->setStyleSheet("QLabel { color: red; font-weight: bold; min-width: 50px; }");

    // Green channel average
    QLabel *greenAvgLabel = new QLabel("G Avg:", this);
    greenAvgLabel->setStyleSheet("QLabel { color: green; font-weight: bold; }");
    m_greenAvgLabel = new QLabel("0", this);
    m_greenAvgLabel->setStyleSheet("QLabel { color: green; font-weight: bold; min-width: 50px; }");

    // Blue channel average
    QLabel *blueAvgLabel = new QLabel("B Avg:", this);
    blueAvgLabel->setStyleSheet("QLabel { color: blue; font-weight: bold; }");
    m_blueAvgLabel = new QLabel("0", this);
    m_blueAvgLabel->setStyleSheet("QLabel { color: blue; font-weight: bold; min-width: 50px; }");

    rgbMinMaxLayout->addWidget(redMinLabel);
    rgbMinMaxLayout->addWidget(m_redMinLabel);
    rgbMinMaxLayout->addWidget(redMaxLabel);
    rgbMinMaxLayout->addWidget(m_redMaxLabel);
    rgbMinMaxLayout->addWidget(redAvgLabel);
    rgbMinMaxLayout->addWidget(m_redAvgLabel);
    rgbMinMaxLayout->addSpacing(20);
    rgbMinMaxLayout->addWidget(greenMinLabel);
    rgbMinMaxLayout->addWidget(m_greenMinLabel);
    rgbMinMaxLayout->addWidget(greenMaxLabel);
    rgbMinMaxLayout->addWidget(m_greenMaxLabel);
    rgbMinMaxLayout->addWidget(greenAvgLabel);
    rgbMinMaxLayout->addWidget(m_greenAvgLabel);
    rgbMinMaxLayout->addSpacing(20);
    rgbMinMaxLayout->addWidget(blueMinLabel);
    rgbMinMaxLayout->addWidget(m_blueMinLabel);
    rgbMinMaxLayout->addWidget(blueMaxLabel);
    rgbMinMaxLayout->addWidget(m_blueMaxLabel);
    rgbMinMaxLayout->addWidget(blueAvgLabel);
    rgbMinMaxLayout->addWidget(m_blueAvgLabel);
    rgbMinMaxLayout->addStretch(); // Push to the left

    layout->addLayout(rgbMinMaxLayout);

    // Sharpness display
    QHBoxLayout *sharpnessLayout     = new QHBoxLayout();
    QLabel      *sharpnessTitleLabel = new QLabel("Sharpness:", this);
    sharpnessTitleLabel->setStyleSheet(
        "QLabel { color: #2c3e50; font-weight: bold; font-size: 14px; }");
    m_sharpnessLabel = new QLabel("0.00", this);
    m_sharpnessLabel->setStyleSheet(
        "QLabel { color: #e74c3c; font-weight: bold; font-size: 14px; min-width: 80px; }");

    sharpnessLayout->addStretch();
    sharpnessLayout->addWidget(sharpnessTitleLabel);
    sharpnessLayout->addWidget(m_sharpnessLabel);
    sharpnessLayout->addStretch();

    layout->addLayout(sharpnessLayout);

    // Save button
    m_saveButton = new QPushButton("Save Image", this);
    m_saveButton->setStyleSheet("QPushButton { padding: 5px 15px; font-weight: bold; }");
    connect(m_saveButton, &QPushButton::clicked, this, &CalibrationWindow::onSaveImageClicked);
    layout->addWidget(m_saveButton);

    // Motor control buttons
    QHBoxLayout *motorLayout = new QHBoxLayout();

    m_motorLeftButton = new QPushButton("← Left", this);
    m_motorLeftButton->setFixedSize(100, 50);
    connect(m_motorLeftButton, &QPushButton::pressed, this, &CalibrationWindow::onMotorLeftPressed);
    connect(
        m_motorLeftButton, &QPushButton::released, this, &CalibrationWindow::onMotorLeftReleased);

    m_motorRightButton = new QPushButton("Right →", this);
    m_motorRightButton->setFixedSize(100, 50);
    connect(
        m_motorRightButton, &QPushButton::pressed, this, &CalibrationWindow::onMotorRightPressed);
    connect(
        m_motorRightButton, &QPushButton::released, this, &CalibrationWindow::onMotorRightReleased);

    motorLayout->addStretch(); // Push buttons to center
    motorLayout->addWidget(m_motorLeftButton);
    motorLayout->addSpacing(20);
    motorLayout->addWidget(m_motorRightButton);
    motorLayout->addStretch(); // Push buttons to center

    layout->addLayout(motorLayout);

    // Status label
    QLabel *statusLabel = new QLabel("Scanner Calibration Preview - 50fps (12px height stretched "
                                     "to 120px) | RGB Backlight + Exposure + Gain + Motor Controls",
                                     this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("QLabel { font-size: 12px; color: #666; }");
    layout->addWidget(statusLabel);
}

void CalibrationWindow::setupScanner()
{
    try
    {
        // Initialize and connect to the scanner
        Knokke::Error initResult = m_knokke->initialize();
        if (initResult != Knokke::Error::SUCCESS)
        {
            m_previewLabel->setText("Failed to initialize scanner");
            return;
        }

        Knokke::Error connectResult = m_knokke->connect();
        if (connectResult != Knokke::Error::SUCCESS)
        {
            m_previewLabel->setText("Failed to connect to scanner");
            return;
        }
    }
    catch (const std::exception &e)
    {
        m_previewLabel->setText("Scanner error: " + QString::fromStdString(e.what()));
        return;
    }
    catch (...)
    {
        m_previewLabel->setText("Unknown scanner error");
        return;
    }

    // Read current scanner parameters and set sliders
    Knokke::BacklightParams currentBacklight;
    uint32_t                currentExposure;
    uint16_t                currentGain;

    Knokke::Error backlightResult = m_knokke->getBacklight(currentBacklight);
    Knokke::Error exposureResult  = m_knokke->getExposureTime(currentExposure);
    Knokke::Error gainResult      = m_knokke->getGain(currentGain);

    if (backlightResult == Knokke::Error::SUCCESS)
    {
        // Temporarily disconnect slider signals to prevent triggering scanner updates
        m_redSlider->blockSignals(true);
        m_greenSlider->blockSignals(true);
        m_blueSlider->blockSignals(true);

        // Set slider values to current backlight values
        m_redSlider->setValue(static_cast<int>(currentBacklight.red));
        m_greenSlider->setValue(static_cast<int>(currentBacklight.green));
        m_blueSlider->setValue(static_cast<int>(currentBacklight.blue));

        // Update value labels
        m_redValueLabel->setText(QString::number(currentBacklight.red));
        m_greenValueLabel->setText(QString::number(currentBacklight.green));
        m_blueValueLabel->setText(QString::number(currentBacklight.blue));

        // Initialize pending backlight values
        m_pendingBacklight = currentBacklight;

        // Re-enable slider signals
        m_redSlider->blockSignals(false);
        m_greenSlider->blockSignals(false);
        m_blueSlider->blockSignals(false);

        qDebug() << "Set sliders to current backlight values - R:" << currentBacklight.red
                 << "G:" << currentBacklight.green << "B:" << currentBacklight.blue;
    }
    else
    {
        qDebug() << "Failed to read current backlight values, using defaults";
    }

    if (exposureResult == Knokke::Error::SUCCESS)
    {
        m_exposureSlider->blockSignals(true);
        m_exposureSlider->setValue(static_cast<int>(currentExposure));
        m_exposureValueLabel->setText(QString::number(currentExposure) + "μs");
        m_pendingExposure = currentExposure;
        m_exposureSlider->blockSignals(false);
        qDebug() << "Set exposure slider to current value:" << currentExposure;
    }
    else
    {
        qDebug() << "Failed to read current exposure, using default";
    }

    if (gainResult == Knokke::Error::SUCCESS)
    {
        m_gainSlider->blockSignals(true);
        // Convert from device units (gain_db * 100) to dB for slider
        int gainDb = static_cast<int>(currentGain / 100);
        m_gainSlider->setValue(gainDb);
        m_gainValueLabel->setText(QString::number(gainDb) + "dB");
        m_pendingGain = currentGain; // Store original device value
        m_gainSlider->blockSignals(false);
        qDebug() << "Set gain slider to current value:" << currentGain << "device units (" << gainDb
                 << "dB)";
    }
    else
    {
        qDebug() << "Failed to read current gain, using default";
    }

    // Start streaming
    Knokke::Error streamResult = m_knokke->startStreaming();
    if (streamResult != Knokke::Error::SUCCESS)
    {
        m_previewLabel->setText("Failed to start streaming");
        return;
    }

    m_previewLabel->setText("Scanner connected. Starting preview...");

    // Start the preview timer
    startPreview();
}

void CalibrationWindow::startPreview()
{
    if (m_previewTimer)
    {
        return; // Already running
    }

    m_previewTimer = new QTimer(this);
    connect(m_previewTimer, &QTimer::timeout, this, &CalibrationWindow::updatePreview);
    m_previewTimer->start(PREVIEW_INTERVAL_MS);

    m_frameCounter = 0;
}

void CalibrationWindow::stopPreview()
{
    if (m_previewTimer)
    {
        m_previewTimer->stop();
        m_previewTimer->deleteLater();
        m_previewTimer = nullptr;
    }

    if (m_knokke)
    {
        m_knokke->stopStreaming();
        m_knokke->disconnect();
    }
}

void CalibrationWindow::updatePreview()
{
    if (!m_knokke || !m_knokke->isStreaming())
    {
        return;
    }

    // Drop frames to achieve 50fps from 400fps
    m_frameCounter++;
    if (m_frameCounter % FRAME_DROP_COUNT != 0)
    {
        return;
    }

    // Get the latest frame from streaming
    std::vector<uint8_t> frameData(FRAME_BYTES);
    Knokke::Error        result = m_knokke->getLatestFrame(frameData.data(), FRAME_BYTES);

    if (result != Knokke::Error::SUCCESS)
    {
        return;
    }

    // Convert raw data to OpenCV Mat (12-bit bayer data)
    // Handle little-endian 16-bit data like Python implementation
    cv::Mat   bayerMat(FRAME_HEIGHT, FRAME_WIDTH, CV_16UC1);
    uint16_t *matData = reinterpret_cast<uint16_t *>(bayerMat.data);
    for (size_t i = 0; i < FRAME_WIDTH * FRAME_HEIGHT; i++)
    {
        // Convert little-endian bytes to uint16_t
        matData[i] = static_cast<uint16_t>(frameData[i * 2]) |
                     (static_cast<uint16_t>(frameData[i * 2 + 1]) << 8);
    }

    // Convert 12-bit to 8-bit for processing (matching Python implementation)
    cv::Mat bayer8bit = cv::Mat::zeros(FRAME_HEIGHT, FRAME_WIDTH, CV_8UC1);
    for (int y = 0; y < FRAME_HEIGHT; y++)
    {
        for (int x = 0; x < FRAME_WIDTH; x++)
        {
            uint16_t pixel_value = bayerMat.at<uint16_t>(y, x);
            uint8_t  normalized_value =
                std::min(255, static_cast<int>(std::round((pixel_value / 4095.0) * 255)));
            bayer8bit.at<uint8_t>(y, x) = normalized_value;
        }
    }

    // Demosaic the bayer data first
    cv::Mat rgbImage = demosaicBayer(bayer8bit);

    // Calculate min/max/average values for each RGB channel from the demosaiced image
    uint8_t red_min = 255, red_max = 0;
    uint8_t green_min = 255, green_max = 0;
    uint8_t blue_min = 255, blue_max = 0;

    uint64_t red_sum = 0, green_sum = 0, blue_sum = 0;
    int      pixel_count = 0;

    for (int y = 0; y < rgbImage.rows; y++)
    {
        for (int x = 0; x < rgbImage.cols; x++)
        {
            cv::Vec3b pixel = rgbImage.at<cv::Vec3b>(y, x);
            uint8_t   r     = pixel[0]; // Red channel
            uint8_t   g     = pixel[1]; // Green channel
            uint8_t   b     = pixel[2]; // Blue channel

            red_min   = std::min(red_min, r);
            red_max   = std::max(red_max, r);
            green_min = std::min(green_min, g);
            green_max = std::max(green_max, g);
            blue_min  = std::min(blue_min, b);
            blue_max  = std::max(blue_max, b);

            red_sum += r;
            green_sum += g;
            blue_sum += b;
            pixel_count++;
        }
    }

    // Calculate averages
    uint8_t red_avg   = pixel_count > 0 ? static_cast<uint8_t>(red_sum / pixel_count) : 0;
    uint8_t green_avg = pixel_count > 0 ? static_cast<uint8_t>(green_sum / pixel_count) : 0;
    uint8_t blue_avg  = pixel_count > 0 ? static_cast<uint8_t>(blue_sum / pixel_count) : 0;

    // Update RGB channel min/max/average value labels
    m_redMinLabel->setText(QString::number(red_min));
    m_redMaxLabel->setText(QString::number(red_max));
    m_redAvgLabel->setText(QString::number(red_avg));
    m_greenMinLabel->setText(QString::number(green_min));
    m_greenMaxLabel->setText(QString::number(green_max));
    m_greenAvgLabel->setText(QString::number(green_avg));
    m_blueMinLabel->setText(QString::number(blue_min));
    m_blueMaxLabel->setText(QString::number(blue_max));
    m_blueAvgLabel->setText(QString::number(blue_avg));

    // Calculate and display sharpness
    double sharpness = calculateSharpness(rgbImage);
    m_sharpnessLabel->setText(QString::number(sharpness, 'f', 2));

    // Debug output (only on first frame)
    static bool debug_printed = false;
    if (!debug_printed)
    {
        qDebug() << "RGB channel ranges - R:" << red_min << "-" << red_max << " (avg:" << red_avg
                 << ") G:" << green_min << "-" << green_max << " (avg:" << green_avg
                 << ") B:" << blue_min << "-" << blue_max << " (avg:" << blue_avg
                 << ") Sharpness:" << sharpness;
        debug_printed = true;
    }

    // Store the last frame for saving
    m_lastFrame = rgbImage.clone();

    // Convert to QImage and display
    QImage  qImage = matToQImage(rgbImage);
    QPixmap pixmap = QPixmap::fromImage(qImage);

    // Scale width to fit and stretch height from 12px to 120px (10x stretch)
    QPixmap scaledPixmap = pixmap.scaled(
        m_previewLabel->width(), 120, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    m_previewLabel->setPixmap(scaledPixmap);
}

cv::Mat CalibrationWindow::demosaicBayer(const cv::Mat &bayerData)
{
    // GRBG Bayer pattern demosaicing (confirmed correct)
    int height = bayerData.rows;
    int width  = bayerData.cols;

    cv::Mat rgbImage(height, width, CV_8UC3);

    // Extract color channels using GRBG pattern
    cv::Mat rChannel = cv::Mat::zeros(height, width, CV_8UC1);
    cv::Mat gChannel = cv::Mat::zeros(height, width, CV_8UC1);
    cv::Mat bChannel = cv::Mat::zeros(height, width, CV_8UC1);

    // Create masks for each color in GRBG pattern
    for (int y = 0; y < height - 1; y += 2)
    {
        for (int x = 0; x < width - 1; x += 2)
        {
            // 2x2 block processing
            cv::Rect blockRect(x, y, 2, 2);
            cv::Mat  block = bayerData(blockRect);

            // GRBG pattern assignments
            gChannel.at<uint8_t>(y + 0, x + 0) = block.at<uint8_t>(0, 0); // G1
            rChannel.at<uint8_t>(y + 0, x + 1) = block.at<uint8_t>(0, 1); // R
            bChannel.at<uint8_t>(y + 1, x + 0) = block.at<uint8_t>(1, 0); // B
            gChannel.at<uint8_t>(y + 1, x + 1) = block.at<uint8_t>(1, 1); // G2
        }
    }

    // Interpolate missing values
    cv::Mat rInterp = interpolateChannel(rChannel);
    cv::Mat gInterp = interpolateChannel(gChannel);
    cv::Mat bInterp = interpolateChannel(bChannel);

    // Combine into RGB image (R, G, B order like Python implementation)
    std::vector<cv::Mat> channels = {rInterp, gInterp, bInterp}; // RGB order
    cv::merge(channels, rgbImage);

    return rgbImage;
}

cv::Mat CalibrationWindow::interpolateChannel(const cv::Mat &channel)
{
    cv::Mat result = channel.clone();
    int     height = channel.rows;
    int     width  = channel.cols;

    // Simple bilinear interpolation for missing pixels
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if (result.at<uint8_t>(y, x) == 0) // Missing pixel
            {
                // Collect neighboring values
                std::vector<uint8_t> neighbors;
                for (int dy = -1; dy <= 1; dy++)
                {
                    for (int dx = -1; dx <= 1; dx++)
                    {
                        int ny = y + dy;
                        int nx = x + dx;
                        if (ny >= 0 && ny < height && nx >= 0 && nx < width &&
                            (dy != 0 || dx != 0) && result.at<uint8_t>(ny, nx) > 0)
                        {
                            neighbors.push_back(result.at<uint8_t>(ny, nx));
                        }
                    }
                }

                if (!neighbors.empty())
                {
                    int sum = 0;
                    for (uint8_t val : neighbors)
                    {
                        sum += val;
                    }
                    result.at<uint8_t>(y, x) = sum / neighbors.size();
                }
            }
        }
    }

    return result;
}

QImage CalibrationWindow::matToQImage(const cv::Mat &mat)
{
    if (mat.type() == CV_8UC3)
    {
        // Mat is already in RGB order, no conversion needed
        QImage qImage(
            mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_RGB888);
        return qImage.copy(); // Make a copy to ensure data persistence
    }

    return QImage();
}

void CalibrationWindow::closeEvent(QCloseEvent *event)
{
    stopPreview();
    emit windowClosed();
    QWidget::closeEvent(event);
}

void CalibrationWindow::onRedSliderChanged(int value)
{
    // Update value label
    m_redValueLabel->setText(QString::number(value));

    if (m_knokke && m_knokke->isConnected())
    {
        // Update pending backlight values
        m_pendingBacklight.red = static_cast<uint16_t>(value);
        m_sliderUpdatePending  = true;

        // Restart debounce timer
        m_sliderUpdateTimer->start();
    }
}

void CalibrationWindow::onGreenSliderChanged(int value)
{
    // Update value label
    m_greenValueLabel->setText(QString::number(value));

    if (m_knokke && m_knokke->isConnected())
    {
        // Update pending backlight values
        m_pendingBacklight.green = static_cast<uint16_t>(value);
        m_sliderUpdatePending    = true;

        // Restart debounce timer
        m_sliderUpdateTimer->start();
    }
}

void CalibrationWindow::onBlueSliderChanged(int value)
{
    // Update value label
    m_blueValueLabel->setText(QString::number(value));

    if (m_knokke && m_knokke->isConnected())
    {
        // Update pending backlight values
        m_pendingBacklight.blue = static_cast<uint16_t>(value);
        m_sliderUpdatePending   = true;

        // Restart debounce timer
        m_sliderUpdateTimer->start();
    }
}

void CalibrationWindow::onExposureSliderChanged(int value)
{
    // Update value label
    m_exposureValueLabel->setText(QString::number(value) + "μs");

    if (m_knokke && m_knokke->isConnected())
    {
        // Update pending exposure value
        m_pendingExposure     = static_cast<uint32_t>(value);
        m_sliderUpdatePending = true;

        // Restart debounce timer
        m_sliderUpdateTimer->start();
    }
}

void CalibrationWindow::onGainSliderChanged(int value)
{
    // Update value label to show dB value
    m_gainValueLabel->setText(QString::number(value) + "dB");

    if (m_knokke && m_knokke->isConnected())
    {
        // Convert dB to device units (multiply by 100)
        m_pendingGain         = static_cast<uint16_t>(value * 100);
        m_sliderUpdatePending = true;

        // Restart debounce timer
        m_sliderUpdateTimer->start();
    }
}

void CalibrationWindow::onSliderUpdateTimeout()
{
    if (m_sliderUpdatePending && m_knokke && m_knokke->isConnected())
    {
        // Apply the pending backlight changes
        Knokke::Error backlightResult = m_knokke->setBacklight(m_pendingBacklight);
        if (backlightResult != Knokke::Error::SUCCESS)
        {
            qDebug() << "Failed to set backlight values:" << static_cast<int>(backlightResult);
        }
        else
        {
            qDebug() << "Backlight updated - R:" << m_pendingBacklight.red
                     << "G:" << m_pendingBacklight.green << "B:" << m_pendingBacklight.blue;
        }

        // Apply the pending exposure changes
        Knokke::Error exposureResult = m_knokke->setExposureTime(m_pendingExposure);
        if (exposureResult != Knokke::Error::SUCCESS)
        {
            qDebug() << "Failed to set exposure time:" << static_cast<int>(exposureResult);
        }
        else
        {
            qDebug() << "Exposure updated to:" << m_pendingExposure << "us";
        }

        // Apply the pending gain changes
        Knokke::Error gainResult = m_knokke->setGain(m_pendingGain);
        if (gainResult != Knokke::Error::SUCCESS)
        {
            qDebug() << "Failed to set gain:" << static_cast<int>(gainResult);
        }
        else
        {
            double gainDb = m_pendingGain / 100.0;
            qDebug() << "Gain updated to:" << m_pendingGain << "device units (" << gainDb << "dB)";
        }

        m_sliderUpdatePending = false;
    }
}

double CalibrationWindow::calculateSharpness(const cv::Mat &image)
{
    if (image.empty() || image.rows == 0 || image.cols == 0)
    {
        return 0.0;
    }

    // Convert to grayscale if needed
    cv::Mat gray;
    if (image.channels() == 3)
    {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    }
    else
    {
        gray = image.clone();
    }

    // Apply Laplacian filter to detect edges and high-frequency content
    cv::Mat laplacian;
    cv::Laplacian(gray, laplacian, CV_64F);

    // Calculate the variance of the Laplacian
    // Higher variance indicates more sharpness (more edges and details)
    cv::Scalar mean, stddev;
    cv::meanStdDev(laplacian, mean, stddev);

    double variance = stddev[0] * stddev[0];

    return variance;
}

void CalibrationWindow::onMotorLeftPressed()
{
    if (m_knokke && m_knokke->isConnected())
    {
        Knokke::Error result = m_knokke->setMotorSpeed(-200 * 1000); // -200 rpm * 1000
        if (result != Knokke::Error::SUCCESS)
        {
            qDebug() << "Failed to set motor speed (left):" << static_cast<int>(result);
        }
        else
        {
            qDebug() << "Motor set to -200 rpm (left)";
        }
    }
}

void CalibrationWindow::onMotorLeftReleased()
{
    if (m_knokke && m_knokke->isConnected())
    {
        Knokke::Error result = m_knokke->setMotorSpeed(0);
        if (result != Knokke::Error::SUCCESS)
        {
            qDebug() << "Failed to set motor speed (stop):" << static_cast<int>(result);
        }
        else
        {
            qDebug() << "Motor stopped (0 rpm)";
        }
    }
}

void CalibrationWindow::onMotorRightPressed()
{
    if (m_knokke && m_knokke->isConnected())
    {
        Knokke::Error result = m_knokke->setMotorSpeed(200 * 1000); // +200 rpm * 1000
        if (result != Knokke::Error::SUCCESS)
        {
            qDebug() << "Failed to set motor speed (right):" << static_cast<int>(result);
        }
        else
        {
            qDebug() << "Motor set to +200 rpm (right)";
        }
    }
}

void CalibrationWindow::onMotorRightReleased()
{
    if (m_knokke && m_knokke->isConnected())
    {
        Knokke::Error result = m_knokke->setMotorSpeed(0);
        if (result != Knokke::Error::SUCCESS)
        {
            qDebug() << "Failed to set motor speed (stop):" << static_cast<int>(result);
        }
        else
        {
            qDebug() << "Motor stopped (0 rpm)";
        }
    }
}

void CalibrationWindow::onSaveImageClicked()
{
    // Check if we have a frame to save
    if (m_lastFrame.empty())
    {
        QMessageBox::warning(
            this, "Save Image", "No frame data available to save. Start the preview first.");
        return;
    }

    // Ensure streaming is active for better frame quality
    if (m_knokke && m_knokke->isConnected() && !m_knokke->isStreaming())
    {
        Knokke::Error streamResult = m_knokke->startStreaming();
        if (streamResult != Knokke::Error::SUCCESS)
        {
            QMessageBox::warning(
                this, "Save Image", "Failed to start streaming for better frame quality.");
        }
    }

    // Use file dialog to let user choose save location
    QString defaultFilename = QString("scanner_capture_%1.png")
                                  .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QString filename =
        QFileDialog::getSaveFileName(this,
                                     "Save Scanner Image",
                                     defaultFilename,
                                     "PNG Images (*.png);;JPEG Images (*.jpg);;All Files (*)");

    if (filename.isEmpty())
    {
        return; // User cancelled
    }

    // Save the last displayed frame
    if (cv::imwrite(filename.toStdString(), m_lastFrame))
    {
        QMessageBox::information(this, "Save Image", QString("Image saved as: %1").arg(filename));
    }
    else
    {
        QMessageBox::warning(
            this,
            "Save Image",
            QString("Failed to save image file to: %1\nCheck file permissions and path.")
                .arg(filename));
    }
}
