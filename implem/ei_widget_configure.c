#include "ei_widget_configure.h"
#include "ei_implementation.h"
#include "ei_relief.h"
#include "hw_interface.h"
#include "ei_types.h"
#include "ei_utils.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "ei_draw.h"
#include "ei_placer.h"
#include "ei_event.h"
#include "ei_application.h"
#include <stdio.h>


#define max(a, b) ((a) > (b) ? (a) : (b))

#define TOPLEVEL_TITLE_BAR_HEIGHT 25
#define TOPLEVEL_DECORATION_SIZE 15 // Pour le bouton de fermeture et la poignée de redim.
#define TOPLEVEL_RESIZE_HANDLE_SIZE 15


void ei_frame_configure(ei_widget_t widget,
                        ei_size_t* requested_size,
                        const ei_color_t* color,
                        int* border_width,
                        ei_relief_t* relief,
                        ei_string_t* text,
                        ei_font_t* text_font,
                        ei_color_t* text_color,
                        ei_anchor_t* text_anchor,
                        ei_surface_t* img,
                        ei_rect_ptr_t* img_rect,
                        ei_anchor_t* img_anchor)
{
    assert(widget != NULL && "Widget cannot be NULL");
    ei_impl_frame_t* frame = (ei_impl_frame_t*)widget;


    bool geometry_changed = false;


    if (requested_size != NULL)
    {
        frame->widget.requested_size = *requested_size;
        geometry_changed = true;
    }
    if (color != NULL)
    {
        frame->color = *color;
    }
    if (border_width != NULL)
    {
        frame->border_width = *border_width;
        geometry_changed = true;
    }
    if (relief != NULL)
    {
        frame->relief = *relief;
    }
    if (text != NULL)
    {
        if (frame->text != NULL)
        {
            free(frame->text);
        }
        frame->text = *text != NULL ? strdup(*text) : NULL;
        frame->img = NULL; // Only one of text or img
        geometry_changed = true;
    }
    if (text_font != NULL)
    {
        frame->text_font = *text_font ? *text_font : ei_default_font;
        geometry_changed = true;
    }
    if (text_color != NULL)
    {
        frame->text_color = *text_color;
    }
    if (text_anchor != NULL)
    {
        frame->text_anchor = *text_anchor;
    }
    if (img != NULL)
    {
        frame->img = *img;
        frame->text = NULL;
        geometry_changed = true;
    }
    if (img_rect != NULL)
    {
        if (frame->img_rect != NULL)
        {
            free(frame->img_rect);
        }
        frame->img_rect = *img_rect != NULL ? malloc(sizeof(ei_rect_t)) : NULL;
        if (frame->img_rect != NULL)
        {
            *frame->img_rect = **img_rect;
        }
        geometry_changed = true;
    }
    if (img_anchor != NULL)
    {
        frame->img_anchor = *img_anchor;
    }


    if (geometry_changed)
    {
        ei_size_t natural_size = frame->widget.requested_size;
        if (frame->text != NULL && frame->text_font != NULL)
        {
            int text_width, text_height;
            hw_text_compute_size(frame->text, frame->text_font, &text_width, &text_height);
            natural_size.width = text_width + 2 * frame->border_width;
            natural_size.height = text_height + 2 * frame->border_width;
        }
        else if (frame->img != NULL)
        {
            ei_size_t img_size = hw_surface_get_size(frame->img);
            if (frame->img_rect != NULL)
            {
                img_size = frame->img_rect->size;
            }
            natural_size.width = img_size.width + 2 * frame->border_width;
            natural_size.height = img_size.height + 2 * frame->border_width;
        }
        frame->widget.requested_size.width = max(frame->widget.requested_size.width, natural_size.width);
        frame->widget.requested_size.height = max(frame->widget.requested_size.height, natural_size.height);
    }


    if (geometry_changed && frame->widget.placer_params != NULL)
    {
        ei_impl_placer_run(widget);
    }
}

ei_widget_t frame_allocfunc(void)
{
    ei_impl_frame_t* frame = calloc(1, sizeof(ei_impl_frame_t));
    if (!frame)
    {
        fprintf(stderr, "Erreur d'allocation pour frame.\n");
        return NULL;
    }
    frame->widget.content_rect = &frame->widget.screen_location; // Par défaut
    return (ei_widget_t)frame;
}

void frame_releasefunc(ei_widget_t widget)
{
    ei_impl_frame_t* frame = (ei_impl_frame_t*)widget;

    if (frame->img_rect != NULL)
    {
        free(frame->img_rect);
        frame->img_rect = NULL;
    }
}


