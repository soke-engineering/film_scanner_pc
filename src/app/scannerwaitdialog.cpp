#include "scannerwaitdialog.h"
#include "../drivers/scanners/Knokke.h"

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <iostream>

ScannerWaitDialog::ScannerWaitDialog(QWidget *parent)
    : QDialog(parent), m_statusLabel(nullptr), m_instructionLabel(nullptr), m_cancelButton(nullptr),
      m_layout(nullptr), m_detectionTimer(nullptr), m_knokke(std::make_unique<Knokke>())
{
    setupUI();
    // Defer scanner detection to avoid immediate USB access
    QTimer::singleShot(100, this, &ScannerWaitDialog::startScannerDetection);
}

ScannerWaitDialog::~ScannerWaitDialog() { stopScannerDetection(); }

void ScannerWaitDialog::setupUI()
{
    setWindowTitle("Korova Film Scanner");
    setModal(false); // Try non-modal to see if icon shows
    setFixedSize(400, 250);
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    // Create layout
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(20);
    m_layout->setContentsMargins(30, 30, 30, 30);

    // Add icon as a label - much larger and centered
    QLabel *iconLabel = new QLabel(this);
    iconLabel->setFixedSize(120, 120);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setScaledContents(true);

    // Try to load and display the icon
    QIcon logoIcon;
    if (QFile::exists("icon.png"))
    {
        logoIcon = QIcon("icon.png");
    }
    else
    {
        logoIcon = QIcon(":/icon.png");
    }

    if (!logoIcon.isNull())
    {
        QPixmap iconPixmap = logoIcon.pixmap(120, 120);
        iconLabel->setPixmap(iconPixmap);
    }
    else
    {
        // Fallback: create a simple "K" icon
        QPixmap pixmap(120, 120);
        pixmap.fill(QColor(70, 130, 180));
        QPainter painter(&pixmap);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 48, QFont::Bold));
        painter.drawText(pixmap.rect(), Qt::AlignCenter, "K");
        iconLabel->setPixmap(pixmap);
    }

    // Center the icon
    m_layout->addWidget(iconLabel, 0, Qt::AlignCenter);

    // Add spacing between icon and text
    m_layout->addSpacing(30);

    // Status label - simplified
    m_statusLabel = new QLabel("Connecting to Knokke...", this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("QLabel { font-size: 14px; color: #666; }");
    m_layout->addWidget(m_statusLabel);

    // Remove instruction label and cancel button for simplified UI
    m_instructionLabel = nullptr;
    m_cancelButton     = nullptr;

    // Set application icon with multiple fallback options
    QIcon icon;

    // Try different icon loading methods
    QStringList iconPaths = {
        "icon.png", ":/icon.png", "../icon.png", "../../icon.png", "src/app/icon.png"};

    bool iconLoaded = false;
    for (const QString &path : iconPaths)
    {
        if (QFile::exists(path))
        {
            icon = QIcon(path);
            if (!icon.isNull())
            {
                qDebug() << "Successfully loaded icon from:" << path;
                iconLoaded = true;
                break;
            }
        }
    }

    // If no icon loaded, create a simple text-based icon
    if (!iconLoaded)
    {
        qDebug() << "No icon file found, creating text-based icon";
        QPixmap pixmap(64, 64);
        pixmap.fill(QColor(70, 130, 180)); // Steel blue background

        QPainter painter(&pixmap);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 20, QFont::Bold));
        painter.drawText(pixmap.rect(), Qt::AlignCenter, "K");

        icon = QIcon(pixmap);
    }

    setWindowIcon(icon);

    // Also set the application icon
    QApplication::setWindowIcon(icon);

    // Additional debug to verify icon is set
    qDebug() << "Icon set on dialog. Icon is null:" << icon.isNull();
    qDebug() << "Available icon sizes:" << icon.availableSizes();

    // Force icon refresh after window is shown
    QTimer::singleShot(100,
                       this,
                       [this, icon]()
                       {
                           setWindowIcon(icon);
                           qDebug() << "Icon refreshed after window show";
                       });
}

void ScannerWaitDialog::startScannerDetection()
{
    std::cout << "startScannerDetection() called" << std::endl;
    // First check if scanner is already connected
    if (isScannerConnected())
    {
        std::cout << "Scanner is already connected!" << std::endl;
        m_statusLabel->setText("Knokke scanner found!");
        m_statusLabel->setStyleSheet(
            "QLabel { font-size: 14px; font-weight: bold; color: green; }");
        if (m_instructionLabel)
        {
            m_instructionLabel->setText("Opening application...");
        }

        // Close dialog after a short delay
        std::cout << "Setting up QTimer::singleShot..." << std::endl;
        QTimer::singleShot(1000,
                           this,
                           [this]()
                           {
                               std::cout << "QTimer callback started" << std::endl;
                               std::cout << "Emitting scannerDetected signal..." << std::endl;
                               emit scannerDetected();
                               std::cout << "Signal emitted successfully" << std::endl;
                               std::cout << "Calling accept()..." << std::endl;
                               accept();
                               std::cout << "Dialog closed successfully" << std::endl;
                           });
        std::cout << "QTimer::singleShot set up successfully" << std::endl;
        return;
    }
    std::cout << "Scanner not connected, starting periodic detection..." << std::endl;

    // Set up timer for periodic checking
    m_detectionTimer = new QTimer(this);
    connect(m_detectionTimer, &QTimer::timeout, this, &ScannerWaitDialog::checkForScanner);
    m_detectionTimer->start(DETECTION_INTERVAL_MS);
}

void ScannerWaitDialog::stopScannerDetection()
{
    if (m_detectionTimer)
    {
        m_detectionTimer->stop();
        m_detectionTimer->deleteLater();
        m_detectionTimer = nullptr;
    }
}

void ScannerWaitDialog::checkForScanner()
{
    if (isScannerConnected())
    {
        stopScannerDetection();

        m_statusLabel->setText("Connected!");
        m_statusLabel->setStyleSheet(
            "QLabel { font-size: 14px; font-weight: bold; color: green; }");

        // Close dialog after a short delay
        QTimer::singleShot(1000,
                           this,
                           [this]()
                           {
                               emit scannerDetected();
                               accept();
                           });
    }
    else
    {
        // Update status to show we're still connecting
        static int dotCount = 0;
        dotCount            = (dotCount + 1) % 4;
        QString dots        = QString(".").repeated(dotCount);
        m_statusLabel->setText(QString("Connecting to Knokke%1").arg(dots));
    }
}

void ScannerWaitDialog::onCancelClicked()
{
    // Cancel button removed in simplified UI
    // This method is kept for compatibility but won't be called
}

bool ScannerWaitDialog::isScannerConnected()
{
    if (!m_knokke)
    {
        return false;
    }

    try
    {
        // Initialize libusb if not already done
        Knokke::Error initResult = m_knokke->initialize();
        if (initResult != Knokke::Error::SUCCESS)
        {
            return false;
        }

        // Try to connect to the scanner
        Knokke::Error connectResult = m_knokke->connect();
        if (connectResult == Knokke::Error::SUCCESS)
        {
            // Disconnect immediately since we just wanted to check if it's available
            m_knokke->disconnect();
            return true;
        }

        return false;
    }
    catch (const std::exception &e)
    {
        // Log error but don't crash
        std::cerr << "Scanner detection error: " << e.what() << std::endl;
        return false;
    }
    catch (...)
    {
        // Log error but don't crash
        std::cerr << "Unknown scanner detection error" << std::endl;
        return false;
    }
}
