#include "ei_application.h"
#include "ei_widget.h"
#include "ei_widgetclass.h"
#include "ei_types.h"
#include "hw_interface.h"
#include "ei_implementation.h"
#include "ei_draw.h"
#include "ei_event.h"
#include "ei_utils.h"
#include <stdio.h>
#include <stdlib.h>

// Macro pour max, utilisée dans ei_toplevel.c
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

// Variables globales
static ei_surface_t g_root_surface = NULL;
extern ei_surface_t pick_surface;
static ei_widget_t g_root_widget = NULL;
static bool g_application_quit_request = false;
static ei_linked_rect_t* g_invalidated_rects_head = NULL;



// Fonction utilitaire pour fusionner deux rectangles
static ei_rect_t rect_union(const ei_rect_t* a, const ei_rect_t* b) {
    ei_rect_t result;
    result.top_left.x = min(a->top_left.x, b->top_left.x);
    result.top_left.y = min(a->top_left.y, b->top_left.y);
    int right_a = a->top_left.x + a->size.width;
    int right_b = b->top_left.x + b->size.width;
    int bottom_a = a->top_left.y + a->size.height;
    int bottom_b = b->top_left.y + b->size.height;
    result.size.width = max(right_a, right_b) - result.top_left.x;
    result.size.height = max(bottom_a, bottom_b) - result.top_left.y;
    return result;
}

void ei_app_create(ei_size_t main_window_size, bool fullscreen) {
    // Initialiser le matériel
    hw_init();

    // Charger la police par défaut
    ei_default_font = hw_text_font_create(ei_default_font_filename, ei_style_normal, ei_font_default_size);
    if (!ei_default_font) {
        fprintf(stderr, "Erreur fatale: Impossible de charger la police '%s'.\n", ei_default_font_filename);
        hw_quit();
        exit(1);
    }

    // Enregistrer les classes de widgets
    ei_frame_register_class();
    ei_button_register_class();
    ei_toplevel_register_class();

    // Créer la fenêtre principale
    g_root_surface = hw_create_window(main_window_size, fullscreen);
    if (!g_root_surface) {
        fprintf(stderr, "Erreur: Impossible de créer la fenêtre principale.\n");
        hw_text_font_free(ei_default_font);
        hw_quit();
        exit(1);
    }
    if (fullscreen) {
        main_window_size = hw_surface_get_size(g_root_surface);
    }

    // Créer la surface de picking (forcer alpha pour faciliter la détection via canal alpha)
    pick_surface = hw_surface_create(g_root_surface, main_window_size, true);
    if (!pick_surface) {
        fprintf(stderr, "Erreur: Impossible de créer la surface de picking.\n");
        hw_surface_free(g_root_surface);
        hw_text_font_free(ei_default_font);
        hw_quit();
        exit(1);
    }

    // Créer le widget racine (frame)
    g_root_widget = ei_widget_create("frame", NULL, NULL, NULL);
    if (!g_root_widget) {
        fprintf(stderr, "Erreur: Impossible de créer le widget racine.\n");
        hw_surface_free(pick_surface);
        hw_surface_free(g_root_surface);
        hw_text_font_free(ei_default_font);
        hw_quit();
        exit(1);
    }

    // Configurer le widget racine
    g_root_widget->screen_location.top_left = ei_point_zero();
    g_root_widget->screen_location.size = main_window_size;
    g_root_widget->content_rect = &g_root_widget->screen_location; // Frame racine
    g_root_widget->requested_size = main_window_size;

    // Invalider la zone initiale pour le premier dessin
    ei_app_invalidate_rect(&g_root_widget->screen_location);
}

static void redraw_invalidated_areas(void) {
    if (!g_root_widget || !g_root_widget->wclass || !g_root_widget->wclass->drawfunc) {
        return;
    }
    if (!g_invalidated_rects_head) {
        return;
    }

    hw_surface_lock(g_root_surface);
    hw_surface_lock(pick_surface);

    ei_color_t pick_clear_color = (ei_color_t){0, 0, 0, 0x00};

    // Dessiner chaque rectangle invalidé
    ei_linked_rect_t* current = g_invalidated_rects_head;
    while (current) {
        // Réinitialiser uniquement la zone à redessiner pour conserver les couleurs des autres widgets.
        ei_fill(pick_surface, &pick_clear_color, &current->rect);
        g_root_widget->wclass->drawfunc(g_root_widget, g_root_surface, pick_surface, &current->rect);
        current = current->next;
    }

    hw_surface_unlock(pick_surface);
    hw_surface_unlock(g_root_surface);

    // Mettre à jour l'écran
    hw_surface_update_rects(g_root_surface, g_invalidated_rects_head);

    // Libérer les rectangles invalidés
    ei_linked_rect_t* node = g_invalidated_rects_head;
    while (node) {
        ei_linked_rect_t* next = node->next;
        free(node);
        node = next;
    }
    g_invalidated_rects_head = NULL;
}

