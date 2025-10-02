#include "mainwindow.h"
#include "calibrationwindow.h"
#include "ui_mainwindow.h"

#include <QDateTime>
#include <QMessageBox>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <opencv2/opencv.hpp>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    std::cout << "MainWindow constructor started" << std::endl;
    ui->setupUi(this);
    std::cout << "UI setup completed" << std::endl;

    // Initialize calibration window reference
    m_calibrationWindow = nullptr;

    // Initialize scanner settings with defaults
    m_currentExposure       = 100;
    m_currentGain           = 0;
    m_currentRedBacklight   = 0;
    m_currentGreenBacklight = 0;
    m_currentBlueBacklight  = 0;

    setDefaults();
    std::cout << "setDefaults completed" << std::endl;
    setupThumbnailContainer();
    std::cout << "setupThumbnailContainer completed" << std::endl;
    addSampleThumbnails();
    std::cout << "addSampleThumbnails completed" << std::endl;

    // Initialize last focused thumbnail index
    m_lastFocusedThumbnailIndex = 0;
    std::cout << "MainWindow constructor completed successfully" << std::endl;
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::setDefaults(void)
{
    // Set the active tab
    ui->tabWidget->setCurrentIndex(0);

    // Scan tab
    ui->filmTypeComboBox->setCurrentIndex(0); // Default to DXN (auto)

    // Gain slider (using wAdjustSlider)
    ui->wAdjustSlider->setRange(0, 3000); // 0 to 30dB (in device units * 100)
    ui->wAdjustSlider->setValue(0);

    // RGB backlight sliders
    ui->rAdjustSlider->setRange(0, 255);
    ui->gAdjustSlider->setRange(0, 255);
    ui->bAdjustSlider->setRange(0, 255);

    // Exposure slider
    ui->exposureSlider->setRange(1, 10000); // 1μs to 10000μs
    ui->exposureSlider->setValue(100);

    // Export tab
    ui->folderNameLineEdit->setText(QString("output"));
    ui->datetimeCheckBox->setChecked(true);
}

