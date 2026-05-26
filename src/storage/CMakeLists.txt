find_package(unofficial-sqlite3 CONFIG REQUIRED)

add_library(ontask_storage STATIC
  Database.cpp
  ActivityRepository.cpp
)

target_include_directories(ontask_storage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ontask_storage
  PUBLIC  ontask_tracker
  PRIVATE unofficial::sqlite3::sqlite3
)
