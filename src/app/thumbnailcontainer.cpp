#include "thumbnailcontainer.h"
#include <QVBoxLayout>
#include <QApplication>
#include <QKeyEvent>
#include <QFocusEvent>

ThumbnailContainer::ThumbnailContainer(QWidget* parent)
    : QWidget(parent)
    , m_lastSelectedIndex(-1)
    , m_thumbnailsPerRow(5)
    , m_thumbnailWidth(150)
    , m_thumbnailHeight(100)
{
    // Create main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Create scroll area
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mainLayout->addWidget(m_scrollArea);
    
    // Create scroll content widget
    m_scrollContent = new QWidget(m_scrollArea);
    m_scrollArea->setWidget(m_scrollContent);
    
    // Create grid layout for thumbnails
    m_gridLayout = new QGridLayout(m_scrollContent);
    m_gridLayout->setSpacing(10);
    m_gridLayout->setContentsMargins(10, 10, 10, 10);
    
    // Set size policy
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Enable keyboard focus
    setFocusPolicy(Qt::StrongFocus);
    setTabOrder(this, m_scrollArea);
}

ThumbnailContainer::~ThumbnailContainer()
{
    clearThumbnails();
}

void ThumbnailContainer::addThumbnail(const cv::Mat& mat)
{
    int index = m_thumbnails.size();
    ThumbnailWidget* thumbnail = new ThumbnailWidget(mat, index, m_scrollContent);
    thumbnail->setThumbnailSize(m_thumbnailWidth, m_thumbnailHeight);
    
    // Connect signals
    connect(thumbnail, &ThumbnailWidget::thumbnailClicked, 
            this, &ThumbnailContainer::onThumbnailClicked);
    connect(thumbnail, &ThumbnailWidget::thumbnailDoubleClicked, 
            this, &ThumbnailContainer::onThumbnailDoubleClicked);
    
    m_thumbnails.append(thumbnail);
    
    // Add to grid layout
    int row = index / m_thumbnailsPerRow;
    int col = index % m_thumbnailsPerRow;
    m_gridLayout->addWidget(thumbnail, row, col);
}

void ThumbnailContainer::clearThumbnails()
{
    // Clear selection
    deselectAll();
    
    // Remove all thumbnails from layout and delete them
    for (ThumbnailWidget* thumbnail : m_thumbnails) {
        m_gridLayout->removeWidget(thumbnail);
        delete thumbnail;
    }
    m_thumbnails.clear();
}

void ThumbnailContainer::setThumbnailSize(int width, int height)
{
    m_thumbnailWidth = width;
    m_thumbnailHeight = height;
    
    for (ThumbnailWidget* thumbnail : m_thumbnails) {
        thumbnail->setThumbnailSize(width, height);
    }
    
    refreshLayout();
}

void ThumbnailContainer::selectThumbnail(int index, bool multiSelect)
{
    if (index < 0 || index >= m_thumbnails.size()) {
        return;
    }
    
    updateSelection(index, multiSelect);
}

void ThumbnailContainer::deselectAll()
{
    for (ThumbnailWidget* thumbnail : m_thumbnails) {
        thumbnail->setSelected(false);
    }
    m_selectedIndices.clear();
    m_lastSelectedIndex = -1;
    emit selectionChanged(m_selectedIndices);
}

QList<int> ThumbnailContainer::getSelectedIndices() const
{
    return m_selectedIndices;
}

QList<cv::Mat> ThumbnailContainer::getSelectedMats() const
{
    QList<cv::Mat> selectedMats;
    for (int index : m_selectedIndices) {
        if (index >= 0 && index < m_thumbnails.size()) {
            selectedMats.append(m_thumbnails[index]->getMat());
        }
    }
    return selectedMats;
}

cv::Mat ThumbnailContainer::getImageAtIndex(int index) const
{
    if (index >= 0 && index < m_thumbnails.size()) {
        return m_thumbnails[index]->getMat();
    }
    return cv::Mat();
}

void ThumbnailContainer::setThumbnailsPerRow(int count)
{
    if (count > 0 && count != m_thumbnailsPerRow) {
        m_thumbnailsPerRow = count;
        refreshLayout();
    }
}

void ThumbnailContainer::setThumbnailsPerRowFromWidth(int availableWidth, int minThumbnailWidth)
{
    // Calculate how many thumbnails can fit with minimum width
    int gridMargins = 20; // 10px left + 10px right margins
    int availableForThumbnails = availableWidth - gridMargins;
    
    // Calculate thumbnails per row based on minimum width and spacing
    int spacing = 10; // 10px spacing between thumbnails
    int maxThumbnails = (availableForThumbnails + spacing) / (minThumbnailWidth + spacing);
    
    // Ensure at least 1 thumbnail per row
    int newThumbnailsPerRow = qMax(1, maxThumbnails);
    
    if (newThumbnailsPerRow != m_thumbnailsPerRow) {
        m_thumbnailsPerRow = newThumbnailsPerRow;
        refreshLayout();
    }
}

void ThumbnailContainer::refreshLayout()
{
    // Reorganize thumbnails in the grid
    for (int i = 0; i < m_thumbnails.size(); ++i) {
        int row = i / m_thumbnailsPerRow;
        int col = i % m_thumbnailsPerRow;
        m_gridLayout->removeWidget(m_thumbnails[i]);
        m_gridLayout->addWidget(m_thumbnails[i], row, col);
    }
}

void ThumbnailContainer::onThumbnailClicked(int index, bool multiSelect)
{
    // Set focus to this container so it can receive keyboard events
    setFocus();
    updateSelection(index, multiSelect);
}