void MainWindow::setupThumbnailContainer(void)
{
    // Create thumbnail container and add it to the scroll area
    m_thumbnailContainer = new ThumbnailContainer(this);

    // Connect signals
    connect(m_thumbnailContainer,
            &ThumbnailContainer::selectionChanged,
            this,
            &MainWindow::onThumbnailSelectionChanged);
    connect(m_thumbnailContainer,
            &ThumbnailContainer::thumbnailDoubleClicked,
            this,
            &MainWindow::onThumbnailDoubleClicked);
    connect(m_thumbnailContainer, &ThumbnailContainer::openImage, this, &MainWindow::onOpenImage);
    connect(m_thumbnailContainer,
            &ThumbnailContainer::enterPressedOnThumbnail,
            this,
            &MainWindow::onEnterPressedOnThumbnail);

    // Add to the scroll area (assuming there's a scroll area in the UI)
    // You may need to adjust this based on your actual UI layout
    if (ui->scrollArea)
    {
        ui->scrollArea->setWidget(m_thumbnailContainer);
        ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    // Set focus to the thumbnail container so it can receive keyboard events
    m_thumbnailContainer->setFocus();
}

void MainWindow::addSampleThumbnails(void)
{
    // Create some sample OpenCV Mat objects for testing
    for (int i = 0; i < 36; ++i)
    {
        // Create a test image with different colors
        cv::Mat testImage = cv::Mat::zeros(200, 200, CV_8UC3);

        // Fill with different colors based on index
        cv::Scalar color;
        switch (i % 5)
        {
        case 0:
            color = cv::Scalar(255, 0, 0);
            break; // Blue
        case 1:
            color = cv::Scalar(0, 255, 0);
            break; // Green
        case 2:
            color = cv::Scalar(0, 0, 255);
            break; // Red
        case 3:
            color = cv::Scalar(255, 255, 0);
            break; // Cyan
        case 4:
            color = cv::Scalar(255, 0, 255);
            break; // Magenta
        }

        testImage.setTo(color);

        // Add some text to identify the image
        cv::putText(testImage,
                    std::to_string(i),
                    cv::Point(50, 100),
                    cv::FONT_HERSHEY_SIMPLEX,
                    2,
                    cv::Scalar(255, 255, 255),
                    3);

        // Add a circle
        cv::circle(testImage, cv::Point(100, 100), 30, cv::Scalar(255, 255, 255), 2);

        m_thumbnailContainer->addThumbnail(testImage);
    }
}

void MainWindow::onThumbnailSelectionChanged(const QList<int> &selectedIndices)
{
    // Handle thumbnail selection changes
    QString selectionText = QString("Selected: ");
    if (selectedIndices.isEmpty())
    {
        selectionText += "None";
    }
    else
    {
        QStringList indices;
        for (int index : selectedIndices)
        {
            indices.append(QString::number(index));
        }
        selectionText += indices.join(", ");
    }

    // You can update a status bar or label here
    statusBar()->showMessage(selectionText);
}

void MainWindow::onThumbnailDoubleClicked(int index)
{
    // Store the index of the thumbnail that was double-clicked
    m_lastFocusedThumbnailIndex = index;

    // Handle thumbnail double-click by opening the image at the clicked index
    cv::Mat image = m_thumbnailContainer->getImageAtIndex(index);
    if (!image.empty())
    {
        onOpenImage(image);
    }
}

void MainWindow::onOpenImage(const cv::Mat &image)
{
    // Create and show the image viewer window
    ImageViewer *viewer = new ImageViewer(image, this);

    // Connect to the viewer's closed signal to restore focus
    connect(viewer,
            &ImageViewer::viewerClosed,
            [this, viewer]()
            {
                // Restore focus to the thumbnail container
                if (m_thumbnailContainer)
                {
                    m_thumbnailContainer->setFocus();
                    // Restore selection to the last focused thumbnail
                    if (m_lastFocusedThumbnailIndex >= 0)
                    {
                        m_thumbnailContainer->selectThumbnail(m_lastFocusedThumbnailIndex, false);
                    }
                }
                // Clean up the viewer
                viewer->deleteLater();
            });

    viewer->show();
    viewer->raise();
    viewer->activateWindow();
}

void MainWindow::onEnterPressedOnThumbnail(int index)
{
    // Store the index of the thumbnail that was opened with Enter
    m_lastFocusedThumbnailIndex = index;

    // Open the image at the specified index
    cv::Mat image = m_thumbnailContainer->getImageAtIndex(index);
    if (!image.empty())
    {
        onOpenImage(image);
    }
}

void MainWindow::updateFolderNamePreview(void)
{
    QString folder_name = ui->folderNameLineEdit->text();

    // Replace spaces with underscores
    folder_name = folder_name.replace(" ", "_");

    if (ui->datetimeCheckBox->isChecked())
    {
        if (folder_name.length() != 0)
        {
            folder_name.prepend(QString("_"));
        }
        folder_name.prepend(QDateTime::currentDateTime().toString("yyyy-MM-dd'T'HH-mm-ss"));
    }

    if (folder_name.length() == 0)
    {
        ui->folderNameLabel->setText(QString("Invalid folder name"));
        ui->folderNameLabel->setStyleSheet("QLabel { color : red; }");

        ui->fileExportPushButton->setDisabled(true);
    }

    else
    {
        ui->folderNameLabel->setText(folder_name);
        ui->folderNameLabel->setStyleSheet("");

        ui->fileExportPushButton->setDisabled(false);
    }
}

void MainWindow::on_startStopPushButton_clicked() {}

void MainWindow::on_gotoBeginningButton_clicked() {}

void MainWindow::on_gotoEndButton_clicked() {}

void MainWindow::on_moveLeftButton_pressed() {}

void MainWindow::on_moveLeftButton_released() {}

void MainWindow::on_moveRightButton_pressed() {}

void MainWindow::on_moveRightButton_released() {}

void MainWindow::on_histColourComboBox_currentIndexChanged(int index) {}

void MainWindow::on_filmTypeComboBox_currentIndexChanged(int index)
{
    switch (index)
    {
    // Auto (DXN) mode
    case 0:

        ui->filmColourComboBox->setDisabled(true);
        ui->wAdjustSlider->setDisabled(true);
        ui->rAdjustSlider->setDisabled(true);
        ui->gAdjustSlider->setDisabled(true);
        ui->bAdjustSlider->setDisabled(true);

        ui->filmColourComboBox->setVisible(false);
        ui->wAdjustSlider->setVisible(false);
        ui->rAdjustSlider->setVisible(false);
        ui->gAdjustSlider->setVisible(false);
        ui->bAdjustSlider->setVisible(false);
        break;

    // Manual mode
    case 1:
        ui->filmColourComboBox->setDisabled(false);
        ui->wAdjustSlider->setDisabled(false);
        ui->rAdjustSlider->setDisabled(false);
        ui->gAdjustSlider->setDisabled(false);
        ui->bAdjustSlider->setDisabled(false);

        ui->filmColourComboBox->setVisible(true);
        ui->wAdjustSlider->setVisible(true);
        ui->rAdjustSlider->setVisible(true);
        ui->gAdjustSlider->setVisible(true);
        ui->bAdjustSlider->setVisible(true);
        break;

    // Film selection mode
    default:
        ui->filmColourComboBox->setDisabled(true);
        ui->wAdjustSlider->setDisabled(false);
        ui->rAdjustSlider->setDisabled(false);
        ui->gAdjustSlider->setDisabled(false);
        ui->bAdjustSlider->setDisabled(false);

        ui->filmColourComboBox->setVisible(false);
        ui->wAdjustSlider->setVisible(true);
        ui->rAdjustSlider->setVisible(true);
        ui->gAdjustSlider->setVisible(true);
        ui->bAdjustSlider->setVisible(true);
        break;
    }
}

void MainWindow::on_filmColourComboBox_currentIndexChanged(int index) {}

void MainWindow::on_wAdjustSlider_valueChanged(int value)
{
    double gainDb = value / 100.0;
    qDebug() << "Main window gain slider changed to:" << value << "device units (" << gainDb
             << "dB)";
    ui->gainValueLabel->setText(QString::number(gainDb, 'f', 1) + "dB");
}

void MainWindow::on_rAdjustSlider_valueChanged(int value)
{
    qDebug() << "Main window red slider changed to:" << value;
    ui->redValueLabel->setText(QString::number(value));
}

void MainWindow::on_gAdjustSlider_valueChanged(int value)
{
    qDebug() << "Main window green slider changed to:" << value;
    ui->greenValueLabel->setText(QString::number(value));
}

void MainWindow::on_bAdjustSlider_valueChanged(int value)
{
    qDebug() << "Main window blue slider changed to:" << value;
    ui->blueValueLabel->setText(QString::number(value));
}

void MainWindow::on_folderNameLineEdit_returnPressed() { updateFolderNamePreview(); }

void MainWindow::on_datetimeCheckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    updateFolderNamePreview();
}

