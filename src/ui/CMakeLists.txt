add_library(ontask_ui STATIC
  TrayIcon.cpp
  MainWindow.cpp
)

target_include_directories(ontask_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ontask_ui PUBLIC user32 shell32 gdi32)
