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
    void exposureChanged(uint16_t exposure);
    void gainChanged(uint16_t gain);
    void redBacklightChanged(uint16_t red);
    void greenBacklightChanged(uint16_t green);
    void blueBacklightChanged(uint16_t blue);

  protected:
    void closeEvent(QCloseEvent *event) override;

  private slots:
    void updatePreview();
    void onRedSliderChanged(int value);
    void onGreenSliderChanged(int value);
    void onBlueSliderChanged(int value);
    void onExposureSliderChanged(int value);
    void onGainSliderChanged(int value);
    void onSaveImageClicked();
    void onSliderUpdateTimeout();
    void onMotorLeftPressed();
    void onMotorLeftReleased();
    void onMotorRightPressed();
    void onMotorRightReleased();

  private:
    void    setupUI();
    void    setupScanner();
    cv::Mat demosaicBayer(const cv::Mat &bayerData);
    cv::Mat interpolateChannel(const cv::Mat &channel);
    QImage  matToQImage(const cv::Mat &mat);
    double  calculateSharpness(const cv::Mat &image);

    QLabel                 *m_previewLabel;
    QTimer                 *m_previewTimer;
    std::unique_ptr<Knokke> m_knokke;

    // Backlight control sliders
    QSlider *m_redSlider;
    QSlider *m_greenSlider;
    QSlider *m_blueSlider;

    // Backlight value labels
    QLabel *m_redValueLabel;
    QLabel *m_greenValueLabel;
    QLabel *m_blueValueLabel;

    // Exposure and gain control sliders
    QSlider *m_exposureSlider;
    QSlider *m_gainSlider;

    // Exposure and gain value labels
    QLabel *m_exposureValueLabel;
    QLabel *m_gainValueLabel;

    // RGB channel min/max/average value labels
    QLabel *m_redMinLabel;
    QLabel *m_redMaxLabel;
    QLabel *m_redAvgLabel;
    QLabel *m_greenMinLabel;
    QLabel *m_greenMaxLabel;
    QLabel *m_greenAvgLabel;
    QLabel *m_blueMinLabel;
    QLabel *m_blueMaxLabel;
    QLabel *m_blueAvgLabel;

    // Save button
    QPushButton *m_saveButton;

    // Motor control buttons
    QPushButton *m_motorLeftButton;
    QPushButton *m_motorRightButton;

    // Sharpness display
    QLabel *m_sharpnessLabel;

    // Last captured frame data for saving
    cv::Mat m_lastFrame;

    // Debouncing for slider updates to prevent USB interference
    QTimer                 *m_sliderUpdateTimer;
    bool                    m_sliderUpdatePending;
    Knokke::BacklightParams m_pendingBacklight;
    uint32_t                m_pendingExposure;
    uint16_t                m_pendingGain;

    // Frame dropping for 50fps (400fps / 8 = 50fps)
    int                  m_frameCounter;
    static constexpr int FRAME_DROP_COUNT    = 8;
    static constexpr int PREVIEW_INTERVAL_MS = 20; // 50fps = 20ms interval
    static constexpr int FRAME_WIDTH         = 3840;
    static constexpr int FRAME_HEIGHT        = 12;
    static constexpr int FRAME_BYTES         = FRAME_WIDTH * FRAME_HEIGHT * 2; // RAW16 format
};

#endif // CALIBRATIONWINDOW_H