void MainWindow::on_fileFormatComboBox_currentIndexChanged(int index) {}

void MainWindow::on_fileResComboBox_currentIndexChanged(int index) {}

void MainWindow::on_fileExportPushButton_clicked() {}

void MainWindow::on_folderNameLineEdit_textChanged(const QString &arg1)
{
    updateFolderNamePreview();
}

// File menu implementations
void MainWindow::on_actionNew_triggered()
{
    // TODO: Implement new project functionality
    statusBar()->showMessage("New project - not yet implemented");
}

void MainWindow::on_actionOpen_triggered()
{
    // TODO: Implement open project functionality
    statusBar()->showMessage("Open project - not yet implemented");
}

void MainWindow::on_actionSave_triggered()
{
    // TODO: Implement save project functionality
    statusBar()->showMessage("Save project - not yet implemented");
}

void MainWindow::on_actionSaveAs_triggered()
{
    // TODO: Implement save as project functionality
    statusBar()->showMessage("Save as project - not yet implemented");
}

void MainWindow::on_actionExit_triggered() { close(); }

// Edit menu implementations
void MainWindow::on_actionPreferences_2_triggered()
{
    // TODO: Implement preferences dialog
    statusBar()->showMessage("Preferences - not yet implemented");
}

// Help menu implementations
void MainWindow::on_actionCalibration_triggered()
{
    // Create or reuse calibration window
    if (m_calibrationWindow == nullptr)
    {
        m_calibrationWindow = new CalibrationWindow(this);

        // Connect to the calibration window's signals
        connect(m_calibrationWindow,
                &CalibrationWindow::windowClosed,
                this,
                [this]()
                {
                    m_calibrationWindow->deleteLater();
                    m_calibrationWindow = nullptr;
                });

        // Connect synchronization signals
        connect(m_calibrationWindow,
                &CalibrationWindow::exposureChanged,
                this,
                &MainWindow::onExposureChanged);
        connect(
            m_calibrationWindow, &CalibrationWindow::gainChanged, this, &MainWindow::onGainChanged);
        connect(m_calibrationWindow,
                &CalibrationWindow::redBacklightChanged,
                this,
                &MainWindow::onRedBacklightChanged);
        connect(m_calibrationWindow,
                &CalibrationWindow::greenBacklightChanged,
                this,
                &MainWindow::onGreenBacklightChanged);
        connect(m_calibrationWindow,
                &CalibrationWindow::blueBacklightChanged,
                this,
                &MainWindow::onBlueBacklightChanged);
    }

    // Show as a separate window
    m_calibrationWindow->show();
    m_calibrationWindow->raise();
    m_calibrationWindow->activateWindow();
    m_calibrationWindow->startPreview();
}

