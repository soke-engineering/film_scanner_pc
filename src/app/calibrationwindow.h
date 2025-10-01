#ifndef CALIBRATIONWINDOW_H
#define CALIBRATIONWINDOW_H

#include "../drivers/scanners/Knokke.h"
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QWidget>
#include <memory>
#include <opencv2/opencv.hpp>

class CalibrationWindow : public QWidget
{
    Q_OBJECT

  public:
    explicit CalibrationWindow(QWidget *parent = nullptr);
    ~CalibrationWindow();

    void startPreview();
    void stopPreview();

  signals:
    void windowClosed();

  protected:
    void closeEvent(QCloseEvent *event) override;

  private slots:
    void updatePreview();
    void onRedSliderChanged(int value);
    void onGreenSliderChanged(int value);
    void onBlueSliderChanged(int value);
    void onSaveImageClicked();
    void onSliderUpdateTimeout();

  private:
    void    setupUI();
    void    setupScanner();
    cv::Mat demosaicBayer(const cv::Mat &bayerData);
    cv::Mat interpolateChannel(const cv::Mat &channel);
    QImage  matToQImage(const cv::Mat &mat);

    QLabel                 *m_previewLabel;
    QTimer                 *m_previewTimer;
    std::unique_ptr<Knokke> m_knokke;

    // Backlight control sliders
    QSlider *m_redSlider;
    QSlider *m_greenSlider;
    QSlider *m_blueSlider;

    // Save button
    QPushButton *m_saveButton;

    // Last captured frame data for saving
    cv::Mat m_lastFrame;

    // Debouncing for slider updates to prevent USB interference
    QTimer                 *m_sliderUpdateTimer;
    bool                    m_sliderUpdatePending;
    Knokke::BacklightParams m_pendingBacklight;

    // Frame dropping for 50fps (400fps / 8 = 50fps)
    int                  m_frameCounter;
    static constexpr int FRAME_DROP_COUNT    = 8;
    static constexpr int PREVIEW_INTERVAL_MS = 20; // 50fps = 20ms interval
    static constexpr int FRAME_WIDTH         = 3840;
    static constexpr int FRAME_HEIGHT        = 12;
    static constexpr int FRAME_BYTES         = FRAME_WIDTH * FRAME_HEIGHT * 2; // RAW16 format
};

#endif // CALIBRATIONWINDOW_H
