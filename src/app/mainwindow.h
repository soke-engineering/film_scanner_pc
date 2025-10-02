#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "calibrationwindow.h"
#include "imageviewer.h"
#include "thumbnailcontainer.h"
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui
{
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

  private slots:
    // File menu slots
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();
    void on_actionExit_triggered();

    // Edit menu slots
    void on_actionPreferences_2_triggered();

    // Help menu slots
    void on_actionCalibration_triggered();
    void on_actionHelp_triggered();
    void on_actionAbout_triggered();

    // UI control slots
    void on_startStopPushButton_clicked();
    void on_gotoBeginningButton_clicked();
    void on_gotoEndButton_clicked();
    void on_moveLeftButton_pressed();
    void on_moveLeftButton_released();
    void on_moveRightButton_pressed();
    void on_moveRightButton_released();
    void on_histColourComboBox_currentIndexChanged(int index);
    void on_filmTypeComboBox_currentIndexChanged(int index);
    void on_filmColourComboBox_currentIndexChanged(int index);
    void on_wAdjustSlider_valueChanged(int value);
    void on_rAdjustSlider_valueChanged(int value);
    void on_gAdjustSlider_valueChanged(int value);
    void on_bAdjustSlider_valueChanged(int value);
    void on_exposureSlider_valueChanged(int value);
    void on_folderNameLineEdit_returnPressed();
    void on_datetimeCheckBox_checkStateChanged(const Qt::CheckState &arg1);
    void on_fileFormatComboBox_currentIndexChanged(int index);
    void on_fileResComboBox_currentIndexChanged(int index);
    void on_fileExportPushButton_clicked();
    void on_folderNameLineEdit_textChanged(const QString &arg1);

    // Thumbnail container slots
    void onThumbnailSelectionChanged(const QList<int> &selectedIndices);
    void onThumbnailDoubleClicked(int index);
    void onOpenImage(const cv::Mat &image);
    void onEnterPressedOnThumbnail(int index);

    // Calibration window synchronization slots
    void onExposureChanged(uint16_t exposure);
    void onGainChanged(uint16_t gain);
    void onRedBacklightChanged(uint16_t red);
    void onGreenBacklightChanged(uint16_t green);
    void onBlueBacklightChanged(uint16_t blue);

  private:
    Ui::MainWindow     *ui;
    ThumbnailContainer *m_thumbnailContainer;
    int                 m_lastFocusedThumbnailIndex;

    // Calibration window reference
    CalibrationWindow *m_calibrationWindow;

    // Scanner settings storage
    uint16_t m_currentExposure;
    uint16_t m_currentGain;
    uint16_t m_currentRedBacklight;
    uint16_t m_currentGreenBacklight;
    uint16_t m_currentBlueBacklight;

    void setDefaults(void);
    void updateFolderNamePreview(void);
    void setupThumbnailContainer(void);
    void addSampleThumbnails(void);

  protected:
    void resizeEvent(QResizeEvent *event) override;
};
#endif // MAINWINDOW_H
