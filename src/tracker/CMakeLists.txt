add_library(ontask_tracker STATIC
  ForegroundTracker.cpp
  IdleDetector.cpp
  providers/GenericProvider.cpp
  providers/ChromeProvider.cpp
  providers/VisualStudioProvider.cpp
)

target_include_directories(ontask_tracker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ontask_tracker PUBLIC user32)
