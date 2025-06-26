#include "thumbnailwidget.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QApplication>
#include <QStyle>

ThumbnailWidget::ThumbnailWidget(const cv::Mat& mat, int index, QWidget* parent)
    : QWidget(parent)
    , m_mat(mat.clone())
    , m_index(index)
    , m_selected(false)
    , m_thumbnailWidth(150)
    , m_thumbnailHeight(150)
{
    setFixedSize(m_thumbnailWidth + 10, m_thumbnailHeight + 30);
    setMouseTracking(true);
    setFocusPolicy(Qt::NoFocus);
    
    // Create layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(2);
    
    // Create image label
    m_imageLabel = new QLabel(this);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setScaledContents(true);
    m_imageLabel->setFixedSize(m_thumbnailWidth, m_thumbnailHeight);
    layout->addWidget(m_imageLabel);
    
    // Create index label
    QLabel* indexLabel = new QLabel(QString::number(index), this);
    indexLabel->setAlignment(Qt::AlignCenter);
    indexLabel->setStyleSheet("QLabel { color: gray; font-size: 10px; }");
    layout->addWidget(indexLabel);
    
    updateDisplay();
    
    // Set selection style
    setStyleSheet("ThumbnailWidget { border: 2px solid transparent; border-radius: 5px; }");
}

ThumbnailWidget::~ThumbnailWidget()
{
}

void ThumbnailWidget::setSelected(bool selected)
{
    if (m_selected != selected) {
        m_selected = selected;
        update();
    }
}

void ThumbnailWidget::setThumbnailSize(int width, int height)
{
    m_thumbnailWidth = width;
    m_thumbnailHeight = height;
    setFixedSize(m_thumbnailWidth + 10, m_thumbnailHeight + 30);
    m_imageLabel->setFixedSize(m_thumbnailWidth, m_thumbnailHeight);
    updateDisplay();
}

void ThumbnailWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        bool multiSelect = (event->modifiers() & (Qt::ControlModifier | Qt::MetaModifier)) != 0;
        emit thumbnailClicked(m_index, multiSelect);
    }
}

void ThumbnailWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit thumbnailDoubleClicked(m_index);
    }
}

void ThumbnailWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    
    if (m_selected) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // Draw selection border
        QPen pen(QApplication::palette().color(QPalette::Highlight), 2);
        painter.setPen(pen);
        painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 5, 5);
        
        // Draw selection background
        QColor highlightColor = QApplication::palette().color(QPalette::Highlight);
        highlightColor.setAlpha(50);
        painter.fillRect(rect().adjusted(2, 2, -2, -2), highlightColor);
    }
}

void ThumbnailWidget::updateDisplay()
{
    m_pixmap = matToPixmap(m_mat);
    m_imageLabel->setPixmap(m_pixmap.scaled(m_thumbnailWidth, m_thumbnailHeight, 
                                           Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

QPixmap ThumbnailWidget::matToPixmap(const cv::Mat& mat)
{
    if (mat.empty()) {
        return QPixmap();
    }
    
    // Convert BGR to RGB if needed
    cv::Mat rgbMat;
    if (mat.channels() == 3) {
        cv::cvtColor(mat, rgbMat, cv::COLOR_BGR2RGB);
    } else if (mat.channels() == 1) {
        cv::cvtColor(mat, rgbMat, cv::COLOR_GRAY2RGB);
    } else {
        rgbMat = mat.clone();
    }
    
    // Create QImage from Mat
    QImage image(rgbMat.data, rgbMat.cols, rgbMat.rows, 
                static_cast<int>(rgbMat.step), QImage::Format_RGB888);
    
    // Convert to QPixmap
    return QPixmap::fromImage(image);
} 