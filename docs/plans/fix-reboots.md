# Fix Reboot Loop After Upgrade (Issue #793)

## Overview

When upgrading from Papyrix or CrossPoint v0.16 to CrossPoint v1.0.0, devices get stuck in a reboot loop. The root cause is that `serialization::readString()` in `Serialization.h` performs unbounded heap allocation based on untrusted file data. When reading corrupted or incompatible cache files (old epub cache from a different firmware or version), garbage bytes are interpreted as a string length, requesting gigabytes of memory on an ESP32 with ~380KB heap. This causes heap corruption or std::bad_alloc, triggering the watchdog timer and rebooting the device into an infinite loop.

Two crash entry points exist during boot:
1. `RecentBooksStore::loadFromFile()` migrating old v1/v2 recent books calls `getDataFromBook()` which loads EPUBs and reads old cache
2. The reader activity trying to re-open the last book with an incompatible cache

## Context

- Platform: ESP32-C3 (PlatformIO, ~380KB heap)
- Key files:
  - `lib/Serialization/Serialization.h` - readString() with no bounds check
  - `lib/Epub/Epub/BookMetadataCache.cpp` - book.bin cache loading
  - `src/RecentBooksStore.cpp` - recent books migration loads EPUBs
  - `src/CrossPointState.cpp` - app state with version migration
  - `src/main.cpp` - boot logic

## Validation

Build: `pio run -e default`
Lint: `pio check -e default`

## Tasks

### Task 1: Add bounds checking to serialization readString

- [x] Add a max length parameter (with default) to `readString()` in `lib/Serialization/Serialization.h` for both FsFile and istream overloads. Return false / set string to empty if length exceeds the limit (e.g. 4096 bytes for file paths, 64KB general). Change return type to bool.
- [x] Update all callers of `readString()` to handle the new bool return type where needed (CrossPointState, CrossPointSettings, RecentBooksStore, BookMetadataCache, CssParser)

### Task 2: Make BookMetadataCache::load() resilient to corrupted data

- [x] Add validation of `lutOffset`, `spineCount`, and `tocCount` after reading them in `BookMetadataCache::load()`. Validate that lutOffset is within file bounds and counts are reasonable.
- [x] Add validation in `getSpineEntry()` and `getTocEntry()` to check that the LUT position read is within file bounds before seeking.

### Task 3: Make RecentBooksStore migration crash-safe

- [x] In `RecentBooksStore::loadFromFile()`, wrap the `getDataFromBook()` call in the v1/v2 migration path so that if epub loading fails, we still add the book with just its path (graceful degradation instead of crash).

### Task 4: Clear incompatible caches on version upgrade

- [ ] In `main.cpp` boot sequence, after loading state, detect if this is a fresh boot after upgrade (e.g. state file version was old or missing) and delete epub cache directories to force a clean rebuild.
