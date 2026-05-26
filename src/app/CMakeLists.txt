add_executable(OnTaskManager
  main.cpp
  Application.cpp
)

target_link_libraries(OnTaskManager PRIVATE
  ontask_tracker
  ontask_storage
  ontask_ui
)

set_target_properties(OnTaskManager PROPERTIES
  WIN32_EXECUTABLE FALSE
)