void ei_app_run(void) {
    if (!g_root_widget) {
        fprintf(stderr, "Erreur: Widget racine non initialisé.\n");
        return;
    }

    g_application_quit_request = false;
    ei_event_t event;

    while (!g_application_quit_request) {
        // Redessiner les zones invalidées
        redraw_invalidated_areas();

        // Attendre le prochain événement
        hw_event_wait_next(&event);

        // Gérer l'événement
        ei_widget_t target_widget = NULL;
        bool event_handled = false;

        // Cas spéciaux pour certains types d'événements
        switch (event.type) {
            case ei_ev_exposed:
                // Invalider toute la fenêtre pour redessiner
                ei_app_invalidate_rect(&g_root_widget->screen_location);
                event_handled = true;
                break;
            case ei_ev_close:
                ei_app_quit_request();
                event_handled = true;
                break;
            default:
                break;
        }

        // Sélectionner le widget cible
        if (!event_handled) {
            ei_widget_t active_widget = ei_event_get_active_widget();
            if (active_widget) {
                target_widget = active_widget;
            } else if (event.type >= ei_ev_mouse_buttondown && event.type <= ei_ev_mouse_wheel) {
                target_widget = ei_widget_pick(&event.param.mouse.where);
            } else if (event.type == ei_ev_keydown || event.type == ei_ev_keyup || event.type == ei_ev_text_input) {
                target_widget = ei_event_get_active_widget(); // Utiliser le widget actif pour le clavier
            }

            // Appeler le handlefunc du widget cible
            if (target_widget && target_widget->wclass && target_widget->wclass->handlefunc) {
                event_handled = target_widget->wclass->handlefunc(target_widget, &event);
            }

            // Si non géré, appeler le gestionnaire par défaut
            if (!event_handled) {
                ei_default_handle_func_t default_handler = ei_event_get_default_handle_func();
                if (default_handler) {
                    default_handler(&event);
                }
            }
        }
    }
}

void ei_app_free(void) {
    // Libérer les rectangles invalidés
    ei_linked_rect_t* current = g_invalidated_rects_head;
    while (current) {
        ei_linked_rect_t* next = current->next;
        free(current);
        current = next;
    }
    g_invalidated_rects_head = NULL;

    // Détruire le widget racine et ses enfants
    if (g_root_widget) {
        ei_widget_destroy(g_root_widget);
        g_root_widget = NULL;
    }

    // Libérer les surfaces
    if (pick_surface) {
        hw_surface_free(pick_surface);
        pick_surface = NULL;
    }
    if (g_root_surface) {
        hw_surface_free(g_root_surface);
        g_root_surface = NULL;
    }

    // Libérer la police
    if (ei_default_font) {
        hw_text_font_free(ei_default_font);
        ei_default_font = NULL;
    }

    // Réinitialiser l'état des événements
    ei_event_set_active_widget(NULL);
    ei_event_set_default_handle_func(NULL);

    // Libérer le matériel
    hw_quit();
}

void ei_app_invalidate_rect(const ei_rect_t* rect) {
    if (!rect || rect->size.width <= 0 || rect->size.height <= 0) {
        return;
    }

    if (g_root_surface == NULL) {
        return;
    }

    ei_rect_t clipped_rect = *rect;
    clamp_rect_to_surface(&clipped_rect, g_root_surface);
    if (clipped_rect.size.width <= 0 || clipped_rect.size.height <= 0) {
        return;
    }

    // Fusionner avec les rectangles existants si possible
    ei_linked_rect_t* current = g_invalidated_rects_head;
    ei_linked_rect_t* prev = NULL;
    while (current) {
        // Vérifier si le nouveau rectangle intersecte ou est adjacent
        int dx = max(current->rect.top_left.x, clipped_rect.top_left.x) -
                 min(current->rect.top_left.x + current->rect.size.width,
                     clipped_rect.top_left.x + clipped_rect.size.width);
        int dy = max(current->rect.top_left.y, clipped_rect.top_left.y) -
                 min(current->rect.top_left.y + current->rect.size.height,
                     clipped_rect.top_left.y + clipped_rect.size.height);
        if (dx <= 0 && dy <= 0) {
            // Fusionner les rectangles
            current->rect = rect_union(&current->rect, &clipped_rect);
            return;
        }
        prev = current;
        current = current->next;
    }

    // Ajouter un nouveau rectangle
    ei_linked_rect_t* new_node = malloc(sizeof(ei_linked_rect_t));
    if (!new_node) {
        fprintf(stderr, "Erreur: Impossible d'allouer un rectangle invalidé.\n");
        return;
    }
    new_node->rect = clipped_rect;
    new_node->next = g_invalidated_rects_head;
    g_invalidated_rects_head = new_node;
}

void ei_app_quit_request(void) {
    g_application_quit_request = true;
}

ei_widget_t ei_app_root_widget(void) {
    return g_root_widget;
}

ei_surface_t ei_app_root_surface(void) {
    return g_root_surface;
}
