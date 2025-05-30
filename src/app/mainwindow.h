#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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
    void on_deviceComboBox_currentIndexChanged(int index);
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

  private:
    Ui::MainWindow *ui;
    void            setDefaults(void);
    void            updateFolderNamePreview(void);
};
#endif // MAINWINDOW_H