void frame_drawfunc(ei_widget_t widget, ei_surface_t surface, ei_surface_t pick_surface, ei_rect_t* clipper)
{
    ei_impl_frame_t* frame = (ei_impl_frame_t*)widget;
    ei_rect_t draw_rect = frame->widget.screen_location;
    if (clipper && !intersection_rect(&draw_rect, &frame->widget.screen_location, clipper))
    {
        return; // Pas d’intersection
    }

    // D'abord, peindre le widget sur la surface de picking pour que les enfants puissent le recouvrir ensuite.
    ei_fill(pick_surface, &frame->widget.pick_color, &draw_rect);

    // Dessiner le fond
    if (frame->relief == ei_relief_none && frame->border_width == 0)
    {
        ei_fill(surface, &frame->color, &draw_rect);
    }
    else
    {
        draw_button(surface, &frame->widget.screen_location, 0.0f, frame->color, frame->border_width, frame->relief,
                    clipper);
    }

    // Calculer la zone de contenu
    ei_rect_t content_area = frame->widget.screen_location;
    content_area.top_left.x += frame->border_width;
    content_area.top_left.y += frame->border_width;
    content_area.size.width -= 2 * frame->border_width;
    content_area.size.height -= 2 * frame->border_width;
    if (content_area.size.width < 0) content_area.size.width = 0;
    if (content_area.size.height < 0) content_area.size.height = 0;

    // Dessiner le texte
    if (frame->text != NULL && strlen(frame->text) > 0)
    {
        int text_width, text_height;
        hw_text_compute_size(frame->text, frame->text_font, &text_width, &text_height);
        ei_point_t text_pos;
        switch (frame->text_anchor)
        {
        case ei_anc_northwest:
            text_pos = content_area.top_left;
            break;
        case ei_anc_north:
            text_pos.x = content_area.top_left.x + (content_area.size.width - text_width) / 2;
            text_pos.y = content_area.top_left.y;
            break;
        case ei_anc_center:
        default:
            text_pos.x = content_area.top_left.x + (content_area.size.width - text_width) / 2;
            text_pos.y = content_area.top_left.y + (content_area.size.height - text_height) / 2;
            break;
        }
        ei_draw_text(surface, &text_pos, frame->text, frame->text_font, frame->text_color, &draw_rect);
    }

    // Dessiner l’image
    if (frame->img != NULL)
    {
        ei_rect_t src_img_rect;
        if (frame->img_rect != NULL)
        {
            src_img_rect = *frame->img_rect;
        }
        else
        {
            src_img_rect.top_left = ei_point_zero();
            src_img_rect.size = hw_surface_get_size(frame->img);
        }
        ei_point_t img_pos;
        switch (frame->img_anchor)
        {
        case ei_anc_northwest:
            img_pos = content_area.top_left;
            break;
        case ei_anc_center:
        default:
            img_pos.x = content_area.top_left.x + (content_area.size.width - src_img_rect.size.width) / 2;
            img_pos.y = content_area.top_left.y + (content_area.size.height - src_img_rect.size.height) / 2;
            break;
        }
        ei_rect_t dst_img_rect = {img_pos, src_img_rect.size};
        hw_surface_lock(frame->img);
        ei_copy_surface(surface, &dst_img_rect, frame->img, &src_img_rect, hw_surface_has_alpha(frame->img));
        hw_surface_unlock(frame->img);
    }

    // Dessiner les enfants
    ei_rect_t children_clipper;
    if (intersection_rect(&children_clipper, &content_area, clipper))
    {
        ei_impl_widget_draw_children(widget, surface, pick_surface, &children_clipper);
    }
}

void frame_setdefaultsfunc(ei_widget_t widget)
{
    ei_impl_frame_t* frame = (ei_impl_frame_t*)widget;
    frame->widget.requested_size = ei_size(0, 0);
    frame->color = ei_default_background_color;
    frame->border_width = 0;
    frame->relief = ei_relief_none;
    frame->text = NULL;
    frame->text_font = ei_default_font;
    frame->text_color = ei_font_default_color;
    frame->text_anchor = ei_anc_center;
    frame->img = NULL;
    frame->img_rect = NULL;
    frame->img_anchor = ei_anc_center;
}

static ei_widgetclass_t g_frame_class;

void ei_frame_register_class(void)
{
    strncpy(g_frame_class.name, "frame", sizeof(g_frame_class.name) - 1);
    g_frame_class.name[sizeof(g_frame_class.name) - 1] = '\0';
    g_frame_class.allocfunc = frame_allocfunc;
    g_frame_class.releasefunc = frame_releasefunc;
    g_frame_class.drawfunc = frame_drawfunc;
    g_frame_class.setdefaultsfunc = frame_setdefaultsfunc;
    g_frame_class.geomnotifyfunc = NULL;
    g_frame_class.handlefunc = NULL;
    g_frame_class.next = NULL;
    ei_widgetclass_register(&g_frame_class);
}


