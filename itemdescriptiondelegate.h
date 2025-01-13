#pragma once

#include <QStyledItemDelegate>

#include "common.h"

class ItemDescriptionDelegate : public QStyledItemDelegate
{
    using QStyledItemDelegate::QStyledItemDelegate;

public:
    void setAppLayout(AppLayout::Layout);

    AppLayout::Layout mAppLayout;

private:
    void paint(QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};
