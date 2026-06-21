# KeyState V1.1

> Lightweight Windows real-time keyboard state overlay monitor

---

## Introduction

**KeyState** is a Windows desktop overlay tool that creates a **translucent floating window** on your screen, displaying monitored keys' **press/release status**, **total press count (Total)**, **keys per second (KPS)**, and **BPM rhythm analysis**.

Suitable for game streaming, key teaching, input method testing, keyboard showcases, rhythm game analysis, and more.

---

## Interface Preview

### State Floating Window

- **Transparent background**, floats anywhere without blocking desktop content
- Key mapping boxes arranged in **horizontal rows**
- **Smoothstep color easing + multi-layer glow animation** on press
- Real-time Total count and KPS rate display
- **Mouse drag** to reposition, auto-saves on release

```
+-------+ +-----+ +-----+ +-----+ +------+ +------+
| Space | |  A  | |  S  | |  D  | |  S   | |  B   |
| 5|2s  | |3|1s | |8|4s | |1|0s | |17|7/s| |90|16 |
+-------+ +-----+ +-----+ +-----+ +------+ +------+
                                   S=Total  B=BPM
```

### Settings Panel

| Section | Features |
|---------|----------|
| **Language** | Chinese / English / Japanese |
| **Basic Settings** | Show Total / Show KPS / Show Summary / Show History / Show BPM / Click Through |
| **Layout** | Key Size (30-200px) + Key Spacing (0-40px) |
| **Track** | Track Height (20-600px) + Grow Speed + Float Speed + Block Max% |
| **Visual** | Theme presets (Custom/Neon/Minimal/Forest) + Border Width + Opacity |
| **Key Mapping** | Press any key to capture and add / Delete (at least 1 required); 5-layer key name fallback |
| **Appearance** | Adjust font color / normal background / press background (system color picker) |
| **BPM** | Select note division basis (8th/16th/32nd/64th) |

---

## Controls

| Action | Description |
|--------|-------------|
| **Drag floating window** | Hold and drag the State window freely, auto-saves on release |
| **Right-click window / tray** | Popup menu: Settings / Exit |
| **Ctrl+Shift+K** | Global hotkey to open settings panel |
| **Settings panel X** | Hide settings panel |
| **Settings panel scroll** | Fixed 700px height window, use mouse wheel or drag scrollbar |

---

## Advanced Features

### History Track

Each key has a vertical transparent track above it. When pressed, a **block grows from the bottom in real time**; when released, it is cut off and floats upward at a constant speed before fading out. Block width matches the key box, height = press duration x grow speed.

### BPM Rhythm Analysis

An independent BPM box displayed next to the summary box:
- Total KPS x 240 / note division = equivalent BPM
- Selectable note division: 8th / 16th / 32nd / 64th

### Theme Presets

| Name | Style |
|------|-------|
| Custom | Manual color picker |
| Neon | Pink-purple text + dark purple background + bright pink press |
| Minimal | Dark gray text + light gray background + medium gray press |
| Forest | Light green text + dark green background + bright green press |

---

## Configuration File

- File name: `KeyStateSetting.json`
- Location: Same directory as `KeyState.exe`
- Automatically created with default config on first run
- Any setting change is **auto-saved instantly** and persists across restarts
- Contains: key list, colors, position, size, display options, language, FPS, etc.

---

## Tech Stack

| Component | Details |
|-----------|---------|
| Language | C++17 |
| Rendering | GDI+ (anti-aliased vector graphics + text + premultiplied alpha) |
| Window | Win32 API (WS_EX_LAYERED per-pixel transparency + WS_VSCROLL) |
| Keyboard | WH_KEYBOARD_LL global hook (with IME virtual key filtering) |
| Config | Custom JSON parser |
| i18n | Chinese / English / Japanese string tables |

## Source Files

```
src/
+-- main.cpp          # Entry point, message pump, tray, hotkeys
+-- config.h/cpp      # Config struct + JSON read/write
+-- keyboard.h/cpp    # Keyboard hook, state management, KPS, duration tracking
+-- renderer.h/cpp    # GDI+ rendering, easing animation, track, summary, BPM
+-- display_ui.h/cpp  # State transparent overlay, drag
+-- settings_ui.h/cpp # Settings panel, color picker, Theme, scroll
+-- lang.h/cpp        # Multi-language string table
```

---

## Version History

### V1.1 (2026-06)

- Fixed: Chart settings resetting on every startup
- Fixed: Chart window not closing when resetting to defaults
- Fixed: Key mappings shifting when track is enabled
- Added: Chart snap feature -- snap below key display with adjustable X/Y offset
- Improved: Basic settings layout for clearer grouping

### V1.0 (2026-06)

- Translucent floating State window + full Settings panel (with scrollbar)
- Real-time Total count + KPS sliding window calculation
- Smoothstep color easing animation (80ms press / 100ms release)
- Multi-layer glow press feedback
- 5-layer key name resolution fallback + IME virtual key filtering
- Key history track (real-time growth + constant speed float)
- BPM / note division rhythm calculation
- Sigma summary box + Beta BPM box
- 4 Theme presets + border width + opacity
- Chinese / English / Japanese multi-language support
- Adjustable render FPS (25/45/60/90/120)
- JSON config persistence in exe directory
- Drag-to-position memory
- System tray + global hotkeys
- At least 1 key mapping boundary protection
- Long-press repeat filtering

---

## Open Source License

This project is licensed under the **MIT License**.

```
MIT License

Copyright (c) 2026 KeyState

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

> Commercial use is permitted. You are free to use, modify, and distribute this software, including for commercial projects, without any fee. Simply retain the original copyright notice.
