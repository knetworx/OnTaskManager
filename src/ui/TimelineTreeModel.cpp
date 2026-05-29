#include "TimelineTreeModel.h"

#include <QDateTime>
#include <QTimeZone>

#include <algorithm>
#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace ontask::ui {

namespace {

constexpr qint64 kSampleIntervalMs = 15'000;
constexpr qint64 kClusterGapMs     = 30'000; // samples within this gap merge into one block

QString fromWString(const std::wstring& w) {
    return QString::fromWCharArray(w.data(), static_cast<int>(w.size()));
}

QList<QPair<qint64, qint64>> clusterSamples(std::vector<qint64> sortedTimestamps) {
    QList<QPair<qint64, qint64>> out;
    if (sortedTimestamps.empty()) return out;
    std::sort(sortedTimestamps.begin(), sortedTimestamps.end());
    qint64 blockStart = sortedTimestamps.front();
    qint64 blockEnd   = blockStart + kSampleIntervalMs;
    for (std::size_t i = 1; i < sortedTimestamps.size(); ++i) {
        const qint64 t = sortedTimestamps[i];
        if (t - (blockEnd - kSampleIntervalMs) <= kClusterGapMs) {
            blockEnd = t + kSampleIntervalMs;
        } else {
            out.append({blockStart, blockEnd});
            blockStart = t;
            blockEnd   = t + kSampleIntervalMs;
        }
    }
    out.append({blockStart, blockEnd});
    return out;
}

QList<QPair<qint64, qint64>> mergeRanges(const QList<QPair<qint64, qint64>>& a,
                                         const QList<QPair<qint64, qint64>>& b) {
    QList<QPair<qint64, qint64>> all = a;
    all.append(b);
    std::sort(all.begin(), all.end(),
              [](const auto& x, const auto& y) { return x.first < y.first; });
    QList<QPair<qint64, qint64>> out;
    for (const auto& r : all) {
        if (!out.isEmpty() && r.first <= out.back().second) {
            out.back().second = std::max(out.back().second, r.second);
        } else {
            out.append(r);
        }
    }
    return out;
}

void aggregateTimeRangesPostOrder(TimelineNode* n) {
    QList<QPair<qint64, qint64>> agg = n->timeRanges;
    for (auto& c : n->children) {
        aggregateTimeRangesPostOrder(c.get());
        agg = mergeRanges(agg, c->timeRanges);
    }
    n->timeRanges = std::move(agg);
}

} // namespace

TimelineTreeModel::TimelineTreeModel(storage::ActivityRepository& activities,
                                     storage::CategoryRepository& categories,
                                     storage::SampleRepository& samples,
                                     QObject* parent)
    : QAbstractItemModel(parent),
      activities_(activities),
      categories_(categories),
      samples_(samples),
      root_(std::make_unique<TimelineNode>()) {
    root_->kind = TimelineNode::Kind::CategoryRoot;
}

