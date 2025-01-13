#pragma once

#include <QApplication>
#include <QDrag>
#include <QListView>
#include <QMimeData>
#include <QMouseEvent>

#include "common.h"
#include "itemdescriptiondelegate.h"

class ApplicationsView : public QListView
{
    Q_OBJECT
public:
    ApplicationsView(int iconSize, AppLayout::Layout layout = AppLayout::ListNameAndDescription, QWidget* parent = nullptr);
    ~ApplicationsView() = default;
    void setDelegateDefault(bool isDefault);
    void setAppLayout(int layout) { setAppLayout((AppLayout::Layout)layout); };
    void setAppLayout(AppLayout::Layout layout);
    //    QSize sizeHint() const override;
private:
    int mIconSize;
    bool mFavorites;
    AppLayout::Layout mAppLayout;
    ItemDescriptionDelegate* mDelegate;
    QPoint mDragStartPosition;
    bool mDidDrag;

signals:
    void indexClicked(const QModelIndex& index);

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dragMoveEvent(QDragMoveEvent* e) override;
    void dragLeaveEvent(QDragLeaveEvent* e) override;
    void dropEvent(QDropEvent* e) override;
};
