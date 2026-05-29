find_package(Qt6 CONFIG REQUIRED COMPONENTS Widgets Core Gui)

add_executable(OnTaskManager
  main.cpp
  Application.cpp
)

set_target_properties(OnTaskManager PROPERTIES
  AUTOMOC ON
  WIN32_EXECUTABLE TRUE
)

target_link_libraries(OnTaskManager PRIVATE
  ontask_tracker
  ontask_storage
  ontask_ui
  Qt6::Widgets
)

# Deploy Qt plugins next to the exe so the app launches without the vcpkg tree
# on PATH. vcpkg's applocal pass copies the Qt *DLLs* automatically, but NOT the
# plugins, and the vcpkg qtbase port does not ship windeployqt — so copy the
# platform (qwindows) and style plugins ourselves, picking the debug vs release
# tree and the trailing 'd' debug suffix by config. Without the platform plugin
# Qt aborts at startup with "no Qt platform plugin could be initialized".
if(WIN32)
  set(_ontask_qt_plugins
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}$<$<CONFIG:Debug>:/debug>/Qt6/plugins")
  set(_ontask_qt_dsuffix "$<$<CONFIG:Debug>:d>")
  add_custom_command(TARGET OnTaskManager POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
      "$<TARGET_FILE_DIR:OnTaskManager>/platforms"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${_ontask_qt_plugins}/platforms/qwindows${_ontask_qt_dsuffix}.dll"
      "$<TARGET_FILE_DIR:OnTaskManager>/platforms/"
    COMMAND ${CMAKE_COMMAND} -E make_directory
      "$<TARGET_FILE_DIR:OnTaskManager>/styles"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${_ontask_qt_plugins}/styles/qmodernwindowsstyle${_ontask_qt_dsuffix}.dll"
      "$<TARGET_FILE_DIR:OnTaskManager>/styles/"
    # Image-format plugins. PNG is compiled into QtGui core (no plugin), but
    # jpeg/gif/ico ship as plugins and must sit beside the exe to be loaded.
    COMMAND ${CMAKE_COMMAND} -E make_directory
      "$<TARGET_FILE_DIR:OnTaskManager>/imageformats"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${_ontask_qt_plugins}/imageformats/qjpeg${_ontask_qt_dsuffix}.dll"
      "${_ontask_qt_plugins}/imageformats/qgif${_ontask_qt_dsuffix}.dll"
      "${_ontask_qt_plugins}/imageformats/qico${_ontask_qt_dsuffix}.dll"
      "$<TARGET_FILE_DIR:OnTaskManager>/imageformats/"
    VERBATIM
  )
endif()