void ei_button_configure(ei_widget_t widget, ei_size_t* requested_size, const ei_color_t* color, int* border_width,
                         int* corner_radius, ei_relief_t* relief, ei_string_t* text, ei_font_t* text_font,
                         ei_color_t* text_color, ei_anchor_t* text_anchor, ei_surface_t* img_ptr,
                         /* Renommé pour clarifier que c'est un pointeur vers la surface source */
                         ei_rect_ptr_t* img_rect_ptr, /* Renommé pour clarifier */ ei_anchor_t* img_anchor,
                         ei_callback_t* callback, ei_user_param_t* user_param)
{
    assert(widget != NULL && "Widget cannot be NULL");
    ei_impl_button_t* button = (ei_impl_button_t*)widget;
    assert(strcmp(button->widget.wclass->name, "button") == 0 && "Widget must be a button");

    bool geometry_changed = false;
    bool image_changed = false; // Pour savoir si on doit recalculer la taille à cause de l'image

    // Gérer d'abord les attributs qui ne dépendent pas de l'image/texte
    if (color != NULL)
    {
        button->color = *color;
    }
    if (border_width != NULL)
    {
        button->border_width = *border_width;
        geometry_changed = true;
    }
    if (relief != NULL)
    {
        button->relief = *relief;
    }
    if (corner_radius != NULL)
    {
        button->corner_radius = *corner_radius;
    }
    if (callback != NULL)
    {
        button->callback = *callback;
    }
    if (user_param != NULL)
    {
        button->user_param = *user_param;
    }
    if (text_anchor != NULL)
    {
        // Peut être configuré même si le texte vient plus tard
        button->text_anchor = *text_anchor;
    }
    if (img_anchor != NULL)
    {
        // Peut être configuré même si l'image vient plus tard
        button->img_anchor = *img_anchor;
    }
    if (text_color != NULL)
    {
        button->text_color = *text_color;
    }
    if (text_font != NULL)
    {
        button->text_font = *text_font ? *text_font : ei_default_font;
        geometry_changed = true; // La police peut affecter la taille si le texte est présent
    }


    // Gérer le texte
    if (text != NULL)
    {
        if (button->text != NULL)
        {
            free(button->text);
        }
        button->text = (*text != NULL) ? strdup(*text) : NULL;

        // Si on définit un texte, on s'assure que l'ancienne image du bouton est libérée
        if (button->text != NULL && button->img != NULL)
        {
            hw_surface_free(button->img);
            button->img = NULL;
            if (button->img_rect != NULL)
            {
                // Ce champ n'est plus pertinent pour la propre surface du bouton
                free(button->img_rect);
                button->img_rect = NULL;
            }
        }
        geometry_changed = true;
        image_changed = true; // Pour forcer le recalcul de la taille
    }


    // Gérer l'image (Option 2 : créer une copie de la portion)
    if (img_ptr != NULL)
    {
        // Si le paramètre *img_ptr est fourni
        ei_surface_t source_surface = *img_ptr;

        // Libérer l'ancienne image propre au bouton s'il y en avait une
        if (button->img != NULL)
        {
            hw_surface_free(button->img);
            button->img = NULL;
        }
        // button->img_rect n'est plus utilisé pour stocker le rectangle source une fois la copie faite.
        if (button->img_rect != NULL)
        {
            free(button->img_rect);
            button->img_rect = NULL;
        }


        if (source_surface != NULL)
        {
            ei_rect_t source_rect_for_copy;
            if (img_rect_ptr != NULL && *img_rect_ptr != NULL)
            {
                source_rect_for_copy = **img_rect_ptr;
            }
            else
            {
                // Si pas de img_rect fourni, on prend toute la surface source
                source_rect_for_copy.top_left = ei_point_zero();
                source_rect_for_copy.size = hw_surface_get_size(source_surface);
            }

            if (source_rect_for_copy.size.width > 0 && source_rect_for_copy.size.height > 0)
            {
                // Créer une nouvelle surface pour le bouton de la taille de la portion à copier
                button->img = hw_surface_create(ei_app_root_surface(), source_rect_for_copy.size,
                                                hw_surface_has_alpha(source_surface));

                if (button->img)
                {
                    hw_surface_lock(source_surface);
                    hw_surface_lock(button->img);

                    ei_rect_t dest_rect_on_button_img = {{0, 0}, source_rect_for_copy.size};
                    ei_copy_surface(button->img, &dest_rect_on_button_img, source_surface, &source_rect_for_copy,
                                    false);

                    hw_surface_unlock(button->img);
                    hw_surface_unlock(source_surface);

                    // Le bouton a maintenant sa propre image, on s'assure que le texte est NULL
                    if (button->text != NULL)
                    {
                        free(button->text);
                        button->text = NULL;
                    }
                }
                else
                {
                    fprintf(stderr, "Erreur: Impossible de créer la surface pour l'image du bouton.\n");
                    // button->img reste NULL
                }
            }
            else
            {
                button->img = NULL; // La portion source est vide
            }
        }
        else
        {
            // Le paramètre *img_ptr était NULL, donc on efface l'image du bouton
            button->img = NULL;
        }
        geometry_changed = true;
        image_changed = true;
    }


    // Mise à jour de requested_size en fonction du texte ou de la nouvelle image propre au bouton
    // Doit être fait après que button->text ou button->img (la copie) est défini.
    // 'image_changed' ou 'geometry_changed' (si la bordure a changé par ex.) peut déclencher ça
    if (geometry_changed || image_changed)
    {
        ei_size_t natural_content_size = {0, 0};

        if (button->text != NULL && button->text_font != NULL)
        {
            assert(button->text_font != NULL); // Devrait être initialisé par setdefaults ou paramètre
            int text_w, text_h;
            hw_text_compute_size(button->text, button->text_font, &text_w, &text_h);
            natural_content_size.width = text_w;
            natural_content_size.height = text_h;
        }
        else if (button->img != NULL)
        {
            // L'image du bouton est maintenant sa propre surface, donc on prend sa taille totale
            natural_content_size = hw_surface_get_size(button->img);
        }

        ei_size_t final_requested_size;
        final_requested_size.width = natural_content_size.width + 2 * button->border_width;
        final_requested_size.height = natural_content_size.height + 2 * button->border_width;

        if (requested_size != NULL)
        {
            // Si une taille a été explicitement passée en paramètre
            button->widget.requested_size.width = max(requested_size->width, final_requested_size.width);
            button->widget.requested_size.height = max(requested_size->height, final_requested_size.height);
        }
        else
        {
            // Sinon, on utilise la taille naturelle calculée
            button->widget.requested_size = final_requested_size;
        }
    }
    else if (requested_size != NULL)
    {
        // Si seule requested_size est passée sans autre changement de géométrie
        button->widget.requested_size = *requested_size;
    }


    // Recalculer la géométrie si le widget est déjà placé et que quelque chose a changé
    if ((geometry_changed || image_changed) && button->widget.placer_params != NULL)
    {
        ei_impl_placer_run(widget);
    }
}


ei_widget_t button_allocfunc(void)
{
    ei_impl_button_t* button = calloc(1, sizeof(ei_impl_button_t));
    if (!button)
    {
        fprintf(stderr, "Erreur d'allocation pour button.\n");
        return NULL;
    }
    button->widget.content_rect = &button->widget.screen_location; // Par défaut
    return (ei_widget_t)button;
}

void button_releasefunc(ei_widget_t widget)
{
    ei_impl_button_t* button = (ei_impl_button_t*)widget;
    if (button->text != NULL)
    {
        free(button->text);
        button->text = NULL;
    }
    // button->img_rect n'est plus utilisé pour stocker une sous-partie
    // car button->img est la sous-partie elle-même. On le libère s'il avait été alloué.
    if (button->img_rect != NULL)
    {
        free(button->img_rect);
        button->img_rect = NULL;
    }
    // Libérer la surface de l'image propre au bouton
    if (button->img != NULL)
    {
        hw_surface_free(button->img);
        button->img = NULL;
    }
}

