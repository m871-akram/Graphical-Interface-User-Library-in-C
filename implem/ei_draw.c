#include "ei_draw.h"
#include "hw_interface.h"
#include "ei_implementation.h"
#include "ei_utils.h"
#include <stdint.h>
#include <assert.h>

// Cette fonction remplit une zone avec une couleur (comme si on peignait un mur !)
void ei_fill(ei_surface_t surface, const ei_color_t* couleur, const ei_rect_t* clipper)
{
    // On récupère le buffer (l'endroit où on dessine) et la taille de la surface
    uint8_t* pixel_0 = hw_surface_get_buffer(surface);
    ei_size_t taille_surface = hw_surface_get_size(surface);

    // Convertir la couleur selon l'ordre des canaux de la surface (toujours via map_rgba pour éviter les erreurs de canaux/alpha)
    uint32_t valeur_pixel = ei_impl_map_rgba(surface, *couleur);

    // On définit la zone qu'on va remplir (le clipper, c'est comme une fenêtre où on a le droit de peindre)
    int clip_xmin = 0, clip_ymin = 0, clip_xmax = taille_surface.width - 1, clip_ymax = taille_surface.height - 1;
    if (clipper != NULL)
    {
        clip_xmin = clipper->top_left.x;
        clip_ymin = clipper->top_left.y;
        clip_xmax = clip_xmin + clipper->size.width - 1;
        clip_ymax = clip_ymin + clipper->size.height - 1;
    }

    // Clamp to surface boundaries to prevent buffer overflow
    if (clip_xmin < 0) clip_xmin = 0;
    if (clip_ymin < 0) clip_ymin = 0;
    if (clip_xmax >= taille_surface.width) clip_xmax = taille_surface.width - 1;
    if (clip_ymax >= taille_surface.height) clip_ymax = taille_surface.height - 1;

    // Early exit if clipping region is invalid
    if (clip_xmin > clip_xmax || clip_ymin > clip_ymax)
    {
        return;
    }

    // On remplit juste la zone du clipper, ligne par ligne
    for (int y = clip_ymin; y <= clip_ymax; y++)
    {
        // On trouve le début de la ligne
        uint8_t* ptr_ligne = pixel_0 + (y * taille_surface.width * 4) + (clip_xmin * 4);
        uint32_t* ptr_pixel = (uint32_t*)ptr_ligne;
        // On colore chaque pixel de la ligne
        for (int x = clip_xmin; x <= clip_xmax; x++)
        {
            *ptr_pixel++ = valeur_pixel;
        }
    }
}

// Dessine une polyligne (une série de lignes connectées, comme un chemin)
void ei_draw_polyline(ei_surface_t surface, ei_point_t* points, size_t taille_points,
                      ei_color_t couleur, const ei_rect_t* clipper)
{
    // S’il n’y a pas de points, on fait rien
    if (taille_points == 0) return;

    // Juste un point ? On dessine un point (une ligne qui va nulle part)
    if (taille_points == 1)
    {
        draw_line(surface, points[0], points[0], couleur, clipper);
    }
    else
    {
        // Plusieurs points ? On dessine une ligne entre chaque paire
        for (size_t i = 0; i < taille_points - 1; i++)
        {
            draw_line(surface, points[i], points[i + 1], couleur, clipper);
        }
    }
}

