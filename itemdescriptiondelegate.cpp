#include "itemdescriptiondelegate.h"

#include <QApplication>
#include <QPainter>
#include <QtMath>

#include <QDebug>

void ItemDescriptionDelegate::setAppLayout(AppLayout::Layout appLayout)
{
    mAppLayout = appLayout;
}

void ItemDescriptionDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    // Icons and regular list are painted with default settings
    if (mAppLayout != AppLayout::ListNameAndDescription) {
        return QStyledItemDelegate::paint(painter, option, index);
    }

    // Saving and restoring painter is needed because the
    // same painter is used for the next items
    painter->save();
    auto rect = QRect(option.rect);
    auto font = painter->font();

    //     Calculations for item layout
    //     Spacing s = 1/3 of "font height"
    //
    //      s|3s|s|3s|s|   x    |s
    //      ______________________
    //   s |  _______   ________  |
    //  3s | |       | |__text__| |
    //   s | | icon  |  ________  |
    //  3s | |_______| |__text__| |
    //   s |______________________|

    QFontMetrics fm(painter->font());
    int fontHeight = fm.height();
    int spacing = qFloor(fontHeight / 3);
    int iconX = rect.x() + spacing;
    int iconY = rect.y() + spacing;
    int iconWidth = 2 * fontHeight + spacing;
    QSize iconSize(iconWidth, iconWidth);
    QRect iconRect(QPoint(iconX, iconY), iconSize);
    int textX = iconX + 2 * fontHeight + 2 * spacing;
    int textY1 = iconY;
    int textY2 = textY1 + fontHeight + spacing;
    int textWidth = rect.width() - iconWidth - 3 * spacing;
    QSize textSize(textWidth, fontHeight);
    QRect textTopRect(QPoint(textX, textY1), textSize);
    QRect textBotRect(QPoint(textX, textY2), textSize);

    QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter);
    QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewRow, &option, painter);
    auto icon = index.data(Qt::DecorationRole).value<QIcon>();
    QApplication::style()->drawItemPixmap(
        painter, iconRect, Qt::AlignmentFlag::AlignCenter, icon.pixmap(iconSize));

    QString textBot = fm.elidedText(index.data(Qt::UserRole + 1).toString(), Qt::ElideRight, textBotRect.width());
    QApplication::style()->drawItemText(
        painter, textBotRect, Qt::AlignLeft, QApplication::palette(), true, textBot);
    font.setBold(true);
    painter->setFont(font);
    QFontMetrics fmTop(painter->font());
    QString textTop = fmTop.elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideRight, textTopRect.width());
    QApplication::style()->drawItemText(
        painter, textTopRect, Qt::AlignLeft, QApplication::palette(), true, textTop);
    painter->restore();
}

QSize ItemDescriptionDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (mAppLayout == AppLayout::Icons) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    auto oldSize = QStyledItemDelegate::sizeHint(option, index);
    auto fm = option.fontMetrics;
    int fontHeight = fm.height();

    // When layout is ListNameOnly the height is "fixed" so it doesn't shrink when there's no icon
    if (mAppLayout == AppLayout::ListNameOnly) {
        return QSize(oldSize.width(), 2 * fontHeight);
    }

    int spacing = qFloor(fontHeight / 3);
    // We consider 2  * "font height" as the width alocated by Qt for icon and padding,
    // then increase the width to account for the extra spacing and bigger icon
    // to keep the same amount of text displayed as the default icon+text button.
    //
    // so  oldwidth = text + 2 * fontHeight
    // and newWidth = text + 2 * fontHeight + 4 * spacing
    //              =        oldwidth       + 4 * spacing
    // height is 2 lines of text and 3 spacing
    QSize newSize(oldSize.width() + 4 * spacing, fontHeight * 2 + spacing * 3);
    return newSize;
}
