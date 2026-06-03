# Anti-Aliasing Smooth Font Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Enable anti-aliased smooth font rasterization for all fonts in the HomeDeck project to eliminate jagged text edges.

**Architecture:** Modify the FreeType font-to-VLW tool to remove the target-mono flag, generating 8bpp alpha-channel glyph data. Regenerate and compile the C++ font arrays, and run existing unit tests to verify the compatibility.

**Tech Stack:** C++, FreeType, Python, PlatformIO (Unity test framework).

---

### Task 1: Modify VLW Font Encoder

**Files:**
- Modify: [font_to_vlw.cpp](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/tools/font_to_vlw.cpp)

- [ ] **Step 1: Modify FreeType load flags**

  Change the `renderGlyph` function in `tools/font_to_vlw.cpp` to remove `FT_LOAD_TARGET_MONO`.

  In [tools/font_to_vlw.cpp](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/tools/font_to_vlw.cpp#L151-L158), locate `renderGlyph` and change:
  ```cpp
  Glyph renderGlyph(FT_Face face, std::uint32_t code_point) {
    if (FT_Load_Char(face, code_point, FT_LOAD_RENDER | FT_LOAD_TARGET_MONO) !=
        0) {
  ```
  to:
  ```cpp
  Glyph renderGlyph(FT_Face face, std::uint32_t code_point) {
    if (FT_Load_Char(face, code_point, FT_LOAD_RENDER) != 0) {
  ```

- [ ] **Step 2: Commit encoder changes**

  Run:
  ```bash
  git add tools/font_to_vlw.cpp
  git commit -m "refactor: remove FT_LOAD_TARGET_MONO from font_to_vlw encoder"
  ```

---

### Task 2: Regenerate Device Fonts

**Files:**
- Modify: [device_font_vlw.cpp](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/src/generated/device_font_vlw.cpp)
- Modify: [device_font_vlw.h](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/src/generated/device_font_vlw.h)

- [ ] **Step 1: Run generator script**

  Run:
  ```bash
  python tools/generate_device_font.py
  ```
  Expected:
  The script automatically compiles the updated `font_to_vlw` binary and regenerates all 5 VLW font header and source files in `src/generated/`. It should print output showing `device_font`, `device_large_date_font`, etc.

- [ ] **Step 2: Check generated files status**

  Run:
  ```bash
  git status
  ```
  Expected: Shows modifications in `src/generated/device_font_vlw.cpp` and `src/generated/device_font_vlw.h`.

- [ ] **Step 3: Commit regenerated fonts**

  Run:
  ```bash
  git add src/generated/device_font_vlw.cpp src/generated/device_font_vlw.h
  git commit -m "feat: regenerate all fonts with 8bpp anti-aliased glyphs"
  ```

---

### Task 3: Verify with Compilation and Tests

**Files:**
- Test: [test_device_font_resource](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/test/native/test_device_font_resource/test_main.cpp)
- Test: [test_home_renderer](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/test/native/test_home_renderer/test_main.cpp)

- [ ] **Step 1: Run native unit tests**

  Run:
  ```bash
  pio test -e native
  ```
  Expected: Tests pass successfully (0 failures).

- [ ] **Step 2: Verify ESP32 target compilation**

  Run:
  ```bash
  pio run -e m5stack-papercolor
  ```
  Expected: Successful compilation (SUCCESS).

- [ ] **Step 3: Commit verification report**

  We do not need additional code commits, but we can verify our workspace is clean.
  Run:
  ```bash
  git status
  ```
  Expected: "nothing to commit, working tree clean"
