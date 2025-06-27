#ifndef THUMBNAILWIDGET_H
#define THUMBNAILWIDGET_H

#include <QLabel>
#include <QMouseEvent>
#include <QPixmap>
#include <QWidget>
#include <opencv2/opencv.hpp>

class ThumbnailWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit ThumbnailWidget(const cv::Mat &mat, int index, QWidget *parent = nullptr);
    ~ThumbnailWidget();

    // Getters
    int     getIndex() const { return m_index; }
    bool    isSelected() const { return m_selected; }
    cv::Mat getMat() const { return m_mat.clone(); }
    QPixmap getPixmap() const { return m_pixmap; }

    // Setters
    void setSelected(bool selected);
    void setThumbnailSize(int width, int height);

  signals:
    void thumbnailClicked(int index, bool multiSelect);
    void thumbnailDoubleClicked(int index);

  protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

  private:
    void    updateDisplay();
    QPixmap matToPixmap(const cv::Mat &mat);

    cv::Mat m_mat;
    int     m_index;
    bool    m_selected;
    QPixmap m_pixmap;
    QLabel *m_imageLabel;
    int     m_thumbnailWidth;
    int     m_thumbnailHeight;
};

#endif // THUMBNAILWIDGET_H