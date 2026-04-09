# BUGBUSTER - Bug Tracking & Resolution Log

## Bug #1: Toplevel freezes when dragged outside the window

**Test:** `hello_world`

**Symptom:** Moving the toplevel window to the left and outside the application window causes the program to freeze and become unresponsive.

**Root Cause:** In `toplevel_drawfunc` (`implem/ei_widget_configure.c`), four calls to `intersection_rect` were using the toplevel's stored rectangles as both input and output:

```c
intersection_rect(&toplevel->title_bar_rect, &toplevel->title_bar_rect, &draw_rect);
intersection_rect(&toplevel->close_button_rect, &toplevel->close_button_rect, &draw_rect);
intersection_rect(toplevel->widget.content_rect, toplevel->widget.content_rect, &draw_rect);
intersection_rect(&toplevel->resize_handle_rect, &toplevel->resize_handle_rect, &draw_rect);
```

Each redraw permanently shrank these rects to whatever portion was visible on-screen. When the toplevel moved off-screen, these rects collapsed to `{0,0,0,0}`. After releasing the mouse, `geomnotifyfunc` was no longer called, so the corrupted rects persisted — the title bar became un-clickable and the window appeared frozen.

**Fix:** Replaced all four destructive `intersection_rect` calls with local temporary variables (`visible_title_bar`, `visible_close_btn`, `visible_content`, `visible_resize`). The drawing code now uses the temporaries for clipping/filling while the authoritative rects remain untouched.

---

## Bug #2: Toplevel resize handle not working

**Test:** `ext_testclass`, also affects any toplevel with children filling the content area

**Symptom:** Clicking and dragging the blue resize handle at the bottom-right corner of a toplevel does nothing.

**Root Cause:** The resize handle is positioned inside the toplevel's content area. Child widgets drawn within the content area paint their own `pick_color` on the pick surface, overwriting the toplevel's `pick_color` at the resize handle location. When the user clicks on the resize handle, `ei_widget_pick()` returns the child widget instead of the toplevel. Since the child has no `handlefunc`, the click goes unhandled and the resize never starts.

**Fix:** In `toplevel_drawfunc`, the resize handle section now repaints the toplevel's `pick_color` on the pick surface **after** drawing children:

```c
ei_fill(pick_surface, &toplevel->widget.pick_color, &visible_resize);
```

This ensures the resize handle area always maps back to the toplevel widget for picking, regardless of child overlap.

---

## Bug #4: Puzzle tiles do not move when clicked

**Test:** `puzzle`

**Symptom:** Clicking a puzzle tile does nothing — tiles never swap with the empty slot.

**Root Cause:** In `create_puzzle_window` (`tests/puzzle.c`), the button callback's `user_param` was set to `(void*)&tile` instead of `(void*)tile`. Since `tile` is a local `tile_t*` variable, `&tile` is a `tile_t**` pointing to a stack location that no longer exists after `create_puzzle_window` returns. Every subsequent call to `handle_tile_press` read from that dangling stack address, getting garbage instead of the real `tile_t*`, so the move logic never executed correctly.

**Fix:** Changed the argument from `(void*)&tile` to `(void*)tile` in `tests/puzzle.c:188`:

```c
// Before (bug):
ei_button_configure(..., &callback, (void*)&tile);

// After (fix):
ei_button_configure(..., &callback, (void*)tile);
```

---

## Bug #3: Missing `testclass` widget class for `ext_testclass` test

**Test:** `ext_testclass`

**Symptom:** The `ext_testclass` test fails to build — the `testclass` library it links against does not exist.

**Root Cause:** The test file `ext_testclass.c` references three external functions (`testclass_register`, `testclass_configure`, `testclass_get_margin`) and a widget class named `"testclass"`, but no implementation was provided.

**Fix:** Created `tests/testclass.c` implementing the `"testclass"` widget class — a container widget with a configurable margin property:

- `testclass_register()` — registers the class with the widget system
- `testclass_configure(widget, margin)` — sets the margin and triggers geometry recalculation
- `testclass_get_margin(widget)` — returns the current margin

The widget draws a colored border (the margin area) around its content, and its `content_rect` is the `screen_location` inset by the margin on all sides. Updated `CMakeLists.txt` to build the `testclass` static library.
