#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "FTDIEnumerator.h"

#include <QDateTime>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setDefaults();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::setDefaults(void)
{
    // Set the active tab
    ui->tabWidget->setCurrentIndex(0);

    // Device tab
    ui->deviceComboBox->clear();           // Refresh device list
    ui->deviceTextEdit->setDisabled(true); // Infor box should not be edittable

    // Scan tab
    emit ui->filmTypeComboBox->currentIndexChanged(0); // Default to DXN (auto)

    ui->wAdjustSlider->setRange(0, 255);
    ui->rAdjustSlider->setRange(0, 255);
    ui->gAdjustSlider->setRange(0, 255);
    ui->bAdjustSlider->setRange(0, 255);

    ui->previewRadioButton->setChecked(true);

    // Export tab
    emit ui->folderNameLineEdit->setText(QString("output"));
    emit ui->datetimeCheckBox->setChecked(true);

    // Load availible devices
    ui->deviceComboBox->clear();
    for (const std::string &serial : FTDIEnumerator::getUniqueSerialNumbers())
    {
        ui->deviceComboBox->addItem(QString(serial.c_str()));
    }
}

void MainWindow::updateFolderNamePreview(void)
{
    QString folder_name = ui->folderNameLineEdit->text();
    
    // Replace spaces with underscores
    folder_name = folder_name.replace(" ", "_");

    if (ui->datetimeCheckBox->isChecked())
    {
        if (folder_name.length() != 0)
        {
            folder_name.prepend(QString("_"));
        }
        folder_name.prepend(QDateTime::currentDateTime().toString("yyyy-MM-dd'T'HH-mm-ss"));
    }

    if (folder_name.length() == 0)
    {
        ui->folderNameLabel->setText(QString("Invalid folder name"));
        ui->folderNameLabel->setStyleSheet("QLabel { color : red; }");

        ui->fileExportPushButton->setDisabled(true);
    }

    else
    {
        ui->folderNameLabel->setText(folder_name);
        ui->folderNameLabel->setStyleSheet("");

        ui->fileExportPushButton->setDisabled(false);
    }
}

void MainWindow::on_deviceComboBox_currentIndexChanged(int index) {}

void MainWindow::on_startStopPushButton_clicked() {}

void MainWindow::on_gotoBeginningButton_clicked() {}

void MainWindow::on_gotoEndButton_clicked() {}

void MainWindow::on_moveLeftButton_pressed() {}

void MainWindow::on_moveLeftButton_released() {}

void MainWindow::on_moveRightButton_pressed() {}

void MainWindow::on_moveRightButton_released() {}

void MainWindow::on_histColourComboBox_currentIndexChanged(int index) {}

void MainWindow::on_filmTypeComboBox_currentIndexChanged(int index)
{
    switch (index)
    {
    // Auto (DXN) mode
    case 0:

        ui->filmColourComboBox->setDisabled(true);
        ui->wAdjustSlider->setDisabled(true);
        ui->rAdjustSlider->setDisabled(true);
        ui->gAdjustSlider->setDisabled(true);
        ui->bAdjustSlider->setDisabled(true);

        ui->filmColourComboBox->setVisible(false);
        ui->wAdjustSlider->setVisible(false);
        ui->rAdjustSlider->setVisible(false);
        ui->gAdjustSlider->setVisible(false);
        ui->bAdjustSlider->setVisible(false);
        break;

    // Manual mode
    case 1:
        ui->filmColourComboBox->setDisabled(false);
        ui->wAdjustSlider->setDisabled(false);
        ui->rAdjustSlider->setDisabled(false);
        ui->gAdjustSlider->setDisabled(false);
        ui->bAdjustSlider->setDisabled(false);

        ui->filmColourComboBox->setVisible(true);
        ui->wAdjustSlider->setVisible(true);
        ui->rAdjustSlider->setVisible(true);
        ui->gAdjustSlider->setVisible(true);
        ui->bAdjustSlider->setVisible(true);
        break;

    // Film selection mode
    default:
        ui->filmColourComboBox->setDisabled(true);
        ui->wAdjustSlider->setDisabled(false);
        ui->rAdjustSlider->setDisabled(false);
        ui->gAdjustSlider->setDisabled(false);
        ui->bAdjustSlider->setDisabled(false);

        ui->filmColourComboBox->setVisible(false);
        ui->wAdjustSlider->setVisible(true);
        ui->rAdjustSlider->setVisible(true);
        ui->gAdjustSlider->setVisible(true);
        ui->bAdjustSlider->setVisible(true);
        break;
    }
}

void MainWindow::on_filmColourComboBox_currentIndexChanged(int index) {}

void MainWindow::on_wAdjustSlider_valueChanged(int value) {}

void MainWindow::on_rAdjustSlider_valueChanged(int value) {}

void MainWindow::on_gAdjustSlider_valueChanged(int value) {}

void MainWindow::on_bAdjustSlider_valueChanged(int value) {}

void MainWindow::on_folderNameLineEdit_returnPressed() { updateFolderNamePreview(); }

void MainWindow::on_datetimeCheckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    updateFolderNamePreview();
}

void MainWindow::on_fileFormatComboBox_currentIndexChanged(int index) {}

void MainWindow::on_fileResComboBox_currentIndexChanged(int index) {}

void MainWindow::on_fileExportPushButton_clicked() {}

void MainWindow::on_folderNameLineEdit_textChanged(const QString &arg1)
{
    updateFolderNamePreview();
}
