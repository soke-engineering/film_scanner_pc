#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QCloseEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QMainWindow>
#include <QVBoxLayout>
#include <opencv2/opencv.hpp>

class ImageViewer : public QMainWindow
{
    Q_OBJECT

  public:
    explicit ImageViewer(const cv::Mat &image, QWidget *parent = nullptr);
    ~ImageViewer();

  signals:
    void viewerClosed();

  protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

  private:
    void    setupUI();
    void    displayImage();
    QPixmap matToPixmap(const cv::Mat &mat);

    cv::Mat m_image;
    QLabel *m_imageLabel;
    int     m_windowWidth;
    int     m_windowHeight;
};

#endif // IMAGEVIEWER_H