void button_drawfunc(ei_widget_t widget, ei_surface_t surface, ei_surface_t pick_surface, ei_rect_t* clipper)
{
    ei_impl_button_t* button = (ei_impl_button_t*)widget;
    ei_rect_t widget_screen_location = button->widget.screen_location; // Rectangle total du widget
    ei_rect_t clipped_drawing_rect = widget_screen_location; // Zone à dessiner du widget après clipping global

    // 1. Appliquer le clipper global au rectangle total du widget
    if (clipper && !intersection_rect(&clipped_drawing_rect, &widget_screen_location, clipper))
    {
        return; // Le widget est entièrement en dehors du clipper global, rien à dessiner
    }

    // Peindre la couleur de picking du bouton en premier afin que ses enfants puissent la recouvrir.
    ei_fill(pick_surface, &button->widget.pick_color, &clipped_drawing_rect);

    // 2. Dessiner le bouton lui-même (fond, relief, bordure)
    //    Cette partie est dessinée sur la totalité de clipped_drawing_rect.
    ei_relief_t current_relief = button->is_pressed ? ei_relief_sunken : button->relief;
    // La fonction draw_button de ei_relief.c est utilisée ici pour le rendu du bouton lui-même.
    // Elle prend en compte le rayon, la couleur, la bordure, le relief.
    // Le clipper passé à cette fonction de bas niveau est clipped_drawing_rect.
    draw_button(surface, &widget_screen_location, (float)button->corner_radius,
                button->color, button->border_width, current_relief, &clipped_drawing_rect);

    // 3. Obtenir le content_rect du widget (déjà en coordonnées d'écran)
    //    Ce content_rect a été calculé par le placer ou geomnotify.
    //    Il définit la zone où le texte/image doit être dessiné.
    const ei_rect_t* widget_content_rect = button->widget.content_rect;
    if (!widget_content_rect)
    {
        // Sécurité, devrait toujours être initialisé
        widget_content_rect = &button->widget.screen_location; // Fallback, mais problématique
    }

    // 4. Calculer le clipper effectif pour le contenu :
    //    Intersection du content_rect du widget et du clipped_drawing_rect (qui est déjà le widget clippé).
    ei_rect_t content_clipper;
    if (!intersection_rect(&content_clipper, widget_content_rect, &clipped_drawing_rect))
    {
        // La zone de contenu clippée est vide. On dessine quand même le widget sur la pick_surface.
        ei_fill(pick_surface, &button->widget.pick_color, &clipped_drawing_rect);
        // Pas besoin de dessiner les enfants si la zone de contenu est vide après clipping.
        return;
    }

    // 5. Dessiner le texte (dans widget_content_rect, clippé par content_clipper)
    if (button->text != NULL && strlen(button->text) > 0 && button->text_font != NULL)
    {
        assert(button->text_font != NULL);
        int text_width, text_height;
        hw_text_compute_size(button->text, button->text_font, &text_width, &text_height);
        ei_point_t text_pos;

        // L'ancrage se fait par rapport au widget_content_rect (non clippé)
        switch (button->text_anchor)
        {
        case ei_anc_northwest: text_pos = widget_content_rect->top_left;
            break;
        case ei_anc_north: text_pos = ei_point(
                widget_content_rect->top_left.x + (widget_content_rect->size.width - text_width) / 2,
                widget_content_rect->top_left.y);
            break;
        // ... autres cas d'ancrage ...
        case ei_anc_center:
        default: text_pos = ei_point(
                widget_content_rect->top_left.x + (widget_content_rect->size.width - text_width) / 2,
                widget_content_rect->top_left.y + (widget_content_rect->size.height - text_height) / 2);
            break;
        }
        if (button->is_pressed)
        {
            // Décalage si pressé
            text_pos.x += 1;
            text_pos.y += 1;
        }
        // Dessiner le texte, clippé par content_clipper
        ei_draw_text(surface, &text_pos, button->text, button->text_font, button->text_color, &content_clipper);
    }
    // 6. Dessiner l’image (dans widget_content_rect, clippé par content_clipper)
    else if (button->img != NULL)
    {
        // Utiliser 'else if' si texte et image sont mutuellement exclusifs
        ei_rect_t src_img_rect_for_copy; // La source est maintenant toute l'image du bouton
        src_img_rect_for_copy.top_left = ei_point_zero();
        src_img_rect_for_copy.size = hw_surface_get_size(button->img);

        if (src_img_rect_for_copy.size.width > 0 && src_img_rect_for_copy.size.height > 0)
        {
            ei_point_t img_render_pos; // Position de rendu de l'image (coin sup gauche)

            // L'ancrage se fait par rapport au widget_content_rect (non clippé)
            switch (button->img_anchor)
            {
            case ei_anc_northwest: img_render_pos = widget_content_rect->top_left;
                break;
            // ... autres cas d'ancrage ...
            case ei_anc_center:
            default: img_render_pos = ei_point(
                    widget_content_rect->top_left.x + (widget_content_rect->size.width - src_img_rect_for_copy.size.
                        width) / 2,
                    widget_content_rect->top_left.y + (widget_content_rect->size.height - src_img_rect_for_copy.size.
                        height) / 2);
                break;
            }
            if (button->is_pressed)
            {
                // Décalage si pressé
                img_render_pos.x += 1;
                img_render_pos.y += 1;
            }

            ei_rect_t dst_rect_for_img_on_surface = {img_render_pos, src_img_rect_for_copy.size};

            // Le dessin de l'image doit être clippé par content_clipper
            ei_rect_t final_clipped_dst_rect_for_img;
            if (intersection_rect(&final_clipped_dst_rect_for_img, &dst_rect_for_img_on_surface, &content_clipper))
            {
                // Ajuster la portion de la source à copier si la destination est clippée
                ei_rect_t adjusted_src_rect_for_copy = src_img_rect_for_copy;
                adjusted_src_rect_for_copy.top_left.x += final_clipped_dst_rect_for_img.top_left.x -
                    dst_rect_for_img_on_surface.top_left.x;
                adjusted_src_rect_for_copy.top_left.y += final_clipped_dst_rect_for_img.top_left.y -
                    dst_rect_for_img_on_surface.top_left.y;
                adjusted_src_rect_for_copy.size = final_clipped_dst_rect_for_img.size;

                if (adjusted_src_rect_for_copy.size.width > 0 && adjusted_src_rect_for_copy.size.height > 0)
                {
                    hw_surface_lock(button->img);
                    ei_copy_surface(surface, &final_clipped_dst_rect_for_img, button->img, &adjusted_src_rect_for_copy,
                                    hw_surface_has_alpha(button->img));
                    hw_surface_unlock(button->img);
                }
            }
        }
    }

    // 7. Dessiner les enfants (clippés par content_clipper)
    ei_impl_widget_draw_children(widget, surface, pick_surface, &content_clipper);
    // Important: peindre la surface de picking AVANT les enfants aurait recouvert leur couleur.
    // Ici, on peint la couleur de picking du bouton dès le début pour que les enfants puissent la recouvrir.
}

