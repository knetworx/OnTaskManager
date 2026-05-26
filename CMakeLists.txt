cmake_minimum_required(VERSION 3.25)
project(OnTaskManager VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
  message(FATAL_ERROR "OnTaskManager targets Windows only.")
endif()

if(MSVC)
  add_compile_options(/W4 /permissive- /utf-8)
endif()

add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