void ThumbnailContainer::onThumbnailDoubleClicked(int index)
{
    emit thumbnailDoubleClicked(index);
}

void ThumbnailContainer::updateSelection(int clickedIndex, bool multiSelect)
{
    QApplication::keyboardModifiers();
    bool shiftPressed = QApplication::keyboardModifiers() & Qt::ShiftModifier;
    
    if (shiftPressed && m_lastSelectedIndex >= 0) {
        // Shift+click: select range
        updateShiftSelection(clickedIndex);
    } else if (multiSelect) {
        // Ctrl/Cmd+click: toggle selection
        if (m_selectedIndices.contains(clickedIndex)) {
            m_selectedIndices.removeOne(clickedIndex);
            m_thumbnails[clickedIndex]->setSelected(false);
        } else {
            m_selectedIndices.append(clickedIndex);
            m_thumbnails[clickedIndex]->setSelected(true);
        }
    } else {
        // Single click: select only this thumbnail
        deselectAll();
        m_selectedIndices.append(clickedIndex);
        m_thumbnails[clickedIndex]->setSelected(true);
    }
    
    m_lastSelectedIndex = clickedIndex;
    emit selectionChanged(m_selectedIndices);
}

void ThumbnailContainer::updateShiftSelection(int clickedIndex)
{
    int start = qMin(m_lastSelectedIndex, clickedIndex);
    int end = qMax(m_lastSelectedIndex, clickedIndex);
    
    // Deselect all first
    deselectAll();
    
    // Select range
    for (int i = start; i <= end; ++i) {
        if (i >= 0 && i < m_thumbnails.size()) {
            m_selectedIndices.append(i);
            m_thumbnails[i]->setSelected(true);
        }
    }
}

void ThumbnailContainer::keyPressEvent(QKeyEvent* event)
{
    if (m_thumbnails.isEmpty()) {
        QWidget::keyPressEvent(event);
        return;
    }
    
    int currentIndex = m_lastSelectedIndex;
    if (currentIndex < 0) {
        currentIndex = 0;
    }
    
    int newIndex = currentIndex;
    bool shiftPressed = event->modifiers() & Qt::ShiftModifier;
    bool ctrlPressed = event->modifiers() & Qt::ControlModifier;
    
    switch (event->key()) {
        case Qt::Key_Left:
            if (currentIndex > 0) {
                newIndex = currentIndex - 1;
            }
            break;
            
        case Qt::Key_Right:
            if (currentIndex < m_thumbnails.size() - 1) {
                newIndex = currentIndex + 1;
            }
            break;
            
        case Qt::Key_Up:
            if (currentIndex >= m_thumbnailsPerRow) {
                newIndex = currentIndex - m_thumbnailsPerRow;
            }
            break;
            
        case Qt::Key_Down:
            if (currentIndex + m_thumbnailsPerRow < m_thumbnails.size()) {
                newIndex = currentIndex + m_thumbnailsPerRow;
            }
            break;
            
        case Qt::Key_Home:
            newIndex = 0;
            break;
            
        case Qt::Key_End:
            newIndex = m_thumbnails.size() - 1;
            break;
            
        case Qt::Key_PageUp:
            newIndex = qMax(0, currentIndex - (m_thumbnailsPerRow * 3));
            break;
            
        case Qt::Key_PageDown:
            newIndex = qMin(m_thumbnails.size() - 1, currentIndex + (m_thumbnailsPerRow * 3));
            break;
            
        case Qt::Key_Return:
        case Qt::Key_Enter:
            // Emit signal for Enter pressed on the currently selected thumbnail
            if (m_lastSelectedIndex >= 0 && m_lastSelectedIndex < m_thumbnails.size()) {
                emit enterPressedOnThumbnail(m_lastSelectedIndex);
            }
            break;
            
        default:
            QWidget::keyPressEvent(event);
            return;
    }
    
    if (newIndex != currentIndex && newIndex >= 0 && newIndex < m_thumbnails.size()) {
        if (shiftPressed && m_lastSelectedIndex >= 0) {
            // Shift + arrow: extend selection
            updateShiftSelection(newIndex);
        } else if (ctrlPressed) {
            // Ctrl + arrow: move selection without changing current selection
            m_lastSelectedIndex = newIndex;
            // Scroll to make the thumbnail visible
            m_scrollArea->ensureWidgetVisible(m_thumbnails[newIndex]);
        } else {
            // Regular arrow: select only the new thumbnail
            deselectAll();
            m_selectedIndices.append(newIndex);
            m_thumbnails[newIndex]->setSelected(true);
            m_lastSelectedIndex = newIndex;
            m_scrollArea->ensureWidgetVisible(m_thumbnails[newIndex]);
            emit selectionChanged(m_selectedIndices);
        }
        
        // Ensure this container keeps focus
        setFocus();
    }
    
    event->accept();
}

void ThumbnailContainer::focusInEvent(QFocusEvent* event)
{
    QWidget::focusInEvent(event);
    
    // If no thumbnail is selected, select the first one
    if (m_selectedIndices.isEmpty() && !m_thumbnails.isEmpty()) {
        deselectAll();
        m_selectedIndices.append(0);
        m_thumbnails[0]->setSelected(true);
        m_lastSelectedIndex = 0;
        emit selectionChanged(m_selectedIndices);
    }
    
    // Ensure the selected thumbnail is visible
    if (m_lastSelectedIndex >= 0 && m_lastSelectedIndex < m_thumbnails.size()) {
        m_scrollArea->ensureWidgetVisible(m_thumbnails[m_lastSelectedIndex]);
    }
} 