void button_setdefaultsfunc(ei_widget_t widget)
{
    ei_impl_button_t* button = (ei_impl_button_t*)widget;
    button->widget.requested_size = ei_size(0, 0);
    button->color = ei_default_background_color;
    button->border_width = k_default_button_border_width;
    button->relief = ei_relief_raised;
    button->text = NULL;
    button->text_font = ei_default_font;
    button->text_color = ei_font_default_color;
    button->text_anchor = ei_anc_center;
    button->img = NULL;
    button->img_rect = NULL;
    button->img_anchor = ei_anc_center;
    button->corner_radius = k_default_button_corner_radius;
    button->callback = NULL;
    button->user_param = NULL;
    button->is_pressed = false;
}

static bool point_in_rect(ei_point_t point, const ei_rect_t* rect)
{
    if (!rect) return false;
    return (point.x >= rect->top_left.x &&
        point.x < rect->top_left.x + rect->size.width &&
        point.y >= rect->top_left.y &&
        point.y < rect->top_left.y + rect->size.height);
}

bool button_handlefunc(ei_widget_t widget, ei_event_t* event)
{
    ei_impl_button_t* button = (ei_impl_button_t*)widget;
    const ei_rect_t* screen_loc = &button->widget.screen_location;
    bool event_handled = false;

    switch (event->type)
    {
    case ei_ev_mouse_buttondown:
        if (event->param.mouse.button == ei_mouse_button_left &&
            point_in_rect(event->param.mouse.where, screen_loc))
        {
            bool was_pressed = button->is_pressed;
            button->is_pressed = true;
            ei_event_set_active_widget(widget);
            if (!was_pressed)
            {
                ei_app_invalidate_rect(screen_loc);
            }
            event_handled = true;
        }
        break;

    case ei_ev_mouse_move:
        if (ei_event_get_active_widget() == widget)
        {
            bool inside = point_in_rect(event->param.mouse.where, screen_loc);
            bool was_pressed = button->is_pressed;
            button->is_pressed = inside;
            if (was_pressed != button->is_pressed)
            {
                ei_app_invalidate_rect(screen_loc);
            }
            event_handled = true;
        }
        break;

    case ei_ev_mouse_buttonup:
        if (ei_event_get_active_widget() == widget &&
            event->param.mouse.button == ei_mouse_button_left)
        {
            bool was_pressed = button->is_pressed;
            bool inside = point_in_rect(event->param.mouse.where, screen_loc);
            button->is_pressed = false;
            ei_event_set_active_widget(NULL);
            if (was_pressed)
            {
                ei_app_invalidate_rect(screen_loc);
            }
            if (was_pressed && inside && button->callback != NULL)
            {
                button->callback(widget, event, button->user_param);
            }
            event_handled = true;
        }
        break;

    case ei_ev_keydown:
        if (ei_event_get_active_widget() == widget &&
            (event->param.key_code == SDLK_RETURN || event->param.key_code == SDLK_SPACE))
        {
            if (!button->is_pressed)
            {
                button->is_pressed = true;
                ei_app_invalidate_rect(screen_loc);
            }
            event_handled = true;
        }
        break;

    case ei_ev_keyup:
        if (ei_event_get_active_widget() == widget &&
            (event->param.key_code == SDLK_RETURN || event->param.key_code == SDLK_SPACE))
        {
            bool was_pressed = button->is_pressed;
            button->is_pressed = false;
            if (was_pressed)
            {
                ei_app_invalidate_rect(screen_loc);
                if (button->callback != NULL)
                {
                    button->callback(widget, event, button->user_param);
                }
            }
            event_handled = true;
        }
        break;

    default:
        break;
    }
    return event_handled;
}

static ei_widgetclass_t g_button_class_struct;

void ei_button_register_class(void)
{
    strncpy(g_button_class_struct.name, "button", sizeof(g_button_class_struct.name) - 1);
    g_button_class_struct.name[sizeof(g_button_class_struct.name) - 1] = '\0';
    g_button_class_struct.allocfunc = button_allocfunc;
    g_button_class_struct.releasefunc = button_releasefunc;
    g_button_class_struct.drawfunc = button_drawfunc;
    g_button_class_struct.setdefaultsfunc = button_setdefaultsfunc;
    g_button_class_struct.geomnotifyfunc = NULL;
    g_button_class_struct.handlefunc = button_handlefunc;
    g_button_class_struct.next = NULL;
    ei_widgetclass_register(&g_button_class_struct);
}


void ei_toplevel_configure(ei_widget_t widget,
                           ei_size_t* requested_size,
                           const ei_color_t* color,
                           int* border_width,
                           ei_string_t* title,
                           bool* closable,
                           ei_axis_set_t* resizable,
                           ei_size_ptr_t* min_size)
{
    assert(widget != NULL && "Widget cannot be NULL");
    ei_impl_toplevel_t* toplevel = (ei_impl_toplevel_t*)widget;
    assert(strcmp(toplevel->widget.wclass->name, "toplevel") == 0 && "Widget must be a toplevel");

    bool geometry_changed = false;

    // Update attributes if provided
    if (requested_size != NULL)
    {
        toplevel->widget.requested_size = *requested_size;
        geometry_changed = true;
    }
    if (color != NULL)
    {
        toplevel->color = *color;
    }
    if (border_width != NULL)
    {
        toplevel->border_width = *border_width;
        geometry_changed = true;
    }
    if (title != NULL)
    {
        if (toplevel->title != NULL)
        {
            free(toplevel->title);
        }
        toplevel->title = *title != NULL ? strdup(*title) : strdup("Toplevel");
        geometry_changed = true;
    }
    if (closable != NULL)
    {
        toplevel->closable = *closable;
    }
    if (resizable != NULL)
    {
        toplevel->resizable = *resizable;
    }
    if (min_size != NULL)
    {
        toplevel->min_size = **min_size;
    }


    if (geometry_changed && toplevel->widget.placer_params != NULL)
    {
        ei_impl_placer_run(widget);
    }
}

