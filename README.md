# Graphical Interface Library in C

A lightweight, cross-platform GUI toolkit written in C. It provides a widget system, a scanline polygon renderer, an event loop, and a geometry manager — built on top of SDL2, SDL2_ttf, and FreeImage via a thin hardware-abstraction layer (`eibase`).

---

## Features

- **Widget system** — frame, button, and toplevel (floating window) classes, plus a custom-class API
- **Drawing engine** — scanline polygon fill, Bresenham line, alpha-blended text, rounded rectangles
- **Event handling** — mouse (click, drag, scroll), keyboard, and application events
- **Geometry manager** — placer with absolute/relative position and size, anchor points
- **Widget picking** — off-screen pick surface with unique per-widget RGB colour IDs
- **Cross-platform** — macOS (MacPorts), Linux (X11), Windows

---

## Project Structure

```
.
├── api/            # Public headers (the only includes your app needs)
├── implem/         # Library implementation
│   ├── ei_application.c   # App lifecycle and event loop
│   ├── ei_widget.c        # Widget creation and tree management
│   ├── ei_widget_configure.c  # Frame / button / toplevel classes
│   ├── ei_placer.c        # Geometry manager
│   ├── ei_draw.c          # Fill, polyline, polygon, text, surface copy
│   ├── ei_implementation.c    # Bresenham, scanline, colour mapping
│   └── ei_relief.c        # Rounded-frame polygon generator
├── tests/          # Demo applications (also serve as manual tests)
├── _macos/         # macOS eibase prebuilt library
├── _x11/           # X11/Linux eibase prebuilt library
├── _win/           # Windows eibase prebuilt library
├── docs/           # Doxygen config (docs/doxygen.cfg)
├── CMakeLists.txt
└── README.md
```

---

## Requirements

| Platform | Compiler | Libraries |
|----------|----------|-----------|
| Linux (X11) | GCC / Clang | `libsdl2-dev libsdl2-ttf-dev libfreeimage-dev` |
| macOS | Clang + Xcode CLT | SDL2, SDL2_ttf, FreeImage via MacPorts (`/opt/local`) |
| Windows | MSVC / MinGW | SDL2 + FreeImage under `C:/projetc/SDL2_windows` |

CMake ≥ 3.20 is required.

**Linux quick-install:**
```bash
sudo apt-get install cmake libsdl2-dev libsdl2-ttf-dev libfreeimage-dev
```

---

## Build

```bash
# Configure (run once, or after CMakeLists changes)
cmake -B cmake .

# Build the default set of demos (minimal, frame, button, hello_world, puzzle, two048, minesweeper)
cmake --build cmake

# Build a specific target
cmake --build cmake --target hello_world

# Build everything including demos excluded from ig_all
cmake --build cmake --target lines dessin_relief la_souris_verte test_d_sor3a ext_testclass

# Generate Doxygen documentation
cmake --build cmake --target doc
```

> **Stale cache warning:** if you previously built on a different machine (e.g. macOS → Linux), delete the `cmake/` directory and re-run `cmake -B cmake .` before building.

---

## Running the Demos

After building, executables land in `cmake/`:

```bash
./cmake/minimal          # Blank window — smoke test
./cmake/hello_world      # Toplevel window with a button
./cmake/button           # Button styles and callbacks
./cmake/frame            # Frame widget showcase
./cmake/lines            # Line-drawing primitives
./cmake/dessin_relief    # Polygon relief / rounded buttons
./cmake/la_souris_verte  # Mouse-tracking demo
./cmake/puzzle           # Sliding-tile puzzle game
./cmake/two048           # 2048 game
./cmake/minesweeper      # Minesweeper (left-click reveal, right-click flag)
./cmake/test_d_sor3a     # Performance / stress test
./cmake/ext_testclass    # External custom widget class demo
```

Press **Escape** or close the window to quit any demo.

---

## API Quick Reference

All public headers live in `api/`. Your application only needs to include from there.

| Header | Purpose |
|--------|---------|
| `ei_application.h` | `ei_app_create`, `ei_app_run`, `ei_app_free`, `ei_app_invalidate_rect` |
| `ei_widget.h` | `ei_widget_create`, `ei_widget_destroy`, `ei_widget_pick` |
| `ei_widget_configure.h` | `ei_frame_configure`, `ei_button_configure`, `ei_toplevel_configure` |
| `ei_placer.h` | `ei_place`, `ei_place_xy`, `ei_placer_forget` |
| `ei_event.h` | `ei_event_set_default_handle_func`, active-widget management |
| `ei_draw.h` | `ei_fill`, `ei_draw_polyline`, `ei_draw_polygon`, `ei_draw_text`, `ei_copy_surface` |
| `ei_types.h` | `ei_color_t`, `ei_rect_t`, `ei_point_t`, `ei_size_t`, … |
| `hw_interface.h` | Hardware abstraction (surfaces, fonts, events) — rarely called directly |

### Minimal application

```c
#include "ei_application.h"
#include "ei_widget_configure.h"
#include "ei_placer.h"
#include "ei_event.h"

void quit_on_escape(ei_event_t* event) {
    if (event->type == ei_ev_keydown && event->param.key_code == SDLK_ESCAPE)
        ei_app_quit_request();
}

int main(void) {
    ei_app_create((ei_size_t){800, 600}, false);

    ei_widget_t win = ei_widget_create("toplevel", ei_app_root_widget(), NULL, NULL);
    ei_toplevel_configure(win, &(ei_size_t){400, 300},
                          &(ei_color_t){0xA0, 0xA0, 0xA0, 0xFF},
                          &(int){4},
                          &(ei_string_t){"My Window"}, NULL, NULL, NULL);
    ei_place_xy(win, 100, 80);

    ei_event_set_default_handle_func(quit_on_escape);
    ei_app_run();
    ei_app_free();
    return 0;
}
```

### Implementing a custom widget class

Implement the six function pointers in `ei_widgetclass_t` (allocfunc, releasefunc, drawfunc, setdefaultsfunc, geomnotifyfunc, handlefunc) and call `ei_widgetclass_register`. See `tests/testclass.c` for a complete example.

---

## Documentation

```bash
cmake --build cmake --target doc
# Open docs/html/index.html in a browser
```

---

## Bug Tracking

Known bugs, root-cause analyses, and fixes are logged in [`BUGBUSTER.md`](BUGBUSTER.md).
