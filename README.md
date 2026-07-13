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

```bash
# macOS
brew install sdl2 sdl2_ttf freeimage
# Linux
sudo apt-get install libsdl2-dev libsdl2-ttf-dev libfreeimage-dev
```

```bash
cmake -B cmake .
cmake --build cmake
./cmake/hello_world      # toplevel window with a button
./cmake/two048           # 2048, playable
./cmake/minesweeper      # left-click reveal, right-click flag
```

Other demos: `minimal`, `frame`, `button`, `puzzle`, plus extras built with
`cmake --build cmake --target lines dessin_relief la_souris_verte test_d_sor3a ext_testclass`.
Escape or closing the window quits. If you switch machines, delete `cmake/` and reconfigure.

## Using the library

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

Custom widget classes implement the six function pointers of `ei_widgetclass_t`
(alloc/release/draw/setdefaults/geomnotify/handle) and register with
`ei_widgetclass_register` — `tests/testclass.c` is a complete example.

Doxygen: `cmake --build cmake --target doc`, then open `docs/html/index.html`.

## Bug journal

Known bugs, root-cause analyses and their fixes are logged in [`BUGBUSTER.md`](BUGBUSTER.md).
