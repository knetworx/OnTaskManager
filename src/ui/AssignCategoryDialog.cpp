#include "AssignCategoryDialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace ontask::ui {

namespace {

QString fromWString(const std::wstring& w) {
    return QString::fromWCharArray(w.data(), static_cast<int>(w.size()));
}

constexpr int kIdRole = Qt::UserRole + 1;

} // namespace

AssignCategoryDialog::AssignCategoryDialog(storage::CategoryRepository& categories,
                                           const QString& activityName, QWidget* parent)
    : QDialog(parent), categories_(categories) {
    setWindowTitle(tr("Assign \"%1\" to category").arg(activityName));
    resize(420, 460);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Pick a category for: <b>%1</b>").arg(activityName)));

    tree_ = new QTreeWidget(this);
    tree_->setHeaderLabel(tr("Categories"));
    layout->addWidget(tree_, 1);

    auto* nameRow = new QHBoxLayout();
    nameEdit_ = new QLineEdit(this);
    nameEdit_->setPlaceholderText(tr("New category name"));
    nameRow->addWidget(nameEdit_, 1);
    auto* addRootBtn  = new QPushButton(tr("Add root"), this);
    auto* addChildBtn = new QPushButton(tr("Add child of selected"), this);
    nameRow->addWidget(addRootBtn);
    nameRow->addWidget(addChildBtn);
    layout->addLayout(nameRow);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);

    connect(addRootBtn,  &QPushButton::clicked, this, &AssignCategoryDialog::onAddRoot);
    connect(addChildBtn, &QPushButton::clicked, this, &AssignCategoryDialog::onAddChild);
    connect(buttons, &QDialogButtonBox::accepted, this, &AssignCategoryDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    reload();
}

void AssignCategoryDialog::reload() {
    tree_->clear();
    populate(nullptr, 0);
    tree_->expandAll();
}

void AssignCategoryDialog::populate(QTreeWidgetItem* parentItem, std::int64_t parentId) {
    const auto kids = (parentId == 0)
        ? categories_.rootNodes()
        : categories_.children(parentId);
    for (const auto& c : kids) {
        auto* item = parentItem
            ? new QTreeWidgetItem(parentItem)
            : new QTreeWidgetItem(tree_);
        item->setText(0, fromWString(c.name));
        item->setData(0, kIdRole, QVariant::fromValue(static_cast<qlonglong>(c.id)));
        populate(item, c.id);
    }
}

void AssignCategoryDialog::onAddRoot() {
    const QString name = nameEdit_->text().trimmed();
    if (name.isEmpty()) return;
    try {
        std::wstring w(reinterpret_cast<const wchar_t*>(name.utf16()), name.size());
        categories_.createRoot(w);
        nameEdit_->clear();
        reload();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, tr("Add category failed"), QString::fromUtf8(e.what()));
    }
}

void AssignCategoryDialog::onAddChild() {
    auto* sel = tree_->currentItem();
    if (!sel) {
        QMessageBox::information(this, tr("No parent selected"),
                                 tr("Select a category to add the child under."));
        return;
    }
    const QString name = nameEdit_->text().trimmed();
    if (name.isEmpty()) return;
    const auto parentId = static_cast<std::int64_t>(sel->data(0, kIdRole).toLongLong());
    try {
        std::wstring w(reinterpret_cast<const wchar_t*>(name.utf16()), name.size());
        categories_.createChild(parentId, w);
        nameEdit_->clear();
        reload();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, tr("Add category failed"), QString::fromUtf8(e.what()));
    }
}

void AssignCategoryDialog::onAccept() {
    auto* sel = tree_->currentItem();
    if (!sel) {
        QMessageBox::information(this, tr("No category selected"),
                                 tr("Select a category to assign to."));
        return;
    }
    chosen_ = static_cast<std::int64_t>(sel->data(0, kIdRole).toLongLong());
    accept();
}

} // namespace ontask::ui