// Dessine un polygone rempli (genre un octogone tout coloré !)
void ei_draw_polygon(ei_surface_t surface, ei_point_t* points, size_t taille_points,
                     ei_color_t couleur, const ei_rect_t* clipper)
{
    // S’il n’y a pas assez de points ou c’est vide, on dégage
    if (points == NULL || taille_points < 3) return;

    // On récupère la taille de l’écran
    ei_size_t taille_surface = hw_surface_get_size(surface);

    // On cherche les y minimum et maximum du polygone
    int y_min = points[0].y;
    int y_max = points[0].y;
    for (size_t i = 1; i < taille_points; i++)
    {
        if (points[i].y < y_min) y_min = points[i].y;
        if (points[i].y > y_max) y_max = points[i].y;
    }

    // On ajuste les limites pour pas dessiner n’importe où
    int clip_ymin = y_min, clip_ymax = y_max;
    if (clipper != NULL)
    {
        // On prend le max/min entre le clipper et le polygone
        clip_ymin = (clip_ymin > clipper->top_left.y) ? clip_ymin : clipper->top_left.y;
        clip_ymax = (clip_ymax < (clipper->top_left.y + clipper->size.height))
                        ? clip_ymax
                        : (clipper->top_left.y + clipper->size.height);
    }
    clip_ymin = (clip_ymin > 0) ? clip_ymin : 0;
    clip_ymax = (clip_ymax < taille_surface.height) ? clip_ymax : (taille_surface.height);

    // Si la zone est vide, on sort
    if (clip_ymin > clip_ymax) return;

    // On crée une table pour stocker les côtés (avec calloc, merci <stdlib.h> !)
    // +1 pour inclure à la fois clip_ymin et clip_ymax, sizeof(edge_t*) car c'est un tableau de pointeurs
    int table_size = clip_ymax - clip_ymin + 1;
    edge_t** table_cotes = calloc(table_size, sizeof(edge_t*));
    if (!table_cotes) return; // Si allocation échoue, on sort

    // On remplit la table avec les arêtes du polygone
    creer_table_tc(points, taille_points, table_cotes, clip_ymin, clip_ymax);

    // On commence avec une liste vide pour les arêtes actives (TCA)
    edge_t* tca = NULL;

    // On boucle sur chaque ligne (scanline) de y_min à y_max
    for (int y = clip_ymin; y <= clip_ymax; y++)
    {
        // On vire les arêtes qui se terminent à cette ligne
        supprimer_arete_tca(y, &tca);

        // On ajoute les nouvelles arêtes qui commencent à cette ligne
        int table_index = y - clip_ymin;
        if (table_index >= 0 && table_index < table_size)
        {
            edge_t* cote = table_cotes[table_index];
            while (cote != NULL)
            {
                edge_t* next = cote->next;
                ajoute_arete_tca(cote, &tca); // On met l'arête dans la TCA
                cote = next;
            }
            table_cotes[table_index] = NULL; // On vide cette case
        }

        // On trie la TCA pour avoir les arêtes dans l’ordre des x
        trier_tca_x(&tca);

        // On remplit les bouts entre les arêtes (ça fait le polygone !)
        edge_t* courant = tca;
        while (courant != NULL && courant->next != NULL)
        {
            // On trouve les x de début et de fin pour cette ligne
            int x_debut = (int)ceilf(courant->x);
            int x_fin = (int)floorf(courant->next->x);

            // S’il y a quelque chose à dessiner, on trace une ligne horizontale
            if (x_debut <= x_fin)
            {
                draw_horizontal_line(surface, x_debut, x_fin, y, couleur, clipper);
            }
            courant = courant->next->next; // On passe au prochain couple
        }

        // On met à jour les x pour la ligne suivante
        courant = tca;
        while (courant != NULL)
        {
            courant->x += courant->inv_m;
            courant = courant->next;
        }
    }

    // On nettoie la TCA (on libère la mémoire avec free, merci <stdlib.h> !)
    while (tca != NULL)
    {
        edge_t* u = tca;
        tca = tca->next;
        free(u);
    }
    // On nettoie aussi la table des côtés
    for (int i = 0; i < table_size; i++)
    {
        edge_t* cote = table_cotes[i];
        while (cote != NULL)
        {
            edge_t* u = cote;
            cote = cote->next;
            free(u);
        }
    }
    // On libère la table elle-même
    free(table_cotes);
}


void ei_draw_text(ei_surface_t surface, const ei_point_t* where, ei_const_string_t text, ei_font_t font,
                  ei_color_t color, const ei_rect_t* clipper)
{
    assert(surface != NULL && "Surface cannot be NULL");
    assert(where != NULL && "Position cannot be NULL");
    assert(text != NULL && "Text cannot be NULL");
    assert(color.red != 255 || color.green != 255 || color.blue != 255 || "Color cannot be NULL");


    ei_font_t font_used = font ? font : ei_default_font;
    if (font_used == NULL)
    {
        return; // No valid font
    }

    ei_color_t text_color = {color.red, color.green, color.blue, 255};

    ei_surface_t text_surface = hw_text_create_surface(text, font_used, text_color);
    if (text_surface == NULL)
    {
        return; // Failed to create text surface
    }

    ei_size_t text_size = hw_surface_get_size(text_surface);
    if (text_size.width <= 0 || text_size.height <= 0)
    {
        hw_surface_free(text_surface);
        return; // Empty text surface
    }

    ei_rect_t dst_rect = {*where, text_size};

    ei_rect_t clipped_dst_rect = dst_rect;
    if (clipper != NULL)
    {
        if (!intersection_rect(&clipped_dst_rect, &dst_rect, clipper))
        {
            hw_surface_free(text_surface);
            return;
        }
    }


    ei_rect_t src_rect = {{0, 0}, clipped_dst_rect.size};
    src_rect.top_left.x = clipped_dst_rect.top_left.x - dst_rect.top_left.x;
    src_rect.top_left.y = clipped_dst_rect.top_left.y - dst_rect.top_left.y;

    ei_copy_surface(surface, &clipped_dst_rect, text_surface, &src_rect, true);


    hw_surface_free(text_surface);
}


