#include "ei_widget_attributes.h"
#include "ei_implementation.h"
#include "ei_application.h"
#include <assert.h>
#include <stdlib.h>


ei_widgetclass_t* ei_widget_get_class(ei_widget_t widget)
{
    if (!widget) return NULL;
    return widget->wclass;
}

ei_widget_t ei_widget_get_parent(ei_widget_t widget)
{
    if (widget) return widget->parent;
    return NULL;
}

ei_widget_t ei_widget_get_first_child(ei_widget_t widget)
{
    if (widget) return widget->children_head;
    return NULL;
}

ei_widget_t ei_widget_get_last_child(ei_widget_t widget)
{
    if (widget) return widget->children_tail;
    return NULL;
}


const ei_color_t* ei_widget_get_pick_color(ei_widget_t widget)
{
    assert(widget != NULL && "hmm , un widget NULL !! ");
    return &(widget->pick_color);
}

ei_widget_t ei_widget_get_next_sibling(ei_widget_t widget)
{
    if (widget) return widget->next_sibling;
    return NULL;
}

void* ei_widget_get_user_data(ei_widget_t widget)
{
    assert(widget != NULL && "hmm , un widget NULL !! ");
    return (widget)->user_data;
}

const ei_size_t* ei_widget_get_requested_size(ei_widget_t widget)
{
    assert(widget != NULL && "hmm , un widget NULL !! ");
    return &widget->requested_size;
}


void ei_widget_set_requested_size(ei_widget_t widget, ei_size_t requested_size)
{
    assert(widget != NULL && "hmm , un widget NULL !! ");
    ei_impl_widget_t* impl_widget = widget;
    impl_widget->requested_size = requested_size;
    // Si le widget est géré par le placeur, recalculer sa géométrie
    if (impl_widget->placer_params != NULL)
    {
        ei_impl_placer_run(widget);
    }
    // Invalider la zone pour redessiner
    ei_app_invalidate_rect(&impl_widget->screen_location);
}

const ei_rect_t* ei_widget_get_screen_location(ei_widget_t widget)
{
    assert(widget != NULL && "hmm , un widget NULL !! ");
    return &widget->screen_location;
}

const ei_rect_t* ei_widget_get_content_rect(ei_widget_t widget)
{
    assert(widget != NULL && "hmm , un widget NULL !! ");
    return widget->content_rect ? widget->content_rect : &widget->screen_location;
}

void ei_widget_set_content_rect(ei_widget_t widget, const ei_rect_t* content_rect)
{
    assert(widget != NULL && content_rect != NULL);
    ei_impl_widget_t* impl_widget = widget;
    // Si content_rect pointe déjà vers screen_location, allouer un nouveau rectangle
    if (impl_widget->content_rect == &impl_widget->screen_location || impl_widget->content_rect == NULL)
    {
        impl_widget->content_rect = malloc(sizeof(ei_rect_t));
    }
    // Copier le nouveau rectangle
    *impl_widget->content_rect = *content_rect;

    // Invalider la zone pour redessiner
    ei_app_invalidate_rect(&impl_widget->screen_location);
}