void TimelineTreeModel::rebuild(const QDate& day) {
    beginResetModel();

    const QDateTime startLocal(day, QTime(0, 0), QTimeZone::LocalTime);
    const QDateTime endLocal  (day.addDays(1), QTime(0, 0), QTimeZone::LocalTime);
    dayStartMs_ = startLocal.toMSecsSinceEpoch();
    dayEndMs_   = endLocal.toMSecsSinceEpoch();

    auto rows = samples_.samplesInRange(
        std::chrono::system_clock::time_point{std::chrono::milliseconds{dayStartMs_}},
        std::chrono::system_clock::time_point{std::chrono::milliseconds{dayEndMs_}});

    std::unordered_map<std::int64_t, std::vector<qint64>> samplesByActivity;
    std::vector<qint64> idleStamps;
    std::unordered_set<std::int64_t> seenActivityIds;
    for (const auto& row : rows) {
        const qint64 ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              row.timestamp.time_since_epoch())
                              .count();
        if (row.isIdle) {
            idleStamps.push_back(ms);
        } else if (row.activityNodeId.has_value()) {
            samplesByActivity[*row.activityNodeId].push_back(ms);
            seenActivityIds.insert(*row.activityNodeId);
        }
    }

    // Pre-compute effective categories for every activity node we saw today, plus
    // any ancestor chain those walks traverse.
    std::unordered_map<std::int64_t, std::int64_t> effectiveCat;
    for (const auto id : seenActivityIds) {
        effectiveCat[id] = activities_.effectiveCategoryOrZero(id);
    }

    // Resolve full ancestry of every seen activity node so we can rebuild the
    // path-context tree under each category.
    std::unordered_map<std::int64_t, storage::ActivityNode> nodeCache;
    auto fetchNode = [&](std::int64_t id) -> storage::ActivityNode* {
        auto it = nodeCache.find(id);
        if (it != nodeCache.end()) return &it->second;
        auto n = activities_.node(id);
        if (!n) return nullptr;
        return &(nodeCache[id] = std::move(*n));
    };

    // Builds a subtree under `parent` showing the activity-tree ancestry that
    // contains any seen leaf with effectiveCategory == targetCategoryId.
    // `targetCategoryId == 0` means Uncategorized.
    auto buildActivitySubtree = [&](TimelineNode* parent, std::int64_t targetCategoryId) {
        // Which seen activity nodes belong to this category?
        std::unordered_set<std::int64_t> belongingLeaves;
        for (const auto& [id, _] : samplesByActivity) {
            if (effectiveCat[id] == targetCategoryId) belongingLeaves.insert(id);
        }
        if (belongingLeaves.empty()) return;

        // Collect all ancestor ids that must appear in this category's subtree.
        // An ancestor is included if any descendant in `belongingLeaves` shares
        // that ancestor AND no closer ancestor of that descendant is mapped to
        // a different category.
        std::unordered_set<std::int64_t> includedNodes;
        for (const auto leaf : belongingLeaves) {
            std::int64_t cur = leaf;
            includedNodes.insert(cur);
            while (true) {
                auto* n = fetchNode(cur);
                if (!n || !n->parentId.has_value()) break;
                const std::int64_t parentId = *n->parentId;
                // Stop if a closer ancestor of `leaf` has a *direct* mapping to a
                // different category (it would render under that category instead).
                if (auto direct = activities_.directMapping(parentId);
                    direct.has_value() && *direct != targetCategoryId) {
                    break;
                }
                includedNodes.insert(parentId);
                cur = parentId;
            }
        }

        // For each included node, find its parent within the included set (or
        // null if it's a root within this category's view).
        std::unordered_map<std::int64_t, TimelineNode*> placed;
        std::function<TimelineNode*(std::int64_t)> place = [&](std::int64_t id) -> TimelineNode* {
            if (auto it = placed.find(id); it != placed.end()) return it->second;
            auto* node = fetchNode(id);
            if (!node) return nullptr;

            TimelineNode* parentTimelineNode = parent;
            if (node->parentId.has_value() && includedNodes.count(*node->parentId)) {
                parentTimelineNode = place(*node->parentId);
                if (!parentTimelineNode) parentTimelineNode = parent;
            }

            auto fresh = std::make_unique<TimelineNode>();
            fresh->kind = TimelineNode::Kind::ActivityPath;
            fresh->dbId = id;
            fresh->name = fromWString(node->name);
            fresh->parent = parentTimelineNode;
            // Only own samples — descendants will roll up later in postorder.
            if (auto sIt = samplesByActivity.find(id); sIt != samplesByActivity.end()) {
                fresh->timeRanges = clusterSamples(sIt->second);
            }
            TimelineNode* raw = fresh.get();
            parentTimelineNode->children.push_back(std::move(fresh));
            placed[id] = raw;
            return raw;
        };
        for (const auto id : includedNodes) {
            place(id);
        }
    };

    // Wipe and rebuild the tree.
    root_ = std::make_unique<TimelineNode>();
    root_->kind = TimelineNode::Kind::CategoryRoot;

    // Categories (recursive).
    std::function<void(TimelineNode*, const storage::CategoryNode&)> buildCategory =
        [&](TimelineNode* parent, const storage::CategoryNode& cat) {
            auto node = std::make_unique<TimelineNode>();
            node->kind = TimelineNode::Kind::Category;
            node->dbId = cat.id;
            node->name = fromWString(cat.name);
            node->parent = parent;
            TimelineNode* raw = node.get();
            parent->children.push_back(std::move(node));
            for (const auto& child : categories_.children(cat.id)) {
                buildCategory(raw, child);
            }
            buildActivitySubtree(raw, cat.id);
        };
    for (const auto& cat : categories_.rootNodes()) {
        buildCategory(root_.get(), cat);
    }

    // Uncategorized.
    {
        auto node = std::make_unique<TimelineNode>();
        node->kind = TimelineNode::Kind::Uncategorized;
        node->name = QStringLiteral("Uncategorized");
        node->parent = root_.get();
        TimelineNode* raw = node.get();
        root_->children.push_back(std::move(node));
        buildActivitySubtree(raw, 0);
    }

    // Idle.
    {
        auto node = std::make_unique<TimelineNode>();
        node->kind = TimelineNode::Kind::Idle;
        node->name = QStringLiteral("Idle");
        node->parent = root_.get();
        node->timeRanges = clusterSamples(std::move(idleStamps));
        root_->children.push_back(std::move(node));
    }

    // Roll up time ranges from descendants into each row.
    for (auto& c : root_->children) {
        aggregateTimeRangesPostOrder(c.get());
    }

    endResetModel();
}

