#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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

  private:
    Ui::MainWindow     *ui;
    ThumbnailContainer *m_thumbnailContainer;
    int                 m_lastFocusedThumbnailIndex;

    void setDefaults(void);
    void updateFolderNamePreview(void);
    void setupThumbnailContainer(void);
    void addSampleThumbnails(void);

  protected:
    void resizeEvent(QResizeEvent *event) override;
};
#endif // MAINWINDOW_H
