find_package(Qt6 CONFIG REQUIRED COMPONENTS Widgets)

add_library(ontask_ui STATIC
  TrayIcon.cpp
  MainWindow.cpp
  TimelineTreeModel.cpp
  TimelineBarDelegate.cpp
  AssignCategoryDialog.cpp
)

set_target_properties(ontask_ui PROPERTIES AUTOMOC ON)

target_include_directories(ontask_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ontask_ui
  PUBLIC  Qt6::Widgets ontask_storage ontask_tracker
)
