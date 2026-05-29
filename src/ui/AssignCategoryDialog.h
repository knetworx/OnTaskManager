#pragma once

#include "CategoryRepository.h"

#include <QDialog>

#include <cstdint>
#include <optional>

class QTreeWidget;
class QTreeWidgetItem;
class QLineEdit;

namespace ontask::ui {

class AssignCategoryDialog : public QDialog {
    Q_OBJECT
public:
    AssignCategoryDialog(storage::CategoryRepository& categories,
                         const QString& activityName, QWidget* parent = nullptr);

    std::optional<std::int64_t> chosenCategoryId() const { return chosen_; }

private slots:
    void onAccept();
    void onAddRoot();
    void onAddChild();

private:
    void reload();
    void populate(QTreeWidgetItem* parentItem, std::int64_t parentId);

    storage::CategoryRepository& categories_;
    QTreeWidget* tree_ = nullptr;
    QLineEdit*   nameEdit_ = nullptr;
    std::optional<std::int64_t> chosen_;
};

} // namespace ontask::ui
