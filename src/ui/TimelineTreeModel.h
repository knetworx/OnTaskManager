#pragma once

#include "ActivityRepository.h"
#include "CategoryRepository.h"
#include "SampleRepository.h"

#include <QAbstractItemModel>
#include <QDate>
#include <QList>
#include <QMetaType>
#include <QPair>
#include <QString>

#include <cstdint>
#include <memory>
#include <vector>

namespace ontask::ui {

// Half-open [start_ms, end_ms) intervals. Aliased so the comma in the QPair
// doesn't get parsed as a macro-argument separator by Q_DECLARE_METATYPE below.
using TimeRangeList = QList<QPair<qint64, qint64>>;

struct TimelineNode {
    enum class Kind {
        CategoryRoot,      // top-level synthetic; container only
        Category,          // user-defined category node (CategoryRepository row)
        Uncategorized,     // synthetic root for activity nodes with no effective category
        Idle,              // synthetic single row for idle samples
        ActivityPath,      // activity node (ancestry path-context or directly mapped)
    };

    Kind kind = Kind::CategoryRoot;
    std::int64_t dbId = 0;          // category_node.id or activity_node.id (0 for synthetic)
    QString name;
    TimelineNode* parent = nullptr;
    std::vector<std::unique_ptr<TimelineNode>> children;
    // Half-open [start_ms, end_ms) intervals for the selected day, sorted.
    TimeRangeList timeRanges;
};

class TimelineTreeModel : public QAbstractItemModel {
    Q_OBJECT
public:
    static constexpr int TimeRangesRole = Qt::UserRole + 1;
    static constexpr int NodeKindRole   = Qt::UserRole + 2;
    static constexpr int ActivityIdRole = Qt::UserRole + 3;
    static constexpr int CategoryIdRole = Qt::UserRole + 4;

    TimelineTreeModel(storage::ActivityRepository& activities,
                      storage::CategoryRepository& categories,
                      storage::SampleRepository& samples,
                      QObject* parent = nullptr);

    void rebuild(const QDate& day);

    qint64 dayStartMs() const { return dayStartMs_; }
    qint64 dayEndMs()   const { return dayEndMs_; }

    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    TimelineNode* nodeFor(const QModelIndex& index) const;

    storage::ActivityRepository& activities_;
    storage::CategoryRepository& categories_;
    storage::SampleRepository& samples_;

    std::unique_ptr<TimelineNode> root_;
    qint64 dayStartMs_ = 0;
    qint64 dayEndMs_   = 0;
};

} // namespace ontask::ui

Q_DECLARE_METATYPE(ontask::ui::TimeRangeList)
