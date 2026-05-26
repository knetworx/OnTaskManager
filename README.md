# OnTaskManager

Activity/productivity-tracking app for Windows. This is a testbed for using Claude Code (specifically when I'm not at my computer.) Initial scaffolding set up purely through Claude in the browser. Will eventually get this up and running in VSCode for manual/proper dev and fine-tuning.

## Layout

```
src/
  tracker/   foreground window + idle detection (Win32)
  storage/   SQLite persistence
  ui/        tray icon + main window
  app/       Application wiring + entry point
tests/       GoogleTest suite
```

## Build

Requires Windows, MSVC (Visual Studio 2022), CMake 3.25+, Ninja, and vcpkg.
Set `VCPKG_ROOT` to your vcpkg checkout, then:

```pwsh
cmake --preset windows-msvc
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
```

Dependencies are declared in `vcpkg.json` (manifest mode) and installed
automatically on configure.

## CI

`.github/workflows/ci.yml` builds and tests on `windows-latest` for every push
and pull request.