int ei_copy_surface(ei_surface_t destination,
                    const ei_rect_t* dst_rect,
                    ei_surface_t source,
                    const ei_rect_t* src_rect,
                    bool alpha)
{
    // Validate inputs
    if (destination == NULL || source == NULL)
    {
        return 1;
    }

    // Get surface sizes
    ei_size_t dst_size = hw_surface_get_size(destination);
    ei_size_t src_size = hw_surface_get_size(source);

    // Default to entire surfaces if rectangles are NULL
    ei_rect_t dst_rect_real = dst_rect ? *dst_rect : (ei_rect_t)
    {
        {
            0, 0
        }
        ,
        dst_size
    };
    ei_rect_t src_rect_real = src_rect ? *src_rect : (ei_rect_t)
    {
        {
            0, 0
        }
        ,
        src_size
    };

    // Check rectangle sizes match
    if (dst_rect_real.size.width != src_rect_real.size.width ||
        dst_rect_real.size.height != src_rect_real.size.height)
    {
        return 1;
    }

    // Check for empty or invalid rectangles
    if (dst_rect_real.size.width <= 0 || dst_rect_real.size.height <= 0)
    {
        return 0;
    }

    // Validate rectangle bounds
    if (dst_rect_real.top_left.x < 0 || dst_rect_real.top_left.y < 0 ||
        dst_rect_real.top_left.x + dst_rect_real.size.width > dst_size.width ||
        dst_rect_real.top_left.y + dst_rect_real.size.height > dst_size.height ||
        src_rect_real.top_left.x < 0 || src_rect_real.top_left.y < 0 ||
        src_rect_real.top_left.x + src_rect_real.size.width > src_size.width ||
        src_rect_real.top_left.y + src_rect_real.size.height > src_size.height)
    {
        return 1;
    }

    // Lock source surface to safely access its buffer.
    hw_surface_lock(source);

    // Get pixel buffers and channel indices
    uint8_t* dst_buffer = hw_surface_get_buffer(destination);
    uint8_t* src_buffer = hw_surface_get_buffer(source);
    int dst_ir, dst_ig, dst_ib, dst_ia;
    int src_ir, src_ig, src_ib, src_ia;
    hw_surface_get_channel_indices(destination, &dst_ir, &dst_ig, &dst_ib, &dst_ia);
    hw_surface_get_channel_indices(source, &src_ir, &src_ig, &src_ib, &src_ia);

    // Assume 32-bit RGBA (4 bytes per pixel)
    const int bytes_per_pixel = 4;

    if (!alpha)
    {
        // Direct copy with memcpy for each row
        for (int y = 0; y < dst_rect_real.size.height; y++)
        {
            uint8_t* dst_row = dst_buffer + ((dst_rect_real.top_left.y + y) * dst_size.width +
                dst_rect_real.top_left.x) * bytes_per_pixel;
            uint8_t* src_row = src_buffer + ((src_rect_real.top_left.y + y) * src_size.width +
                src_rect_real.top_left.x) * bytes_per_pixel;
            memcpy(dst_row, src_row, dst_rect_real.size.width * bytes_per_pixel);
        }
    }
    else
    {
        // Alpha blending
        for (int y = 0; y < dst_rect_real.size.height; y++)
        {
            uint8_t* dst_row = dst_buffer + ((dst_rect_real.top_left.y + y) * dst_size.width +
                dst_rect_real.top_left.x) * bytes_per_pixel;
            uint8_t* src_row = src_buffer + ((src_rect_real.top_left.y + y) * src_size.width +
                src_rect_real.top_left.x) * bytes_per_pixel;
            for (int x = 0; x < dst_rect_real.size.width; x++)
            {
                uint8_t* dst_pixel = dst_row + x * bytes_per_pixel;
                uint8_t* src_pixel = src_row + x * bytes_per_pixel;

                // Get source alpha (default to 255 if no alpha channel)
                float alpha_norm = (src_ia >= 0) ? (src_pixel[src_ia] / 255.0f) : 1.0f;

                // Blend RGB channels
                dst_pixel[dst_ir] = (uint8_t)(alpha_norm * src_pixel[src_ir] + (1.0f - alpha_norm) * dst_pixel[dst_ir]);
                dst_pixel[dst_ig] = (uint8_t)(alpha_norm * src_pixel[src_ig] + (1.0f - alpha_norm) * dst_pixel[dst_ig]);
                dst_pixel[dst_ib] = (uint8_t)(alpha_norm * src_pixel[src_ib] + (1.0f - alpha_norm) * dst_pixel[dst_ib]);

                // Set destination alpha to opaque if channel exists
                if (dst_ia >= 0)
                {
                    dst_pixel[dst_ia] = 255;
                }
            }
        }
    }

    // Unlock the source surface now that we are done reading from it.
    hw_surface_unlock(source);

    return 0;
}