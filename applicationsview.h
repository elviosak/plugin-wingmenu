/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt-project.org/
 *
 * Copyright: 2021 LXQt team
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#pragma once

#include <QApplication>
#include <QDrag>
#include <QListView>
#include <QMouseEvent>
#include <QMimeData>

#include "itemdescriptiondelegate.h"
#include "common.h"

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