ei_widget_t toplevel_allocfunc(void)
{
    ei_impl_toplevel_t* toplevel = calloc(1, sizeof(ei_impl_toplevel_t));

    toplevel->widget.content_rect = malloc(sizeof(ei_rect_t));

    *toplevel->widget.content_rect = ei_rect_zero();
    return (ei_widget_t)toplevel;
}

void toplevel_releasefunc(ei_widget_t widget)
{
    ei_impl_toplevel_t* toplevel = (ei_impl_toplevel_t*)widget;

    (void)toplevel; // unused if no frees
}

static void toplevel_geomnotifyfunc(ei_widget_t widget)
{
    ei_impl_toplevel_t* toplevel = (ei_impl_toplevel_t*)widget;

    // La screen_location.size du widget a été fixée par placer_run
    // à la requested_size (qui est la taille du contenu pour un toplevel).
    // Nous devons maintenant ajuster screen_location.size pour inclure les décorations.
    // Et ensuite, nous recalculerons content_rect pour qu'il corresponde à la requested_size originale.

    ei_size_t content_size = toplevel->widget.screen_location.size; // C'était la requested_size (contenu)

    // Ajuster la taille totale de la fenêtre (screen_location.size)
    toplevel->widget.screen_location.size.width = content_size.width + 2 * toplevel->border_width;
    toplevel->widget.screen_location.size.height = content_size.height + 2 * toplevel->border_width + toplevel->
        title_bar_height;

    // Mettre à jour content_rect
    // Il est relatif à la screen_location.top_left du toplevel
    toplevel->widget.content_rect->top_left.x = toplevel->widget.screen_location.top_left.x + toplevel->border_width;
    toplevel->widget.content_rect->top_left.y = toplevel->widget.screen_location.top_left.y + toplevel->border_width +
        toplevel->title_bar_height;
    toplevel->widget.content_rect->size = content_size; // La taille du contenu est celle demandée

    // Assurer des tailles non négatives pour content_rect (au cas où la fenêtre serait trop petite)
    if (toplevel->widget.content_rect->size.width < 0) toplevel->widget.content_rect->size.width = 0;
    if (toplevel->widget.content_rect->size.height < 0) toplevel->widget.content_rect->size.height = 0;


    // Mettre à jour les rectangles interactifs (title_bar_rect, close_button_rect, resize_handle_rect)
    // Ces calculs sont relatifs à la screen_location.top_left de la fenêtre toplevel
    toplevel->title_bar_rect.top_left.x = toplevel->widget.screen_location.top_left.x + toplevel->border_width;
    toplevel->title_bar_rect.top_left.y = toplevel->widget.screen_location.top_left.y + toplevel->border_width;
    toplevel->title_bar_rect.size.width = content_size.width; // La barre de titre a la largeur du contenu
    toplevel->title_bar_rect.size.height = toplevel->title_bar_height;
    if (toplevel->title_bar_rect.size.width < 0) toplevel->title_bar_rect.size.width = 0;

    toplevel->close_button_rect = ei_rect_zero(); // Initialiser
    if (toplevel->closable)
    {
        toplevel->close_button_rect.top_left.x = toplevel->title_bar_rect.top_left.x + 2; // Marge
        toplevel->close_button_rect.top_left.y = toplevel->title_bar_rect.top_left.y + (toplevel->title_bar_rect.size.
            height - TOPLEVEL_DECORATION_SIZE) / 2;
        toplevel->close_button_rect.size.width = TOPLEVEL_DECORATION_SIZE;
        toplevel->close_button_rect.size.height = TOPLEVEL_DECORATION_SIZE;
    }

    toplevel->resize_handle_rect = ei_rect_zero(); // Initialiser
    if (toplevel->resizable != ei_axis_none)
    {
        toplevel->resize_handle_rect.size.width = TOPLEVEL_RESIZE_HANDLE_SIZE;
        toplevel->resize_handle_rect.size.height = TOPLEVEL_RESIZE_HANDLE_SIZE;
        // Positionné dans le coin inférieur droit de la *fenêtre totale* (screen_location)
        toplevel->resize_handle_rect.top_left.x = toplevel->widget.screen_location.top_left.x + toplevel->widget.
            screen_location.size.width - toplevel->border_width - toplevel->resize_handle_rect.size.width;
        toplevel->resize_handle_rect.top_left.y = toplevel->widget.screen_location.top_left.y + toplevel->widget.
            screen_location.size.height - toplevel->border_width - toplevel->resize_handle_rect.size.height;
    }

    // Mettre à jour la géométrie des enfants
    // Les enfants seront placés par rapport au *nouveau* content_rect
    ei_widget_t child = toplevel->widget.children_head;
    while (child != NULL)
    {
        if (child->placer_params != NULL)
        {
            ei_impl_placer_run(child); // Ceci va invalider les enfants
        }
        child = child->next_sibling;
    }
    // L'invalidation du toplevel lui-même est déjà gérée par l'appelant de geomnotify (placer_run)
}

