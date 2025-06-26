#ifndef THUMBNAILCONTAINER_H
#define THUMBNAILCONTAINER_H

#include <QWidget>
#include <QScrollArea>
#include <QGridLayout>
#include <QList>
#include <opencv2/opencv.hpp>
#include "imageviewer.h"
#include "thumbnailwidget.h"

QT_BEGIN_NAMESPACE
class QScrollArea;
class QGridLayout;
QT_END_NAMESPACE

class ThumbnailContainer : public QWidget
{
    Q_OBJECT

public:
    explicit ThumbnailContainer(QWidget* parent = nullptr);
    ~ThumbnailContainer();

    // Thumbnail management
    void addThumbnail(const cv::Mat& mat);
    void clearThumbnails();
    void setThumbnailSize(int width, int height);
    
    // Selection management
    void selectThumbnail(int index, bool multiSelect = false);
    void deselectAll();
    QList<int> getSelectedIndices() const;
    QList<cv::Mat> getSelectedMats() const;
    cv::Mat getImageAtIndex(int index) const;
    int getThumbnailsPerRow() const { return m_thumbnailsPerRow; }
    
    // Layout management
    void setThumbnailsPerRow(int count);
    void setThumbnailsPerRowFromWidth(int availableWidth, int minThumbnailWidth);
    void refreshLayout();

signals:
    void selectionChanged(const QList<int>& selectedIndices);
    void thumbnailDoubleClicked(int index);
    void openImage(const cv::Mat& image);
    void enterPressedOnThumbnail(int index);

private slots:
    void onThumbnailClicked(int index, bool multiSelect);
    void onThumbnailDoubleClicked(int index);

private:
    void updateSelection(int clickedIndex, bool multiSelect);
    void updateShiftSelection(int clickedIndex);

    QScrollArea* m_scrollArea;
    QWidget* m_scrollContent;
    QGridLayout* m_gridLayout;
    QList<ThumbnailWidget*> m_thumbnails;
    QList<int> m_selectedIndices;
    int m_lastSelectedIndex;
    int m_thumbnailsPerRow;
    int m_thumbnailWidth;
    int m_thumbnailHeight;

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
};

#endif // THUMBNAILCONTAINER_H 