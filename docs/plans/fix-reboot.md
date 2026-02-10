# Fix CSS Parser Memory Crash Causing Reboot Loop

## Overview

The device is stuck in a reboot loop. After the previous fix for serialization bounds checking, a new crash path emerged. The crash occurs during CSS parsing when loading a book whose CSS rules cache was deleted (by the cache-clearing upgrade logic). The CSS file (OEBPS/epub.css) decompresses to 51,555 bytes, and the parser's memory allocation pattern causes abort() due to heap exhaustion on the ESP32-C3.

## Root Cause

In `CssParser::loadFromStream()`, the parser reads the entire CSS file into a `std::string content` (~51KB), then creates a second copy via `stripComments()` into `std::string cleaned` (~51KB). Both strings coexist on the heap simultaneously, consuming ~102KB. Combined with parsing temporaries (substr, splitOnChar vectors, normalized copies), peak memory reaches ~150KB+. On the ESP32-C3 with ~380KB total heap and other subsystems already loaded, this triggers malloc failure -> abort().

## Context

- Platform: ESP32-C3 (PlatformIO, ~380KB heap)
- Key files:
  - `lib/Epub/Epub/css/CssParser.cpp` - loadFromStream() memory-heavy parsing
  - `lib/Epub/Epub.cpp` - parseCssFiles() calls loadFromStream()

## Validation

Build: `pio run -e default`

## Tasks

### Task 1: Reduce CSS parser peak memory usage and add crash resilience

- [x] Refactor `CssParser::loadFromStream()` to strip comments in-place rather than creating a second string, eliminating the ~51KB duplicate allocation
- [x] Add a try/catch around the CSS parsing call in `Epub::parseCssFiles()` so that if CSS parsing fails (bad_alloc), the book still loads without CSS styling rather than crashing
- [x] Add a maximum CSS file size check in `readFileContent()` - if the file exceeds a reasonable limit (e.g. 32KB), truncate to avoid excessive memory use on ESP32

### Task 2: Build, flash, and verify the fix on device

- [x] Build the firmware with `pio run -e default`
- [x] Flash the firmware to the connected device
- [x] Monitor device logs to confirm the reboot loop is resolved and the book loads successfully
