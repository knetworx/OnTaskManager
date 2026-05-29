#pragma once

#include <QStyledItemDelegate>

namespace ontask::ui {

class TimelineBarDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit TimelineBarDelegate(QObject* parent = nullptr);

    void setDayBounds(qint64 startMs, qint64 endMs);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

private:
    qint64 dayStartMs_ = 0;
    qint64 dayEndMs_   = 0;
};

} // namespace ontask::ui
