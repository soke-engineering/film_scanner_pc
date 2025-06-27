#include "imageviewer.h"
#include <QApplication>
#include <QPainter>

ImageViewer::ImageViewer(const cv::Mat &image, QWidget *parent)
    : QMainWindow(parent), m_image(image.clone()), m_windowWidth(600), m_windowHeight(400)
{
    setupUI();
    displayImage();
}

ImageViewer::~ImageViewer() {}

void ImageViewer::setupUI()
{
    // Set window properties
    setWindowTitle("Image Viewer");
    setWindowFlags(Qt::Window);
    resize(m_windowWidth, m_windowHeight);

    // Create central widget
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Create layout
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(10, 10, 10, 10);

    // Create image label
    m_imageLabel = new QLabel(this);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setScaledContents(true);
    layout->addWidget(m_imageLabel);

    // Set focus policy to receive keyboard events
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
}

void ImageViewer::displayImage()
{
    if (m_image.empty())
    {
        m_imageLabel->setText("No image to display");
        return;
    }

    // Calculate display size maintaining aspect ratio
    double aspectRatio   = static_cast<double>(m_image.cols) / m_image.rows;
    int    displayWidth  = m_windowWidth - 20; // Account for margins
    int    displayHeight = static_cast<int>(displayWidth / aspectRatio);

    // Ensure height doesn't exceed window height
    if (displayHeight > m_windowHeight - 40)
    {
        displayHeight = m_windowHeight - 40;
        displayWidth  = static_cast<int>(displayHeight * aspectRatio);
    }

    // Convert Mat to QPixmap and scale
    QPixmap pixmap = matToPixmap(m_image);
    QPixmap scaledPixmap =
        pixmap.scaled(displayWidth, displayHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    m_imageLabel->setPixmap(scaledPixmap);
}

QPixmap ImageViewer::matToPixmap(const cv::Mat &mat)
{
    if (mat.empty())
    {
        return QPixmap();
    }

    // Convert BGR to RGB if needed
    cv::Mat rgbMat;
    if (mat.channels() == 3)
    {
        cv::cvtColor(mat, rgbMat, cv::COLOR_BGR2RGB);
    }
    else if (mat.channels() == 1)
    {
        cv::cvtColor(mat, rgbMat, cv::COLOR_GRAY2RGB);
    }
    else
    {
        rgbMat = mat.clone();
    }

    // Create QImage from Mat
    QImage image(rgbMat.data,
                 rgbMat.cols,
                 rgbMat.rows,
                 static_cast<int>(rgbMat.step),
                 QImage::Format_RGB888);

    // Convert to QPixmap
    return QPixmap::fromImage(image);
}

void ImageViewer::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_Escape:
        close();
        break;
    default:
        QMainWindow::keyPressEvent(event);
        break;
    }
}

void ImageViewer::closeEvent(QCloseEvent *event)
{
    // Emit signal before closing
    emit viewerClosed();
    QMainWindow::closeEvent(event);
}