void toplevel_drawfunc(ei_widget_t widget, ei_surface_t surface, ei_surface_t pick_surface, ei_rect_t* clipper)
{
    ei_impl_toplevel_t* toplevel = (ei_impl_toplevel_t*)widget;
    ei_rect_t draw_rect = toplevel->widget.screen_location;
    if (clipper && !intersection_rect(&draw_rect, &toplevel->widget.screen_location, clipper))
    {
        return; // Pas d'intersection
    }

    // Clamp draw_rect to surface boundaries to prevent out-of-bounds drawing
    clamp_rect_to_surface(&draw_rect, surface);
    if (draw_rect.size.width <= 0 || draw_rect.size.height <= 0)
    {
        return; // Nothing to draw
    }

    // D'abord, remplir la surface de picking avec la couleur du toplevel.
    ei_fill(pick_surface, &toplevel->widget.pick_color, &draw_rect);

    // Dessiner la bordure
    if (toplevel->border_width > 0)
    {
        ei_color_t border_color = {0x30, 0x30, 0x30, 0xff};
        ei_rect_t border_part;
        // Haut
        border_part = ei_rect(toplevel->widget.screen_location.top_left,
                              ei_size(toplevel->widget.screen_location.size.width, toplevel->border_width));
        if (intersection_rect(&border_part, &border_part, &draw_rect))
        {
            ei_fill(surface, &border_color, &border_part);
        }
        // Bas
        border_part = ei_rect(ei_point(toplevel->widget.screen_location.top_left.x,
                                       toplevel->widget.screen_location.top_left.y + toplevel->widget.screen_location.
                                       size.height - toplevel->border_width),
                              ei_size(toplevel->widget.screen_location.size.width, toplevel->border_width));
        if (intersection_rect(&border_part, &border_part, &draw_rect))
        {
            ei_fill(surface, &border_color, &border_part);
        }
        // Gauche
        border_part = ei_rect(ei_point(toplevel->widget.screen_location.top_left.x,
                                       toplevel->widget.screen_location.top_left.y + toplevel->border_width),
                              ei_size(toplevel->border_width,
                                      toplevel->widget.screen_location.size.height - 2 * toplevel->border_width));
        if (intersection_rect(&border_part, &border_part, &draw_rect))
        {
            ei_fill(surface, &border_color, &border_part);
        }
        // Droite
        border_part = ei_rect(
            ei_point(
                toplevel->widget.screen_location.top_left.x + toplevel->widget.screen_location.size.width - toplevel->
                border_width, toplevel->widget.screen_location.top_left.y + toplevel->border_width),
            ei_size(toplevel->border_width, toplevel->widget.screen_location.size.height - 2 * toplevel->border_width));
        if (intersection_rect(&border_part, &border_part, &draw_rect))
        {
            ei_fill(surface, &border_color, &border_part);
        }
    }

    // Dessiner la barre de titre (utiliser une variable locale pour ne pas corrompre title_bar_rect)
    ei_rect_t visible_title_bar;
    if (intersection_rect(&visible_title_bar, &toplevel->title_bar_rect, &draw_rect))
    {
        ei_color_t title_bar_color = {0x80, 0x80, 0x80, 0xff};
        ei_fill(surface, &title_bar_color, &visible_title_bar);
        if (toplevel->title)
        {
            int text_width, text_height;
            hw_text_compute_size(toplevel->title, ei_default_font, &text_width, &text_height);
            ei_point_t text_pos;
            text_pos.x = toplevel->title_bar_rect.top_left.x + 5;
            if (toplevel->closable) text_pos.x += TOPLEVEL_DECORATION_SIZE + 2;
            text_pos.y = toplevel->title_bar_rect.top_left.y + (toplevel->title_bar_rect.size.height - text_height) / 2;
            ei_draw_text(surface, &text_pos, toplevel->title, ei_default_font, ei_font_default_color,
                         &visible_title_bar);
        }
    }

    // Dessiner le bouton de fermeture (utiliser une variable locale pour ne pas corrompre close_button_rect)
    ei_rect_t visible_close_btn;
    if (toplevel->closable && intersection_rect(&visible_close_btn, &toplevel->close_button_rect, &draw_rect))
    {
        draw_button(surface, &toplevel->close_button_rect, 2.0f, (ei_color_t)
        {
            0xff, 0x60, 0x60, 0xff
        }
        ,
        0, ei_relief_raised, &draw_rect
        )
        ;
        ei_point_t p1 = {toplevel->close_button_rect.top_left.x + 3, toplevel->close_button_rect.top_left.y + 3};
        ei_point_t p2 = {
            toplevel->close_button_rect.top_left.x + toplevel->close_button_rect.size.width - 4,
            toplevel->close_button_rect.top_left.y + toplevel->close_button_rect.size.height - 4
        };
        ei_point_t p3 = {
            toplevel->close_button_rect.top_left.x + 3,
            toplevel->close_button_rect.top_left.y + toplevel->close_button_rect.size.height - 4
        };
        ei_point_t p4 = {
            toplevel->close_button_rect.top_left.x + toplevel->close_button_rect.size.width - 4,
            toplevel->close_button_rect.top_left.y + 3
        };
        ei_color_t x_color = {0x00, 0x00, 0x00, 0xff};
        ei_point_t lines_x[] = {p1, p2, p3, p4};
        ei_draw_polyline(surface, &lines_x[0], 2, x_color, &visible_close_btn);
        ei_draw_polyline(surface, &lines_x[2], 2, x_color, &visible_close_btn);
    }

    // Dessiner le contenu (utiliser une variable locale pour ne pas corrompre content_rect)
    ei_rect_t visible_content;
    if (intersection_rect(&visible_content, toplevel->widget.content_rect, &draw_rect))
    {
        ei_fill(surface, &toplevel->color, &visible_content);
    }

    // Dessiner les enfants
    ei_rect_t children_clipper;
    if (intersection_rect(&children_clipper, toplevel->widget.content_rect, &draw_rect))
    {
        ei_impl_widget_draw_children(widget, surface, pick_surface, &children_clipper);
    }

    // Dessiner la poignée de redimensionnement en dernier pour qu'elle reste visible.
    // Repaint aussi la pick surface pour que le resize handle soit cliquable même si
    // un enfant couvre cette zone.
    ei_rect_t visible_resize;
    if (toplevel->resizable != ei_axis_none && intersection_rect(&visible_resize,
                                                                 &toplevel->resize_handle_rect, &draw_rect))
    {
        ei_fill(pick_surface, &toplevel->widget.pick_color, &visible_resize);
        ei_color_t resize_color = (ei_color_t)
        {
            0x60, 0x60, 0xff, 0xff
        };
        ei_fill(surface, &resize_color, &visible_resize);
    }
}


