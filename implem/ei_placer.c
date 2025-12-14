#include "ei_placer.h"
#include "ei_implementation.h"
#include "ei_application.h"
#include "hw_interface.h"
#include <assert.h>
#include <stdlib.h>
#include "ei_utils.h"


// Dans ei_placer.c
void ei_impl_placer_run(ei_widget_t widget) {
    ei_impl_widget_t* impl_widget = (ei_impl_widget_t*)widget;
    assert(impl_widget->placer_params != NULL && "hmm , pas de placer_params");


    ei_impl_placer_params_t* params = impl_widget->placer_params;

    // Stocker l'ancienne position pour invalidation
    ei_rect_t old_screen_location = impl_widget->screen_location;

    // Get parent’s content_rect (or root surface size for root widget)
    ei_rect_t parent_rect;
    if (impl_widget->parent != NULL) {
        parent_rect = *(((ei_impl_widget_t*)impl_widget->parent)->content_rect);
    } else {

        ei_size_t surface_size = hw_surface_get_size(ei_app_root_surface());
        parent_rect.top_left = (ei_point_t){0, 0};
        parent_rect.size = surface_size;
    }


    int pos_x = params->x + (int)(params->rel_x * parent_rect.size.width);
    int pos_y = params->y + (int)(params->rel_y * parent_rect.size.height);


    int final_width, final_height;
    // ... (le reste du calcul de final_width et final_height reste inchangé) ...
    if (params->rel_width > 0.0f) {
        final_width = (int)(params->rel_width * parent_rect.size.width);
    } else if (params->width > 0) {
        final_width = params->width;
    } else {
        final_width = impl_widget->requested_size.width;
        // Si requested_size est aussi 0, la taille par défaut de la classe devrait déjà
        // être dans requested_size via setdefaultsfunc.
        // Si elle est toujours 0, c'est que le widget n'a pas de taille intrinsèque (ex: frame vide sans bordure).
    }

    if (params->rel_height > 0.0f) {
        final_height = (int)(params->rel_height * parent_rect.size.height);
    } else if (params->height > 0) {
        final_height = params->height;
    } else {
        final_height = impl_widget->requested_size.height;
    }


    // Adjust position based on anchor
    // ... (le switch pour l'ancre reste inchangé) ...
    switch (params->anchor) {
        case ei_anc_center:
            pos_x -= final_width / 2;
            pos_y -= final_height / 2;
            break;
        case ei_anc_north:
            pos_x -= final_width / 2;
            break;
        case ei_anc_northeast:
            pos_x -= final_width;
            break;
        case ei_anc_east:
            pos_x -= final_width;
            pos_y -= final_height / 2;
            break;
        case ei_anc_southeast:
            pos_x -= final_width;
            pos_y -= final_height;
            break;
        case ei_anc_south:
            pos_x -= final_width / 2;
            pos_y -= final_height;
            break;
        case ei_anc_southwest:
            pos_y -= final_height;
            break;
        case ei_anc_west:
            pos_y -= final_height / 2;
            break;
        case ei_anc_northwest:
        default:

            break;
    }



    impl_widget->screen_location.top_left.x = parent_rect.top_left.x + pos_x;
    impl_widget->screen_location.top_left.y = parent_rect.top_left.y + pos_y;
    impl_widget->screen_location.size.width = final_width;
    impl_widget->screen_location.size.height = final_height;

    // **APPELER GEOMNOTIFYFUNC ICI**
    // Il est crucial que geomnotifyfunc soit appelé *après* que screen_location soit à jour,
    // car geomnotifyfunc (surtout pour toplevel) utilise screen_location pour calculer son content_rect.
    // if (impl_widget->wclass && impl_widget->wclass->geomnotifyfunc) {
    //     if (impl_widget->geomnotify_in_progress) {
    //         return;
    //     }
    //     impl_widget->geomnotify_in_progress = true;
    //     impl_widget->wclass->geomnotifyfunc(widget);
    //     impl_widget->geomnotify_in_progress = false;
    // }
    if (impl_widget->wclass && impl_widget->wclass->geomnotifyfunc) {
        impl_widget->wclass->geomnotifyfunc(widget);
    }
    // Si `geomnotifyfunc` a mis à jour la géométrie des enfants,
    // ces enfants auront déjà été invalidés par leurs propres appels à `ei_impl_placer_run`.

    // Invalidate l'ancienne et la nouvelle position du widget
    // (si elles sont différentes et valides)
    if (old_screen_location.size.width > 0 && old_screen_location.size.height > 0 &&
        (old_screen_location.top_left.x != impl_widget->screen_location.top_left.x ||
         old_screen_location.top_left.y != impl_widget->screen_location.top_left.y ||
         old_screen_location.size.width != impl_widget->screen_location.size.width ||
         old_screen_location.size.height != impl_widget->screen_location.size.height)) {
        ei_app_invalidate_rect(&old_screen_location);
    }
    if (impl_widget->screen_location.size.width > 0 && impl_widget->screen_location.size.height > 0) {
        ei_app_invalidate_rect(&impl_widget->screen_location);
    }
}

void ei_place(ei_widget_t widget, ei_anchor_t* anchor, int* x, int* y, int* width, int* height,
              float* rel_x, float* rel_y, float* rel_width, float* rel_height) {
    assert(widget != NULL && "Widget cannot be NULL");

    ei_impl_widget_t* impl_widget = (ei_impl_widget_t*)widget;


    if (impl_widget->placer_params == NULL) {
        impl_widget->placer_params = (ei_impl_placer_params_t*)malloc(sizeof(ei_impl_placer_params_t));
        assert(impl_widget->placer_params != NULL && "Failed to allocate placer_params");

        impl_widget->placer_params->anchor = ei_anc_northwest;
        impl_widget->placer_params->x = 0;
        impl_widget->placer_params->y = 0;
        impl_widget->placer_params->width = 0;
        impl_widget->placer_params->height = 0;
        impl_widget->placer_params->rel_x = 0.0f;
        impl_widget->placer_params->rel_y = 0.0f;
        impl_widget->placer_params->rel_width = 0.0f;
        impl_widget->placer_params->rel_height = 0.0f;
    }

    ei_impl_placer_params_t* params = impl_widget->placer_params;


    if (anchor != NULL) params->anchor = *anchor;
    if (x != NULL) params->x = *x;
    if (y != NULL) params->y = *y;
    if (width != NULL) params->width = *width;
    if (height != NULL) params->height = *height;
    if (rel_x != NULL) params->rel_x = *rel_x;
    if (rel_y != NULL) params->rel_y = *rel_y;
    if (rel_width != NULL) params->rel_width = *rel_width;
    if (rel_height != NULL) params->rel_height = *rel_height;


    ei_impl_placer_run(widget);
}


void ei_placer_forget(ei_widget_t widget) {
    assert(widget != NULL && "Widget cannot be NULL");

    ei_impl_widget_t* impl_widget = (ei_impl_widget_t*)widget;


    ei_app_invalidate_rect(&impl_widget->screen_location);


    if (impl_widget->placer_params != NULL) {
        free(impl_widget->placer_params);
        impl_widget->placer_params = NULL;
    }


    impl_widget->screen_location = ei_rect_zero();
    if (impl_widget->content_rect != &impl_widget->screen_location) {
        free(impl_widget->content_rect);
        impl_widget->content_rect = &impl_widget->screen_location;
    }
}