QModelIndex TimelineTreeModel::index(int row, int column, const QModelIndex& parent) const {
    if (!hasIndex(row, column, parent)) return {};
    TimelineNode* p = nodeFor(parent);
    if (!p) p = root_.get();
    if (row < 0 || row >= static_cast<int>(p->children.size())) return {};
    return createIndex(row, column, p->children[row].get());
}

QModelIndex TimelineTreeModel::parent(const QModelIndex& child) const {
    if (!child.isValid()) return {};
    auto* node = static_cast<TimelineNode*>(child.internalPointer());
    if (!node || !node->parent || node->parent == root_.get()) return {};
    auto* grandparent = node->parent->parent;
    if (!grandparent) grandparent = root_.get();
    int row = 0;
    for (const auto& sib : grandparent->children) {
        if (sib.get() == node->parent) {
            return createIndex(row, 0, node->parent);
        }
        ++row;
    }
    return {};
}

int TimelineTreeModel::rowCount(const QModelIndex& parent) const {
    TimelineNode* p = nodeFor(parent);
    if (!p) p = root_.get();
    return static_cast<int>(p->children.size());
}

int TimelineTreeModel::columnCount(const QModelIndex&) const {
    return 2;
}

QVariant TimelineTreeModel::data(const QModelIndex& index, int role) const {
    auto* node = nodeFor(index);
    if (!node) return {};
    if (index.column() == 0 && role == Qt::DisplayRole) {
        return node->name;
    }
    if (role == TimeRangesRole) {
        return QVariant::fromValue(node->timeRanges);
    }
    if (role == NodeKindRole) {
        return static_cast<int>(node->kind);
    }
    if (role == ActivityIdRole && node->kind == TimelineNode::Kind::ActivityPath) {
        return QVariant::fromValue(static_cast<qlonglong>(node->dbId));
    }
    if (role == CategoryIdRole && node->kind == TimelineNode::Kind::Category) {
        return QVariant::fromValue(static_cast<qlonglong>(node->dbId));
    }
    return {};
}

QVariant TimelineTreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    if (section == 0) return QStringLiteral("Activity / Category");
    if (section == 1) return QStringLiteral("Timeline");
    return {};
}

TimelineNode* TimelineTreeModel::nodeFor(const QModelIndex& index) const {
    if (!index.isValid()) return nullptr;
    return static_cast<TimelineNode*>(index.internalPointer());
}

} // namespace ontask::ui
