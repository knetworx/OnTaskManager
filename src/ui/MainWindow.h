#pragma once

#include "ActivityRepository.h"
#include "CategoryRepository.h"
#include "SampleRepository.h"

#include <QMainWindow>

class QDateEdit;
class QTreeView;
class QCloseEvent;

namespace ontask::ui {

class TimelineTreeModel;
class TimelineBarDelegate;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(storage::ActivityRepository& activities,
               storage::CategoryRepository& categories,
               storage::SampleRepository& samples,
               QWidget* parent = nullptr);

    void refreshData();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onDateChanged();
    void onTreeContextMenu(const QPoint& pos);

private:
    void assignSelectedActivity();

    storage::ActivityRepository& activities_;
    storage::CategoryRepository& categories_;
    storage::SampleRepository& samples_;

    QTreeView*           tree_      = nullptr;
    QDateEdit*           datePick_  = nullptr;
    TimelineTreeModel*   model_     = nullptr;
    TimelineBarDelegate* delegate_  = nullptr;
};

} // namespace ontask::ui
