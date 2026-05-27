# BUGBUSTER — Bug Tracking 

## #1 — Toplevel freezes when dragged off-screen

**Test:** `hello_world`

`toplevel_drawfunc` passed the toplevel's own stored rects as both input **and** output to `intersection_rect`, so every redraw permanently shrank them to the visible portion. When the window moved fully off-screen the rects collapsed to `{0,0,0,0}`, making the title bar un-clickable on return.

**Fix:** Replaced the four aliased `intersection_rect` calls with local temporaries (`visible_title_bar`, `visible_close_btn`, `visible_content`, `visible_resize`). Authoritative rects are now read-only during drawing.

---

## #2 — Resize handle unclickable when children overlap it

**Test:** `ext_testclass`

Child widgets paint their `pick_color` over the resize handle area, so `ei_widget_pick` returns the child instead of the toplevel. The child has no drag handler, so the resize never starts.

**Fix:** After drawing children, `toplevel_drawfunc` repaints the toplevel's `pick_color` over the resize handle rect on the pick surface:
```c
ei_fill(pick_surface, &toplevel->widget.pick_color, &visible_resize);
```

---

## #3 — Missing `testclass` widget class

**Test:** `ext_testclass`

`ext_testclass.c` referenced `testclass_register`, `testclass_configure`, and `testclass_get_margin` but no implementation existed, causing a link failure.

**Fix:** Created `tests/testclass.c` — a container widget with a configurable margin. Its `content_rect` is `screen_location` inset by the margin on all sides. Updated `CMakeLists.txt` to build it as a static library.

---

## #4 — Puzzle tiles do not move

**Test:** `puzzle`

`create_puzzle_window` passed `(void*)&tile` (address of a local `tile_t*`) as `user_param`. After the function returned, the pointer was dangling. `handle_tile_press` read garbage instead of the real tile pointer.

**Fix:** `(void*)&tile` → `(void*)tile` at `tests/puzzle.c:188`.

---

## #5 — Memory leaks on frame/toplevel destruction

**Test:** `hello_world`, `button`

Three incomplete cleanup paths:
- `frame_releasefunc` freed `img_rect` but not `frame->text` (`strdup`'d by `ei_frame_configure`).
- `toplevel_releasefunc` was empty — `toplevel->title` and `content_rect` were never freed.
- `ei_frame_configure`: assigning `NULL` to `frame->text` when setting an image skipped the required `free`.

**Fix:** Added `free(frame->text)` in `frame_releasefunc` and before the `frame->text = NULL` assignment. Implemented `toplevel_releasefunc` to free `title` and (conditionally) `content_rect`.

---

## #6 — Widget picking collides with >~19 widgets

**Test:** `minesweeper`, `two048`, `puzzle`

`ei_widget_create` derived `pick_id` by truncating the heap pointer to `uint32_t`. With 16-byte allocator alignment the bottom 4 bits are always 0, leaving only ~19 effective bits of entropy. Collisions occur with a few hundred widgets.

**Fix:** Replaced pointer-based IDs with a `static uint32_t g_pick_id_counter` starting at 1. All 24 RGB bits are now used uniformly, supporting up to 16 M unique widgets.

---

## #7 — `ei_fill` crashes with `NULL` colour

**Test:** any

`ei_fill` dereferenced `couleur` immediately despite `ei_draw.h` documenting that `NULL` means "paint opaque black".

**Fix:** Added a NULL guard before `ei_impl_map_rgba`:
```c
ei_color_t black = {0, 0, 0, 0xff};
uint32_t val = ei_impl_map_rgba(surface, couleur ? *couleur : black);
```

---

## #8 — Wrong colours on macOS

**Test:** any on macOS

`draw_line` and `draw_horizontal_line` in `ei_implementation.c` guarded the correct colour-mapping path with `#if defined(_APPLE_)`. The valid macro is `__APPLE__` (double underscores), so the guard was never true on macOS and colours were read from the wrong byte order.

**Fix:** `_APPLE_` → `__APPLE__` in both functions (`replace_all`).

---

## #9 — Frame children not drawn on full redraw

**Test:** `hello_world`, `button`

`frame_drawfunc` called `intersection_rect(&children_clipper, &content_area, clipper)` without a NULL guard. `intersection_rect` asserts all arguments non-NULL; passing `clipper = NULL` (full-surface draw) caused an assert or UB.

**Fix:** When `clipper == NULL`, call `ei_impl_widget_draw_children` directly with `&content_area`.

---

## #10 — Clicking toplevel body captures all future events

**Test:** `hello_world`, `button`

A plain body click called `ei_event_set_active_widget(widget)` but the mouse-up handler only cleared the active widget if `was_dragging` was true. A body click set no drag flag, so the active widget was never cleared.

**Fix:** Removed the `was_dragging` condition — `ei_event_set_active_widget(NULL)` is now unconditional on left mouse-up while the toplevel is active.

---

## #11 — Polygon scanline skips last row

**Test:** `dessin_relief`, `button`

`creer_table_tc` used `index < (y_max - y_min)` to guard edge-table insertion. The table has `y_max - y_min + 1` slots (indices 0 … `y_max - y_min`), so the strict `<` silently dropped all edges whose lower endpoint sits exactly at `y_max`.

**Fix:** `index < (y_max - y_min)` → `index <= (y_max - y_min)`.

---

## #12 — `toplevel_allocfunc` crashes on alloc failure

**Test:** any under memory pressure

`calloc` and the subsequent `malloc` for `content_rect` were both unchecked. A NULL return from either caused an immediate NULL dereference on the next line.

**Fix:** Added NULL checks with proper cleanup (free the toplevel if `content_rect` alloc fails) and early return of NULL.

---

## #13 — Root widget creation violates API contract *(design note)*

`ei_application.c` creates the root widget with `parent = NULL`, which `ei_widget.h` documents as forbidden. No crash occurs because `ei_widget_create` silently skips parent attachment for NULL. The API doc comment should be updated to note that NULL is accepted only for the internal root widget.

---

## #14 — Dangling pointer after toplevel close *(design note)*

When the close button is clicked, `toplevel_handlefunc` calls `ei_widget_destroy(widget)` and returns `true`. The event loop in `ei_application.c` holds a now-dangling `target_widget` pointer. No use-after-free occurs today because the loop does not touch `target_widget` after `handlefunc` returns — but any future change to the event loop must account for this.

---

## #15 — Right-click flag never appears in minesweeper

**Test:** `minesweeper`

`button_handlefunc` only processed `ei_mouse_button_left` in `ei_ev_mouse_buttondown`. Right-clicks within bounds set `event_handled = true` but never called the callback. `cell_button_handler` (which calls `switch_flag()` on right-click) was therefore never reached.

**Fix:** Added an `else if` for non-left clicks inside the bounds check:
```c
else if (button->callback != NULL)
    button->callback(widget, event, button->user_param);
```

---

## #16 — Toplevel resists leftward dragging *(design note)*

Not a code bug. The drag formula `new_x = start_x + (mouse.x - drag_start.x)` is unconstrained. The perceived limit comes from the physical screen: the SDL window typically opens at the screen's top-left corner (x = 0), so the cursor cannot report values below 0 regardless of how far the user moves left. Rightward/downward movement has more room because the physical screen extends beyond the 800 × 600 window in those directions. A fix would require `SDL_SetRelativeMouseMode` during drags, which `hw_interface` does not expose.
