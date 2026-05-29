# OnTaskManager

Activity/productivity-tracking app for Windows. This is a testbed for using Claude Code (specifically when I'm not at my computer.) Initial scaffolding set up purely through Claude in the browser. Will eventually get this up and running in VSCode for manual/proper dev and fine-tuning.

## Layout

```
src/
  tracker/   foreground window + idle detection (Win32),
             pluggable ActivityProviders (Generic / Chrome / Visual Studio)
  storage/   SQLite persistence — activity tree, category tree,
             activity→category mapping, samples
  ui/        Qt 6 main window with the combined category/activity timeline,
             system tray icon
  app/       QApplication wiring + entry point
tests/       GoogleTest suite (storage, providers, tracker)
```

See `CLAUDE.md` for the design intent (two-tree model, providers, tray-first
lifecycle, what's still open).

## Build

Requires Windows, MSVC (Visual Studio 2022), CMake 3.25+, Ninja, and vcpkg.
Qt 6 (qtbase) is pulled in via vcpkg manifest mode; the first configure
builds Qt from source and may take 30–60 minutes.
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
