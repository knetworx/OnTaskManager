find_package(GTest CONFIG REQUIRED)

add_executable(ontask_tests
  test_tracker.cpp
  test_storage.cpp
  test_providers.cpp
)

target_link_libraries(ontask_tests PRIVATE
  ontask_tracker
  ontask_storage
  GTest::gtest
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(ontask_tests)
