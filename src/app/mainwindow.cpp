#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDateTime>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <opencv2/opencv.hpp>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setDefaults();
    setupThumbnailContainer();
    addSampleThumbnails();

    // Initialize last focused thumbnail index
    m_lastFocusedThumbnailIndex = 0;
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::setDefaults(void)
{
    // Set the active tab
    ui->tabWidget->setCurrentIndex(0);

    // Scan tab
    ui->filmTypeComboBox->setCurrentIndex(0); // Default to DXN (auto)

    ui->wAdjustSlider->setRange(0, 255);
    ui->rAdjustSlider->setRange(0, 255);
    ui->gAdjustSlider->setRange(0, 255);
    ui->bAdjustSlider->setRange(0, 255);

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

void MainWindow::on_wAdjustSlider_valueChanged(int value) {}

void MainWindow::on_rAdjustSlider_valueChanged(int value) {}

void MainWindow::on_gAdjustSlider_valueChanged(int value) {}

void MainWindow::on_bAdjustSlider_valueChanged(int value) {}

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
