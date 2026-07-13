# Graphical Interface Library in C

A GUI toolkit written from scratch in C: widget tree, scanline polygon renderer,
event loop and geometry manager, on top of SDL2 / SDL2_ttf / FreeImage through a thin
hardware-abstraction layer (`eibase`).

- **Widgets** — frame, button, toplevel (draggable, resizable, closable windows), plus an API for custom widget classes
- **Drawing** — scanline polygon fill with active-edge table, Bresenham lines, alpha-blended text, rounded rectangles with 3-D relief
- **Picking** — off-screen surface with per-widget colour IDs
- **Damage tracking** — invalidated-rectangle redraw with rect merging
- **Placer** — absolute/relative positions and sizes, nine anchor points

```
api/       public headers (the only includes an app needs)
implem/    the library: application/widget/placer/draw/relief
tests/     12 demo apps — including puzzle, two048 and minesweeper built on the library
_macos/ _x11/ _win/   prebuilt eibase per platform
```

## Build & run

CMake ≥ 3.20 plus the graphics libraries:

<<<<<<< HEAD
```bash
# macOS
brew install sdl2 sdl2_ttf freeimage
# Linux
sudo apt-get install libsdl2-dev libsdl2-ttf-dev libfreeimage-dev
```
=======
| Platform | Compiler | Libraries |
|----------|----------|-----------|
| Linux (X11) | GCC / Clang | `libsdl2-dev libsdl2-ttf-dev libfreeimage-dev` |
| macOS | Clang + Xcode CLT | SDL2, SDL2_ttf, FreeImage via MacPorts (`/opt/local`) |

---

## Build
>>>>>>> 17712832755394d10cb89f0b1025ca15b215fb9c

```bash
cmake -B cmake .
cmake --build cmake
./cmake/hello_world      # toplevel window with a button
./cmake/two048           # 2048, playable
./cmake/minesweeper      # left-click reveal, right-click flag
```

<<<<<<< HEAD
Other demos: `minimal`, `frame`, `button`, `puzzle`, plus extras built with
`cmake --build cmake --target lines dessin_relief la_souris_verte test_d_sor3a ext_testclass`.
Escape or closing the window quits. If you switch machines, delete `cmake/` and reconfigure.

## Using the library
=======
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

All public headers live in `api/`. The application only needs to include from there.

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
>>>>>>> 17712832755394d10cb89f0b1025ca15b215fb9c

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

<<<<<<< HEAD
Custom widget classes implement the six function pointers of `ei_widgetclass_t`
(alloc/release/draw/setdefaults/geomnotify/handle) and register with
`ei_widgetclass_register` — `tests/testclass.c` is a complete example.

Doxygen: `cmake --build cmake --target doc`, then open `docs/html/index.html`.

## Bug journal

Known bugs, root-cause analyses and their fixes are logged in [`BUGBUSTER.md`](BUGBUSTER.md).
=======
---

## Documentation

```bash
cmake --build cmake --target doc
# Open docs/html/index.html in a browser
```

Known bugs, root-cause analyses, and fixes are logged in [`BUGBUSTER.md`](BUGBUSTER.md).
>>>>>>> 17712832755394d10cb89f0b1025ca15b215fb9c