void MainWindow::on_actionHelp_triggered()
{
    // TODO: Implement help dialog or documentation
    statusBar()->showMessage("Help - not yet implemented");
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this,
                       "About Korova",
                       "Korova Film Scanner v1.0\n\n"
                       "A professional film scanning application for the Knokke film scanner.\n\n"
                       "© 2024 Soke Engineering\n"
                       "All rights reserved.");
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    // Calculate available width for thumbnails - be very conservative
    int scrollAreaWidth   = width() - 120; // Very conservative margin for scroll area frame
    int minThumbnailWidth = 300;           // Minimum thumbnail width

    if (m_thumbnailContainer)
    {
        // First, calculate how many thumbnails can fit per row
        m_thumbnailContainer->setThumbnailsPerRowFromWidth(scrollAreaWidth, minThumbnailWidth);

        // Then calculate the actual thumbnail size to fill the available space
        int gridMargins    = 20; // 10px left + 10px right margins
        int spacing        = 10; // 10px spacing between thumbnails
        int availableWidth = scrollAreaWidth - gridMargins;

        // Get the current number of thumbnails per row
        int thumbnailsPerRow = m_thumbnailContainer->getThumbnailsPerRow();

        // Calculate spacing between thumbnails
        int totalSpacing = (thumbnailsPerRow - 1) * spacing;

        // Calculate thumbnail width to fill available space
        int thumbnailWidth  = (availableWidth - totalSpacing) / thumbnailsPerRow;
        int thumbnailHeight = (thumbnailWidth * 2) / 3; // 3:2 aspect ratio

        // Ensure minimum size but don't exceed available space
        thumbnailWidth  = qMax(thumbnailWidth, minThumbnailWidth);
        thumbnailHeight = qMax(thumbnailHeight, (minThumbnailWidth * 2) / 3);

        // Double-check that we don't exceed available width
        int totalWidth = (thumbnailWidth * thumbnailsPerRow) + totalSpacing;
        if (totalWidth > availableWidth)
        {
            // Recalculate with smaller thumbnails to fit
            thumbnailWidth  = (availableWidth - totalSpacing) / thumbnailsPerRow;
            thumbnailHeight = (thumbnailWidth * 2) / 3;
        }

        m_thumbnailContainer->setThumbnailSize(thumbnailWidth, thumbnailHeight);
    }
}

// Calibration window synchronization slots
void MainWindow::onExposureChanged(uint16_t exposure)
{
    m_currentExposure = exposure;
    qDebug() << "Exposure updated to:" << exposure << "μs";

    // Update main window exposure slider and label
    ui->exposureSlider->setValue(static_cast<int>(exposure));
    ui->exposureValueLabel->setText(QString::number(exposure) + "μs");
}

void MainWindow::onGainChanged(uint16_t gain)
{
    m_currentGain = gain;
    double gainDb = gain / 100.0;
    qDebug() << "Gain updated to:" << gain << "device units (" << gainDb << "dB)";

    // Update main window gain slider (wAdjustSlider) and label
    ui->wAdjustSlider->setValue(static_cast<int>(gain));
    ui->gainValueLabel->setText(QString::number(gainDb, 'f', 1) + "dB");
}

void MainWindow::onRedBacklightChanged(uint16_t red)
{
    m_currentRedBacklight = red;
    qDebug() << "Red backlight updated to:" << red;

    // Update main window red slider (convert from 0-65535 to 0-255)
    int sliderValue = static_cast<int>((red * 255) / 65535);
    ui->rAdjustSlider->setValue(sliderValue);
    ui->redValueLabel->setText(QString::number(sliderValue));
}

void MainWindow::onGreenBacklightChanged(uint16_t green)
{
    m_currentGreenBacklight = green;
    qDebug() << "Green backlight updated to:" << green;

    // Update main window green slider (convert from 0-65535 to 0-255)
    int sliderValue = static_cast<int>((green * 255) / 65535);
    ui->gAdjustSlider->setValue(sliderValue);
    ui->greenValueLabel->setText(QString::number(sliderValue));
}

void MainWindow::onBlueBacklightChanged(uint16_t blue)
{
    m_currentBlueBacklight = blue;
    qDebug() << "Blue backlight updated to:" << blue;

    // Update main window blue slider (convert from 0-65535 to 0-255)
    int sliderValue = static_cast<int>((blue * 255) / 65535);
    ui->bAdjustSlider->setValue(sliderValue);
    ui->blueValueLabel->setText(QString::number(sliderValue));
}

// Main window slider change handlers
void MainWindow::on_exposureSlider_valueChanged(int value)
{
    qDebug() << "Main window exposure slider changed to:" << value << "μs";
    ui->exposureValueLabel->setText(QString::number(value) + "μs");
    // This could trigger updates to calibration window if needed
}
