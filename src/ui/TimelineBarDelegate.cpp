#include "TimelineBarDelegate.h"

#include "TimelineTreeModel.h"

#include <QList>
#include <QPainter>
#include <QPair>

namespace ontask::ui {

namespace {

QColor barColorFor(TimelineNode::Kind kind) {
    switch (kind) {
        case TimelineNode::Kind::Idle:           return QColor(140, 140, 150);
        case TimelineNode::Kind::Uncategorized:  return QColor(170, 170, 170);
        case TimelineNode::Kind::Category:       return QColor(70, 130, 200);
        case TimelineNode::Kind::ActivityPath:   return QColor(90, 160, 110);
        default:                                 return QColor(120, 120, 120);
    }
}

} // namespace

TimelineBarDelegate::TimelineBarDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

void TimelineBarDelegate::setDayBounds(qint64 startMs, qint64 endMs) {
    dayStartMs_ = startMs;
    dayEndMs_   = endMs;
}

void TimelineBarDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                const QModelIndex& index) const {
    if (index.column() != 1) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    painter->save();
    painter->fillRect(option.rect, option.palette.base());

    // Hour gridlines.
    if (dayEndMs_ > dayStartMs_) {
        const qint64 span = dayEndMs_ - dayStartMs_;
        QPen grid(QColor(220, 220, 220));
        painter->setPen(grid);
        for (int h = 0; h <= 24; ++h) {
            const qint64 t = dayStartMs_ + (span * h) / 24;
            const double x = option.rect.left() +
                             (option.rect.width() * static_cast<double>(t - dayStartMs_)) /
                                 static_cast<double>(span);
            painter->drawLine(QPointF(x, option.rect.top()), QPointF(x, option.rect.bottom()));
        }
    }

    const auto rangesVariant = index.data(TimelineTreeModel::TimeRangesRole);
    const auto kindVariant   = index.data(TimelineTreeModel::NodeKindRole);
    if (rangesVariant.isValid() && dayEndMs_ > dayStartMs_) {
        const auto ranges = rangesVariant.value<QList<QPair<qint64, qint64>>>();
        const auto kind = static_cast<TimelineNode::Kind>(kindVariant.toInt());
        const QColor color = barColorFor(kind);

        painter->setPen(Qt::NoPen);
        painter->setBrush(color);

        const qint64 span = dayEndMs_ - dayStartMs_;
        const int barTop    = option.rect.top() + 4;
        const int barHeight = std::max(4, option.rect.height() - 8);

        for (const auto& r : ranges) {
            const qint64 a = std::max(r.first,  dayStartMs_);
            const qint64 b = std::min(r.second, dayEndMs_);
            if (b <= a) continue;
            const double x0 = option.rect.left() +
                              (option.rect.width() * static_cast<double>(a - dayStartMs_)) /
                                  static_cast<double>(span);
            const double x1 = option.rect.left() +
                              (option.rect.width() * static_cast<double>(b - dayStartMs_)) /
                                  static_cast<double>(span);
            const double w = std::max(1.0, x1 - x0);
            painter->drawRect(QRectF(x0, barTop, w, barHeight));
        }
    }

    painter->restore();
}

QSize TimelineBarDelegate::sizeHint(const QStyleOptionViewItem& option,
                                    const QModelIndex& index) const {
    if (index.column() == 1) {
        return QSize(600, std::max(22, QStyledItemDelegate::sizeHint(option, index).height()));
    }
    return QStyledItemDelegate::sizeHint(option, index);
}

} // namespace ontask::ui
