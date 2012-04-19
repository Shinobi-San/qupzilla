/* ============================================================
* QupZilla - WebKit based browser
* Copyright (C) 2010-2012  David Rosca <nowrep@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
#include "locationcompleterdelegate.h"

#include <QPainter>
#include <QTreeView>
#include <QApplication>
#include <QMouseEvent>

#include <QDebug>

CompleterTreeView::CompleterTreeView(QWidget* parent)
    : QTreeView(parent)
    , m_selectedItemByMousePosition(false)
    , m_rowHeight(0)
{
    setMouseTracking(true);
}

bool CompleterTreeView::ignoreSelectedFlag() const
{
    return m_selectedItemByMousePosition;
}

int CompleterTreeView::rowHeight() const
{
    return m_rowHeight;
}

void CompleterTreeView::setRowHeight(int height)
{
    m_rowHeight = height;
}

void CompleterTreeView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    m_selectedItemByMousePosition = false;
    m_lastMouseIndex = current;

    QTreeView::currentChanged(current, previous);

    viewport()->repaint();
}

void CompleterTreeView::mouseMoveEvent(QMouseEvent* event)
{
    QModelIndex last = m_lastMouseIndex;
    QModelIndex atCursor = indexAt(mapFromGlobal(QCursor::pos()));

    if (atCursor.isValid()) {
        m_lastMouseIndex = atCursor;
        m_selectedItemByMousePosition = true;
    }

    if (last != atCursor) {
        viewport()->repaint();
    }

    QTreeView::mouseMoveEvent(event);
}

void CompleterTreeView::keyPressEvent(QKeyEvent* event)
{
    if (currentIndex() != m_lastMouseIndex) {
        setCurrentIndex(m_lastMouseIndex);
    }

    QTreeView::keyPressEvent(event);
}

LocationCompleterDelegate::LocationCompleterDelegate(CompleterTreeView* parent)
    : QStyledItemDelegate(parent)
    , m_rowHeight(0)
    , m_padding(0)
    , m_treeView(parent)
{
}

void LocationCompleterDelegate::paint(QPainter* painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);

    const QWidget* w = opt.widget;
    const QStyle* style = w ? w->style() : QApplication::style();
    const int height = opt.rect.height();
    const int center = height / 2 + opt.rect.top();

    // Prepare title font
    QFont titleFont = opt.font;
    titleFont.setPointSize(titleFont.pointSize() + 1);

    const QFontMetrics titleMetrics(titleFont);

    int leftPosition = m_padding * 2;
    int rightPosition = opt.rect.right() - m_padding;

    if (m_treeView->ignoreSelectedFlag()) {
        if (opt.state.testFlag(QStyle::State_MouseOver)) {
            opt.state |= QStyle::State_Selected;
        }
        else {
            opt.state &= ~QStyle::State_Selected;
        }
    }

    const QPalette::ColorRole colorRole = opt.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Text;
    const QPalette::ColorRole colorLinkRole = opt.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Link;

    // Draw background
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, w);

    // Draw icon
    const int iconSize = 16;
    const int iconYPos = center - (iconSize / 2);
    QRect iconRect(leftPosition, iconYPos, iconSize, iconSize);
    QPixmap pixmap = index.data(Qt::DecorationRole).value<QIcon>().pixmap(iconSize);
    painter->drawPixmap(iconRect, pixmap);
    leftPosition = iconRect.right() + m_padding * 2;

    // Draw title
    const int leftTitleEdge = leftPosition + 2;
    const int rightTitleEdge = rightPosition - m_padding;
    QRect titleRect(leftTitleEdge, opt.rect.top() + m_padding, rightTitleEdge - leftTitleEdge, titleMetrics.height());
    QString title(titleMetrics.elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideRight, titleRect.width()));
    painter->setFont(titleFont);
    style->drawItemText(painter, titleRect, Qt::AlignLeft | Qt::TextSingleLine, opt.palette, true, title, colorRole);

    // Draw link
    const int infoYPos = titleRect.bottom() + opt.fontMetrics.leading();
    QRect linkRect(titleRect.x(), infoYPos, titleRect.width(), opt.fontMetrics.height());
    const QString &link = opt.fontMetrics.elidedText(index.data(Qt::UserRole).toString(), Qt::ElideRight, linkRect.width());
    painter->setFont(opt.font);
    style->drawItemText(painter, linkRect, Qt::TextSingleLine | Qt::AlignLeft, opt.palette, true, link, colorLinkRole);
}

QSize LocationCompleterDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)

    if (!m_rowHeight) {
        QStyleOptionViewItemV4 opt(option);
        initStyleOption(&opt, index);

        const QWidget* w = opt.widget;
        const QStyle* style = w ? w->style() : QApplication::style();
        const int padding = style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0) + 1;

        QFont titleFont = opt.font;
        titleFont.setPointSize(titleFont.pointSize() + 1);

        m_padding = padding > 3 ? padding : 3;

        const QFontMetrics titleMetrics(titleFont);

        m_rowHeight = 2 * m_padding + opt.fontMetrics.leading() + opt.fontMetrics.height() + titleMetrics.height();

        m_treeView->setRowHeight(m_rowHeight);
        m_treeView->setMaximumHeight(6 * m_rowHeight);
    }

    return QSize(200, m_rowHeight);
}