void toplevel_setdefaultsfunc(ei_widget_t widget)
{
    ei_impl_toplevel_t* toplevel = (ei_impl_toplevel_t*)widget;
    toplevel->widget.requested_size = ei_size(320, 240);
    toplevel->color = ei_default_background_color;
    toplevel->border_width = 4;
    toplevel->title = strdup("Toplevel");
    toplevel->closable = true;
    toplevel->resizable = ei_axis_both;
    toplevel->min_size = ei_size(160, 120);
    toplevel->title_bar_height = TOPLEVEL_TITLE_BAR_HEIGHT;
    toplevel->is_moving = false;
    toplevel->is_resizing = false;
}


bool toplevel_handlefunc(ei_widget_t widget, ei_event_t* event)
{
    ei_impl_toplevel_t* toplevel = (ei_impl_toplevel_t*)widget;
    bool event_handled = false;

    switch (event->type)
    {
    case ei_ev_mouse_buttondown:
        if (event->param.mouse.button == ei_mouse_button_left)
        {
            ei_point_t click_pos = event->param.mouse.where;

            // Amener au premier plan
            if (toplevel->widget.parent && toplevel->widget.parent->children_tail != widget)
            {
                // Supposons une fonction ei_widget_bring_to_front (à implémenter)
                // Pour l’instant, invalider la zone
                ei_app_invalidate_rect(&toplevel->widget.screen_location);
            }

            if (toplevel->closable && point_in_rect(click_pos, &toplevel->close_button_rect))
            {
                // Invalider la zone avant de détruire
                ei_app_invalidate_rect(&toplevel->widget.screen_location);
                // Désactiver le widget s'il est actif pour éviter les références pendantes
                if (ei_event_get_active_widget() == widget)
                {
                    ei_event_set_active_widget(NULL);
                }
                // Détruire le widget (qui appellera automatiquement le destructeur utilisateur)
                ei_widget_destroy(widget);
                event_handled = true;
            }
            else if (toplevel->resizable != ei_axis_none && point_in_rect(click_pos, &toplevel->resize_handle_rect))
            {
                toplevel->is_resizing = true;
                toplevel->is_moving = false;
                toplevel->drag_start_pos = click_pos;
                toplevel->widget_start_size = toplevel->widget.requested_size;
                ei_event_set_active_widget(widget);
                event_handled = true;
            }
            else if (point_in_rect(click_pos, &toplevel->title_bar_rect))
            {
                toplevel->is_moving = true;
                toplevel->is_resizing = false;
                toplevel->drag_start_pos = click_pos;
                toplevel->widget_start_pos = toplevel->widget.screen_location.top_left;
                ei_event_set_active_widget(widget);
                event_handled = true;
            }
            else
            {
                // Focus the toplevel so it receives keyboard events.
                ei_event_set_active_widget(widget);
            }
        }
        break;

    case ei_ev_mouse_move:
        if (ei_event_get_active_widget() == widget)
        {
            if (toplevel->is_moving)
            {
                int dx = event->param.mouse.where.x - toplevel->drag_start_pos.x;
                int dy = event->param.mouse.where.y - toplevel->drag_start_pos.y;
                int new_x = toplevel->widget_start_pos.x + dx;
                int new_y = toplevel->widget_start_pos.y + dy;
                ei_rect_t old_loc = toplevel->widget.screen_location;
                ei_app_invalidate_rect(&old_loc);
                ei_place(widget, NULL, &new_x, &new_y, NULL, NULL, NULL, NULL, NULL, NULL);
                event_handled = true;
            }
            else if (toplevel->is_resizing)
            {
                int dw = event->param.mouse.where.x - toplevel->drag_start_pos.x;
                int dh = event->param.mouse.where.y - toplevel->drag_start_pos.y;
                ei_size_t new_size = toplevel->widget_start_size;
                if (toplevel->resizable & ei_axis_x)
                {
                    new_size.width = max(toplevel->min_size.width, new_size.width + dw);
                }
                if (toplevel->resizable & ei_axis_y)
                {
                    new_size.height = max(toplevel->min_size.height, new_size.height + dh);
                }
                ei_rect_t old_loc = toplevel->widget.screen_location;
                ei_app_invalidate_rect(&old_loc);
                ei_toplevel_configure(widget, &new_size, NULL, NULL, NULL, NULL, NULL, NULL);
                event_handled = true;
            }
        }
        break;

    case ei_ev_mouse_buttonup:
        if (ei_event_get_active_widget() == widget && event->param.mouse.button == ei_mouse_button_left)
        {
            bool was_dragging = toplevel->is_moving || toplevel->is_resizing;
            toplevel->is_moving = false;
            toplevel->is_resizing = false;
            if (was_dragging)
            {
                ei_event_set_active_widget(NULL);
            }
            ei_app_invalidate_rect(&toplevel->widget.screen_location);
            event_handled = true;
        }
        break;

    default:
        break;
    }
    return event_handled;
}

static ei_widgetclass_t g_toplevel_class_struct;

void ei_toplevel_register_class(void)
{
    strncpy(g_toplevel_class_struct.name, "toplevel", sizeof(g_toplevel_class_struct.name) - 1);
    g_toplevel_class_struct.name[sizeof(g_toplevel_class_struct.name) - 1] = '\0';
    g_toplevel_class_struct.allocfunc = toplevel_allocfunc;
    g_toplevel_class_struct.releasefunc = toplevel_releasefunc;
    g_toplevel_class_struct.drawfunc = toplevel_drawfunc;
    g_toplevel_class_struct.setdefaultsfunc = toplevel_setdefaultsfunc;
    g_toplevel_class_struct.geomnotifyfunc = toplevel_geomnotifyfunc;
    g_toplevel_class_struct.handlefunc = toplevel_handlefunc;
    g_toplevel_class_struct.next = NULL;
    ei_widgetclass_register(&g_toplevel_class_struct);
}