# CLAUDE.md

Design context for OnTaskManager. This is the running record of decisions
we've made so a Claude session walking in cold can implement against the
intended shape of the app — not just the existing scaffold.

If a decision here contradicts the code, the code is wrong (or the decision
is stale and should be updated here first). If something below isn't
decided, don't invent an answer; ask.

## What this app is

A local-first activity/productivity tracker for Windows. It samples the
focused window on an interval, derives a hierarchical "activity path" from
each sample, lets the user organize those activities into a category tree,
and shows where the day's time went.

## Stack

- Qt 6 (Widgets), via vcpkg manifest mode
- C++20, MSVC, CMake + Ninja, CI on `windows-latest`
- SQLite for persistence (single user, single machine, no sync — yet)

## Core model: two trees

There are **two separate trees**, and the relationship between them is
where most of the product lives.

### 1. Activity tree (observed, provider-emitted)

Each sample produces a **path of arbitrary depth**, not a flat
`(app, title)` pair. The depth depends on which provider parsed the
sample. Examples:

```
Chrome / Jira / PHX-123
Chrome / Reddit / r/subreddit1
Visual Studio / Gameplay / GameplayFile1.cpp
Visual Studio / Engine   / EngineFile1.h
```

The activity tree is **built bottom-up from samples** — the user does
not curate it. New leaves appear as new windows are seen.

#### Providers

The piece that turns a focused-window observation into an activity path is
a **provider**. v1 ships with a single title-parsing provider per known
app (Chrome, Visual Studio, etc.) plus a generic fallback that emits just
`<AppName>`. The provider interface is the seam where, later, we can plug
in richer sources — browser extension, IDE plugin, app APIs — without
changing storage or UI.

Don't hardcode `(app, title)` anywhere downstream of the provider. The
storage layer and UI should treat activity as an opaque ordered path.

### 2. Category tree (curated, user-defined)

A hierarchy of user-defined labels:

```
Work
  Project Phoenix
    Coding
    Jira
Non-Work
  Social
```

Top-level buckets are freeform (not a fixed Work/Non-Work pair). Depth
is arbitrary. The user creates and renames nodes.

### Mapping: activity-subtree → category leaf

Each node in the activity tree can be **assigned to exactly one category
leaf**. Assignment is **inherited down the subtree** and **overridable on
descendants**:

- Assigning `Chrome` → `Non-Work/Social` makes all Chrome activity
  Non-Work…
- …unless `Chrome / Jira` is explicitly assigned to `Work/Project Phoenix/Jira`,
  in which case that subtree overrides.
- `Chrome / Reddit / r/subreddit1` can override yet again.

The same app legitimately spans multiple top-level categories because
categorization attaches to **activity-tree subtrees**, not to apps.

Assignment is **retroactive**: changing a mapping reclassifies all
historical samples under that subtree. The UI surfaces this — the user
expects today's pie/timeline to update when they categorize something.

### "Uncategorized"

An implicit bucket, not a user-created category. Any activity-tree leaf
whose nearest assigned ancestor is unset falls here. Uncategorized time
**counts toward total tracked time** and is shown in the UI so the user
can sweep through it.

### Why categories sum to total time

Because every sample maps to exactly one category leaf (or
Uncategorized), per-category totals add up cleanly to the day's tracked
total. This is a core invariant — don't break it by introducing
multi-assignment.

### Tags (deferred)

Tags would be a second, orthogonal axis (many-to-many) — useful for
cross-cutting concerns like "billable" or "deep focus" that don't fit
the hierarchy. **Not in v1.** Leave the schema open to a future tag
table keyed on activity-path or time-range. Don't build UI for it
until there's a concrete many-to-many use case.

## Tracking behavior

- **Sample cadence**: tunable, **default 15s**. Setting lives in
  preferences.
- **Active window definition**: whatever window is currently focused.
- **Idle tracking**: opt-in. When enabled, an **activity-timeout**
  setting (also tunable) defines how long the user must be idle before
  the focused app stops being counted as active.
- **System sleep**: forced idle, regardless of the timeout.
- **Idle timeline**: rendered on its **own lane**, separate from
  activity. It is not a category and does not merge into the activity
  totals.

## UI / view model

### Default view

- Opens to **today's timeline**, with a date picker to navigate other
  days.
- The timeline is the primary surface. Aggregate views (per-category
  totals, etc.) layer on top of it.
- Idle has its own lane on the timeline.

### Tray-first lifecycle

The app lives in the system tray. Closing the main window hides it
rather than quits. Quit is an explicit tray menu action. (Already
reflected in the scaffold — confirm before changing.)

### What's still open (ask before deciding)

These weren't settled in the design conversation. Don't guess:

- **Timeline visual style**: lanes per category? Gantt-style blocks?
  Heatmap? Vertical day strip vs horizontal?
- **Aggregate surface**: is there a per-category totals panel alongside
  the timeline, a separate "reports" tab, or both?
- **Categorization UX**: how does the user assign an activity subtree
  to a category? Drag from an "Uncategorized" list onto the category
  tree? Right-click on a timeline block? Rules engine? All of the above?
- **Uncategorized sweep flow**: dedicated triage view, or inline in
  the main view?
- **Tray menu**: which actions live there beyond Open / Quit?
- **Settings surface**: modal preferences dialog, separate window,
  inline?

## Repository layout

```
src/
  tracker/   foreground window + idle detection (Win32)
  storage/   SQLite persistence
  ui/        tray icon + main window
  app/       Application wiring + entry point
tests/       GoogleTest suite
```

A few things to watch for as the activity-path model lands:

- `storage/` currently looks flat. The schema needs to represent
  variable-depth activity paths (likely a normalized `activity_node`
  table with `parent_id`, plus samples referencing a leaf node id).
- `tracker/ForegroundTracker` is where the provider seam belongs.
  Don't bake title-parsing into the tracker itself — providers should
  be pluggable.
- `ui/MainWindow` is currently a raw table of recorded data. That's a
  debugging view; the real main window is the timeline described above.

## Working notes for future Claude sessions

- This file is the source of truth for **intent**. Update it when a
  design decision changes — before the code change, ideally.
- When the user asks for something that conflicts with what's here,
  flag the conflict rather than silently picking a side.
- Sections marked "still open" are genuinely open. Asking is correct;
  inventing an answer is not.
