#include "MainWindow.h"

#include "AssignCategoryDialog.h"
#include "TimelineBarDelegate.h"
#include "TimelineTreeModel.h"

#include <QAction>
#include <QCloseEvent>
#include <QDate>
#include <QDateEdit>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>

namespace ontask::ui {

MainWindow::MainWindow(storage::ActivityRepository& activities,
                       storage::CategoryRepository& categories,
                       storage::SampleRepository& samples,
                       QWidget* parent)
    : QMainWindow(parent),
      activities_(activities),
      categories_(categories),
      samples_(samples) {
    setWindowTitle(QStringLiteral("OnTaskManager"));
    resize(1100, 640);

    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);

    auto* topBar = new QHBoxLayout();
    topBar->addWidget(new QLabel(tr("Date:")));
    datePick_ = new QDateEdit(QDate::currentDate(), this);
    datePick_->setCalendarPopup(true);
    datePick_->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    topBar->addWidget(datePick_);
    topBar->addStretch(1);
    root->addLayout(topBar);

    tree_ = new QTreeView(this);
    tree_->setAlternatingRowColors(true);
    tree_->setUniformRowHeights(true);
    tree_->setContextMenuPolicy(Qt::CustomContextMenu);
    root->addWidget(tree_, 1);

    model_ = new TimelineTreeModel(activities_, categories_, samples_, this);
    tree_->setModel(model_);
    delegate_ = new TimelineBarDelegate(this);
    tree_->setItemDelegateForColumn(1, delegate_);

    tree_->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    tree_->header()->setStretchLastSection(true);
    tree_->setColumnWidth(0, 320);

    setCentralWidget(central);

    connect(datePick_, &QDateEdit::dateChanged, this, &MainWindow::onDateChanged);
    connect(tree_, &QTreeView::customContextMenuRequested,
            this, &MainWindow::onTreeContextMenu);

    refreshData();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    hide();
    event->ignore();
}

void MainWindow::refreshData() {
    model_->rebuild(datePick_->date());
    delegate_->setDayBounds(model_->dayStartMs(), model_->dayEndMs());
    tree_->expandAll();
    tree_->viewport()->update();
}

void MainWindow::onDateChanged() {
    refreshData();
}

void MainWindow::onTreeContextMenu(const QPoint& pos) {
    const QModelIndex idx = tree_->indexAt(pos);
    if (!idx.isValid()) return;

    const auto kind = static_cast<TimelineNode::Kind>(
        idx.siblingAtColumn(0).data(TimelineTreeModel::NodeKindRole).toInt());

    QMenu menu(this);
    if (kind == TimelineNode::Kind::ActivityPath) {
        auto* assign = menu.addAction(tr("Assign to category…"));
        connect(assign, &QAction::triggered, this, &MainWindow::assignSelectedActivity);
        tree_->setCurrentIndex(idx);
    } else {
        return;
    }
    menu.exec(tree_->viewport()->mapToGlobal(pos));
}

void MainWindow::assignSelectedActivity() {
    const QModelIndex idx = tree_->currentIndex();
    if (!idx.isValid()) return;

    const auto activityId = static_cast<std::int64_t>(
        idx.siblingAtColumn(0).data(TimelineTreeModel::ActivityIdRole).toLongLong());
    if (activityId == 0) return;

    const QString name = idx.siblingAtColumn(0).data(Qt::DisplayRole).toString();
    AssignCategoryDialog dlg(categories_, name, this);
    if (dlg.exec() != QDialog::Accepted) return;
    if (auto chosen = dlg.chosenCategoryId(); chosen.has_value()) {
        try {
            activities_.assignToCategory(activityId, *chosen);
            refreshData();
        } catch (const std::exception& e) {
            QMessageBox::warning(this, tr("Assignment failed"), QString::fromUtf8(e.what()));
        }
    }
}

} // namespace ontask::ui
