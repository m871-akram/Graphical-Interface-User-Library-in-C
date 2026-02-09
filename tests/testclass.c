/*
 *  testclass.c
 *
 *  Implementation of the "testclass" custom widget class.
 *  A simple container widget with a configurable margin around its content area.
 */

#include <stdlib.h>
#include <string.h>

#include "ei_utils.h"
#include "ei_types.h"
#include "ei_widget.h"
#include "ei_widgetclass.h"
#include "ei_draw.h"
#include "ei_placer.h"
#include "ei_application.h"
#include "ei_implementation.h"


/* --- testclass widget structure --- */

typedef struct ei_impl_testclass_t {
    ei_impl_widget_t widget;
    int              margin;
    ei_rect_t        content_rect_storage;
} ei_impl_testclass_t;


/* --- Widget class functions --- */

static ei_widget_t testclass_allocfunc(void)
{
    ei_impl_testclass_t* tc = calloc(1, sizeof(ei_impl_testclass_t));
    if (!tc) return NULL;
    tc->widget.content_rect = &tc->content_rect_storage;
    return (ei_widget_t)tc;
}

static void testclass_releasefunc(ei_widget_t widget)
{
    /* Nothing to free beyond the struct itself (handled by ei_widget_destroy). */
}

static void testclass_setdefaultsfunc(ei_widget_t widget)
{
    ei_impl_testclass_t* tc = (ei_impl_testclass_t*)widget;
    tc->widget.requested_size = ei_size(0, 0);
    tc->margin = 4;
    tc->content_rect_storage = ei_rect_zero();
}

static void testclass_geomnotifyfunc(ei_widget_t widget)
{
    ei_impl_testclass_t* tc = (ei_impl_testclass_t*)widget;
    int m = tc->margin;

    /* content_rect is the screen_location inset by the margin on all sides. */
    tc->content_rect_storage.top_left.x = tc->widget.screen_location.top_left.x + m;
    tc->content_rect_storage.top_left.y = tc->widget.screen_location.top_left.y + m;
    tc->content_rect_storage.size.width  = tc->widget.screen_location.size.width  - 2 * m;
    tc->content_rect_storage.size.height = tc->widget.screen_location.size.height - 2 * m;

    if (tc->content_rect_storage.size.width  < 0) tc->content_rect_storage.size.width  = 0;
    if (tc->content_rect_storage.size.height < 0) tc->content_rect_storage.size.height = 0;

    /* Re-run the placer for all managed children so they adapt to the new content_rect. */
    ei_widget_t child = tc->widget.children_head;
    while (child != NULL) {
        ei_impl_widget_t* child_impl = (ei_impl_widget_t*)child;
        if (child_impl->placer_params != NULL)
            ei_impl_placer_run(child);
        child = child_impl->next_sibling;
    }
}

static void testclass_drawfunc(ei_widget_t widget,
                                ei_surface_t surface,
                                ei_surface_t pick_surface,
                                ei_rect_t* clipper)
{
    ei_impl_testclass_t* tc = (ei_impl_testclass_t*)widget;
    ei_rect_t draw_rect = tc->widget.screen_location;

    if (clipper && !intersection_rect(&draw_rect, &tc->widget.screen_location, clipper))
        return;

    /* Fill the pick surface for widget identification. */
    ei_fill(pick_surface, &tc->widget.pick_color, &draw_rect);

    /* Draw the margin area (outer border) in a distinct color. */
    ei_color_t margin_color = {0x88, 0x44, 0x44, 0xff};
    ei_fill(surface, &margin_color, &draw_rect);

    /* Draw the content area (inner rectangle) in the default background color. */
    ei_rect_t visible_content;
    if (intersection_rect(&visible_content, &tc->content_rect_storage, &draw_rect)) {
        ei_color_t content_color = ei_default_background_color;
        ei_fill(surface, &content_color, &visible_content);
    }

    /* Draw children clipped to the content area. */
    ei_rect_t children_clipper;
    if (intersection_rect(&children_clipper, &tc->content_rect_storage, &draw_rect)) {
        ei_impl_widget_draw_children(widget, surface, pick_surface, &children_clipper);
    }
}


/* --- Static class descriptor --- */

static ei_widgetclass_t g_testclass;


/* --- Public API --- */

void testclass_register(void)
{
    strncpy(g_testclass.name, "testclass", sizeof(g_testclass.name) - 1);
    g_testclass.name[sizeof(g_testclass.name) - 1] = '\0';
    g_testclass.allocfunc       = testclass_allocfunc;
    g_testclass.releasefunc     = testclass_releasefunc;
    g_testclass.drawfunc        = testclass_drawfunc;
    g_testclass.setdefaultsfunc = testclass_setdefaultsfunc;
    g_testclass.geomnotifyfunc  = testclass_geomnotifyfunc;
    g_testclass.handlefunc      = NULL;
    g_testclass.next            = NULL;
    ei_widgetclass_register(&g_testclass);
}

void testclass_configure(ei_widget_t widget, int margin)
{
    ei_impl_testclass_t* tc = (ei_impl_testclass_t*)widget;
    tc->margin = margin;

    /* If the widget is managed by the placer, recompute geometry. */
    if (tc->widget.placer_params != NULL)
        ei_impl_placer_run(widget);

    ei_app_invalidate_rect(&tc->widget.screen_location);
}

int testclass_get_margin(ei_widget_t widget)
{
    ei_impl_testclass_t* tc = (ei_impl_testclass_t*)widget;
    return tc->margin;
}
