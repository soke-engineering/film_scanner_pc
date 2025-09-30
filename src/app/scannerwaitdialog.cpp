#include "scannerwaitdialog.h"
#include "../drivers/scanners/Knokke.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

ScannerWaitDialog::ScannerWaitDialog(QWidget *parent)
    : QDialog(parent), m_statusLabel(nullptr), m_instructionLabel(nullptr), m_cancelButton(nullptr),
      m_layout(nullptr), m_detectionTimer(nullptr), m_knokke(std::make_unique<Knokke>())
{
    setupUI();
    startScannerDetection();
}

ScannerWaitDialog::~ScannerWaitDialog() { stopScannerDetection(); }

void ScannerWaitDialog::setupUI()
{
    setWindowTitle("Waiting for Knokke Scanner");
    setModal(true);
    setFixedSize(400, 200);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Create layout
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(20);
    m_layout->setContentsMargins(30, 30, 30, 30);

    // Status label
    m_statusLabel = new QLabel("Searching for Knokke scanner...", this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; }");
    m_layout->addWidget(m_statusLabel);

    // Instruction label
    m_instructionLabel = new QLabel("Please connect your Knokke film scanner to continue.", this);
    m_instructionLabel->setAlignment(Qt::AlignCenter);
    m_instructionLabel->setWordWrap(true);
    m_instructionLabel->setStyleSheet("QLabel { font-size: 12px; color: #666; }");
    m_layout->addWidget(m_instructionLabel);

    // Cancel button
    m_cancelButton = new QPushButton("Cancel", this);
    m_cancelButton->setFixedSize(100, 30);
    connect(m_cancelButton, &QPushButton::clicked, this, &ScannerWaitDialog::onCancelClicked);

    // Center the cancel button
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addStretch();
    m_layout->addLayout(buttonLayout);

    // Set application icon
    setWindowIcon(QIcon(":/icon.png"));
}

void ScannerWaitDialog::startScannerDetection()
{
    // First check if scanner is already connected
    if (isScannerConnected())
    {
        m_statusLabel->setText("Knokke scanner found!");
        m_statusLabel->setStyleSheet(
            "QLabel { font-size: 14px; font-weight: bold; color: green; }");
        m_instructionLabel->setText("Opening application...");

        // Close dialog after a short delay
        QTimer::singleShot(1000,
                           this,
                           [this]()
                           {
                               emit scannerDetected();
                               accept();
                           });
        return;
    }

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

        m_statusLabel->setText("Knokke scanner detected!");
        m_statusLabel->setStyleSheet(
            "QLabel { font-size: 14px; font-weight: bold; color: green; }");
        m_instructionLabel->setText("Opening application...");

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
        // Update status to show we're still searching
        static int dotCount = 0;
        dotCount            = (dotCount + 1) % 4;
        QString dots        = QString(".").repeated(dotCount);
        m_statusLabel->setText(QString("Searching for Knokke scanner%1").arg(dots));
    }
}

void ScannerWaitDialog::onCancelClicked()
{
    stopScannerDetection();
    emit dialogCancelled();
    reject();
}

bool ScannerWaitDialog::isScannerConnected()
{
    if (!m_knokke)
    {
        return false;
    }

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
