#ifndef SCANNERWAITDIALOG_H
#define SCANNERWAITDIALOG_H

#include <QDialog>
#include <QTimer>
#include <memory>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QVBoxLayout;
QT_END_NAMESPACE

// Forward declaration to avoid including the full Knokke header
class Knokke;

class ScannerWaitDialog : public QDialog
{
    Q_OBJECT

  public:
    explicit ScannerWaitDialog(QWidget *parent = nullptr);
    ~ScannerWaitDialog();

    // Check if scanner is already connected
    bool isScannerConnected();

  signals:
    void scannerDetected();
    void dialogCancelled();

  private slots:
    void checkForScanner();
    void onCancelClicked();

  private:
    void setupUI();
    void startScannerDetection();
    void stopScannerDetection();

    QLabel      *m_statusLabel;
    QLabel      *m_instructionLabel;
    QPushButton *m_cancelButton;
    QVBoxLayout *m_layout;

    QTimer                 *m_detectionTimer;
    std::unique_ptr<Knokke> m_knokke;

    static constexpr int DETECTION_INTERVAL_MS = 1000; // Check every second
};

#endif // SCANNERWAITDIALOG_H
