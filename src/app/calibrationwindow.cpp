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
      m_greenSlider(nullptr), m_blueSlider(nullptr), m_saveButton(nullptr),
      m_sliderUpdateTimer(nullptr), m_sliderUpdatePending(false)
{
    setWindowTitle("Scanner Calibration Preview");
    setFixedSize(1920, 200); // Wide window with sliders for backlight control (increased height for
                             // stretched preview)

    // Ensure it opens as a separate window
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint |
                   Qt::WindowMinimizeButtonHint);
    setAttribute(Qt::WA_DeleteOnClose, true);

    setupUI();
    setupScanner();

    // Setup debouncing timer for slider updates
    m_sliderUpdateTimer = new QTimer(this);
    m_sliderUpdateTimer->setSingleShot(true);
    m_sliderUpdateTimer->setInterval(100); // 100ms debounce
    connect(m_sliderUpdateTimer, &QTimer::timeout, this, &CalibrationWindow::onSliderUpdateTimeout);
}

CalibrationWindow::~CalibrationWindow() { stopPreview(); }

void CalibrationWindow::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);

    // Preview label
    m_previewLabel = new QLabel(this);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet("QLabel { border: 1px solid gray; background-color: black; }");
    m_previewLabel->setMinimumSize(1900, 120); // Increased height to accommodate stretched preview
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
    // Use standard Qt slider styling
    connect(m_redSlider, &QSlider::valueChanged, this, &CalibrationWindow::onRedSliderChanged);

    // Green slider
    QLabel *greenLabel = new QLabel("G:", this);
    greenLabel->setStyleSheet("QLabel { color: green; font-weight: bold; }");
    m_greenSlider = new QSlider(Qt::Horizontal, this);
    m_greenSlider->setRange(0, 65535);
    m_greenSlider->setValue(0);
    // Use standard Qt slider styling
    connect(m_greenSlider, &QSlider::valueChanged, this, &CalibrationWindow::onGreenSliderChanged);

    // Blue slider
    QLabel *blueLabel = new QLabel("B:", this);
    blueLabel->setStyleSheet("QLabel { color: blue; font-weight: bold; }");
    m_blueSlider = new QSlider(Qt::Horizontal, this);
    m_blueSlider->setRange(0, 65535);
    m_blueSlider->setValue(0);
    // Use standard Qt slider styling
    connect(m_blueSlider, &QSlider::valueChanged, this, &CalibrationWindow::onBlueSliderChanged);

    sliderLayout->addWidget(redLabel);
    sliderLayout->addWidget(m_redSlider);
    sliderLayout->addWidget(greenLabel);
    sliderLayout->addWidget(m_greenSlider);
    sliderLayout->addWidget(blueLabel);
    sliderLayout->addWidget(m_blueSlider);

    layout->addLayout(sliderLayout);

    // Save button
    m_saveButton = new QPushButton("Save Image", this);
    m_saveButton->setStyleSheet("QPushButton { padding: 5px 15px; font-weight: bold; }");
    connect(m_saveButton, &QPushButton::clicked, this, &CalibrationWindow::onSaveImageClicked);
    layout->addWidget(m_saveButton);

    // Status label
    QLabel *statusLabel =
        new QLabel("Scanner Calibration Preview - 50fps (12px height stretched to 120px)", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("QLabel { font-size: 12px; color: #666; }");
    layout->addWidget(statusLabel);
}

void CalibrationWindow::setupScanner()
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

    // Read current backlight values and set sliders
    Knokke::BacklightParams currentBacklight;
    Knokke::Error           backlightResult = m_knokke->getBacklight(currentBacklight);
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

    // Debug output (only on first frame)
    static bool debug_printed = false;
    if (!debug_printed)
    {
        uint16_t min_val = 65535, max_val = 0;
        for (int y = 0; y < FRAME_HEIGHT; y++)
        {
            for (int x = 0; x < FRAME_WIDTH; x++)
            {
                uint16_t pixel_value = bayerMat.at<uint16_t>(y, x);
                min_val              = std::min(min_val, pixel_value);
                max_val              = std::max(max_val, pixel_value);
            }
        }
        qDebug() << "Raw pixel range:" << min_val << "-" << max_val;
        qDebug() << "Expected range: 0-4095 (12-bit)";
        debug_printed = true;
    }

    // Demosaic the bayer data
    cv::Mat rgbImage = demosaicBayer(bayer8bit);

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
    if (m_knokke && m_knokke->isConnected())
    {
        // Update pending backlight values
        m_pendingBacklight.blue = static_cast<uint16_t>(value);
        m_sliderUpdatePending   = true;

        // Restart debounce timer
        m_sliderUpdateTimer->start();
    }
}

void CalibrationWindow::onSliderUpdateTimeout()
{
    if (m_sliderUpdatePending && m_knokke && m_knokke->isConnected())
    {
        // Apply the pending backlight changes in a single control transfer
        Knokke::Error result = m_knokke->setBacklight(m_pendingBacklight);
        if (result != Knokke::Error::SUCCESS)
        {
            qDebug() << "Failed to set backlight values:" << static_cast<int>(result);
        }
        else
        {
            qDebug() << "Backlight updated - R:" << m_pendingBacklight.red
                     << "G:" << m_pendingBacklight.green << "B:" << m_pendingBacklight.blue;
        }
        m_sliderUpdatePending = false;
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
