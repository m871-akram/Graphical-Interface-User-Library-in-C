#include <stdlib.h>
#include <math.h>
#include "hw_interface.h"
#include "ei_draw.h"
#include "ei_types.h"
#include "ei_relief.h"
#include <assert.h>


ei_point_t* arc(ei_point_t centre, float rayon, float angle_deb, float angle_fin, size_t* count)
{
    if (rayon < 0)
    {
        return NULL; // Pas possible de dessiner avec un rayon nul ou négatif, désolé !
    }

    float arc_angle = fabs(angle_fin - angle_deb); // On calcule l'angle de l'arc, easy
    int n_segments = (int)ceilf(rayon * arc_angle); // On découpe l'arc en petits morceaux

    ei_point_t* tab_points = malloc((n_segments + 1) * sizeof(ei_point_t));

    int uniq_x = -1, uniq_y = -1; // On garde un œil sur le dernier point pour pas le répéter
    int c = 0;

    for (int i = 0; i <= n_segments; i++)
    {
        float angle = (n_segments == 0) ? angle_deb
                                        : angle_deb + (angle_fin - angle_deb) * i / (float)n_segments;

        int px = roundf(centre.x + rayon * cosf(angle));
        int py = roundf(centre.y + rayon * sinf(angle));

        if (i == 0 || px != uniq_x || py != uniq_y)
        {
            // On vérifie qu'on a pas déjà ce point
            tab_points[c].x = px;
            tab_points[c].y = py;
            c++;
            uniq_x = px;
            uniq_y = py;
        }
    }

    *count = c; // On dit combien de points on a gardés

    return realloc(tab_points, (c + 1) * sizeof(ei_point_t)); // On ajuste la taille du tableau, nickel
}

ei_point_t* rounded_frame(ei_rect_t* rect, float rayon, size_t* count, ei_frame_part_t part)
{
    if (!rect || rayon < 0 || rayon > (float)rect->size.width / 2 || rayon > (float)rect->size.height / 2)
    {
        *count = 0;
        return NULL;
    }

    ei_point_t centres[4]; // Les centres des 4 coins arrondis
    float angles_rad[4][2]; // Les angles pour chaque arc
    int n_arcs = 0; // Combien d'arcs on va dessiner

    // On place les centres des arcs pour chaque coin
    centres[0] = (ei_point_t)
    {
        rect->top_left.x + rayon, rect->top_left.y + rayon
    }; // Coin haut-gauche
    centres[1] = (ei_point_t)
    {
        rect->top_left.x + rayon, rect->top_left.y + rect->size.height - rayon
    }; // Coin bas-gauche
    centres[2] = (ei_point_t)
    {
        rect->top_left.x + rect->size.width - rayon, rect->top_left.y + rect->size.height - rayon
    }; // Coin bas-droit
    centres[3] = (ei_point_t)
    {
        rect->top_left.x + rect->size.width - rayon, rect->top_left.y + rayon
    }; // Coin haut-droit

    ei_point_t* segment_coin[4]; // Tableau pour stocker les points des arcs
    size_t n_seg_coins[4]; // Combien de points par arc
    size_t total_seg_coins = 0; // Total des points pour tous les arcs

    if (part == FRAME_FULL)
    {
        // On dessine tout le cadre
        n_arcs = 4; // 4 coins, donc 4 arcs
        angles_rad[0][0] = 3 * M_PI / 2;
        angles_rad[0][1] = M_PI; // Coin haut-gauche
        angles_rad[1][0] = M_PI;
        angles_rad[1][1] = M_PI / 2; // Coin bas-gauche
        angles_rad[2][0] = M_PI / 2;
        angles_rad[2][1] = 0; // Coin bas-droit
        angles_rad[3][0] = 0;
        angles_rad[3][1] = -M_PI / 2; // Coin haut-droit

        // On calcule les points pour chaque coin
        segment_coin[0] = arc(centres[0], rayon, angles_rad[0][0], angles_rad[0][1], &n_seg_coins[0]);
        segment_coin[1] = arc(centres[1], rayon, angles_rad[1][0], angles_rad[1][1], &n_seg_coins[1]);
        segment_coin[2] = arc(centres[2], rayon, angles_rad[2][0], angles_rad[2][1], &n_seg_coins[2]);
        segment_coin[3] = arc(centres[3], rayon, angles_rad[3][0], angles_rad[3][1], &n_seg_coins[3]);
        total_seg_coins = n_seg_coins[0] + n_seg_coins[1] + n_seg_coins[2] + n_seg_coins[3]; // Total des points
    }
    else if (part == FRAME_TOP)
    {
        // Juste le haut du cadre
        n_arcs = 3; // 3 arcs pour le haut
        angles_rad[0][0] = -M_PI / 4;
        angles_rad[0][1] = -M_PI / 2; // Coin haut-droit partiel
        angles_rad[1][0] = 3 * M_PI / 2;
        angles_rad[1][1] = M_PI; // Coin haut-gauche complet
        angles_rad[2][0] = M_PI;
        angles_rad[2][1] = 3 * M_PI / 4; // Coin bas-gauche partiel

        segment_coin[0] = arc(centres[3], rayon, angles_rad[0][0], angles_rad[0][1], &n_seg_coins[0]);
        segment_coin[1] = arc(centres[0], rayon, angles_rad[1][0], angles_rad[1][1], &n_seg_coins[1]);
        segment_coin[2] = arc(centres[1], rayon, angles_rad[2][0], angles_rad[2][1], &n_seg_coins[2]);
        total_seg_coins = n_seg_coins[0] + n_seg_coins[1] + n_seg_coins[2] + 2; // On ajoute 2 points pour les lignes
    }
    else if (part == FRAME_BOTTOM)
    {
        // Juste le bas du cadre
        n_arcs = 3; // 3 arcs pour le bas
        angles_rad[0][0] = 3 * M_PI / 4;
        angles_rad[0][1] = M_PI / 2; // Coin bas-gauche partiel
        angles_rad[1][0] = M_PI / 2;
        angles_rad[1][1] = 0; // Coin bas-droit complet
        angles_rad[2][0] = 0;
        angles_rad[2][1] = -M_PI / 4; // Coin haut-droit partiel

        segment_coin[0] = arc(centres[1], rayon, angles_rad[0][0], angles_rad[0][1], &n_seg_coins[0]);
        segment_coin[1] = arc(centres[2], rayon, angles_rad[1][0], angles_rad[1][1], &n_seg_coins[1]);
        segment_coin[2] = arc(centres[3], rayon, angles_rad[2][0], angles_rad[2][1], &n_seg_coins[2]);
        total_seg_coins = n_seg_coins[0] + n_seg_coins[1] + n_seg_coins[2] + 2; // Encore 2 points pour les lignes
    }

    ei_point_t* frame = malloc(total_seg_coins * sizeof(ei_point_t)); // On réserve la place pour le cadre

    size_t offset = 0;
    for (int i = 0; i < n_arcs; i++)
    {
        if (segment_coin[i] && n_seg_coins[i] > 0)
        {
            for (int j = 0; j < n_seg_coins[i]; j++)
            {
                frame[offset + j] = segment_coin[i][j]; // On copie les points
            }
            offset += n_seg_coins[i]; // On avance dans le tableau
        }
        free(segment_coin[i]); // On libère la mémoire, faut pas oublier !
    }

    *count = offset;

    return realloc(frame, offset * sizeof(ei_point_t)); // On ajuste la taille du tableau, propre !
}


void draw_button(ei_surface_t surface,
                 ei_rect_t* rect, // Rectangle extérieur total du bouton, sur lequel le relief est appliqué
                 float rayon,
                 ei_color_t color_centre, // Couleur de base du centre du bouton
                 int epaisseur_relief, // ANCIENNEMENT border_width, représente l'épaisseur du biseau du relief
                 ei_relief_t relief,
                 const ei_rect_t* clipper_externe)
{
    assert(surface != NULL && "draw_button: surface cannot be NULL");
    assert(rect != NULL && "draw_button: rect cannot be NULL");
    assert(rect->size.width > 0 && rect->size.height > 0 && "draw_button: rect size must be positive");
    assert(rayon >= 0.0f && "draw_button: rayon cannot be negative");
    assert(epaisseur_relief >= 0 && "draw_button: epaisseur_relief cannot be negative");

    // S'assurer que le rayon n'est pas trop grand pour le rectangle extérieur
    float rayon_exterieur = rayon;
    if (rayon_exterieur > (float)rect->size.width / 2.0f) rayon_exterieur = (float)rect->size.width / 2.0f;
    if (rayon_exterieur > (float)rect->size.height / 2.0f) rayon_exterieur = (float)rect->size.height / 2.0f;

    // ETAPE 1: Calculer les couleurs de relief
    const int delta_relief = 40;
    ei_color_t couleur_claire = color_centre;
    ei_color_t couleur_sombre = color_centre;

    // On garde l'alpha de la couleur de base pour les couleurs de relief
    couleur_claire.alpha = color_centre.alpha;
    couleur_sombre.alpha = color_centre.alpha;

    couleur_claire.red = (uint8_t)(color_centre.red + delta_relief > 255 ? 255 : color_centre.red + delta_relief);
    couleur_claire.green = (uint8_t)(color_centre.green + delta_relief > 255 ? 255 : color_centre.green + delta_relief);
    couleur_claire.blue = (uint8_t)(color_centre.blue + delta_relief > 255 ? 255 : color_centre.blue + delta_relief);

    couleur_sombre.red = (uint8_t)(color_centre.red - delta_relief < 0 ? 0 : color_centre.red - delta_relief);
    couleur_sombre.green = (uint8_t)(color_centre.green - delta_relief < 0 ? 0 : color_centre.green - delta_relief);
    couleur_sombre.blue = (uint8_t)(color_centre.blue - delta_relief < 0 ? 0 : color_centre.blue - delta_relief);

    ei_color_t couleur_relief_haut = (relief == ei_relief_raised) ? couleur_claire : couleur_sombre;
    ei_color_t couleur_relief_bas = (relief == ei_relief_raised) ? couleur_sombre : couleur_claire;

    // ETAPE 2: Dessiner les parties de relief (sur le rect extérieur)
    if (relief != ei_relief_none)
    {
        if (epaisseur_relief > 0)
        {
            // On ne dessine le relief que si l'épaisseur est positive
            size_t count_haut;
            ei_point_t* points_haut = rounded_frame(rect, rayon_exterieur, &count_haut, FRAME_TOP);
            if (points_haut && count_haut > 0)
            {
                ei_draw_polygon(surface, points_haut, count_haut, couleur_relief_haut, clipper_externe);
                free(points_haut);
            }

            size_t count_bas;
            ei_point_t* points_bas = rounded_frame(rect, rayon_exterieur, &count_bas, FRAME_BOTTOM);
            if (points_bas && count_bas > 0)
            {
                ei_draw_polygon(surface, points_bas, count_bas, couleur_relief_bas, clipper_externe);
                free(points_bas);
            }
        }
        else
        {
            // Si epaisseur_relief est 0 mais relief demandé, on dessine plat avec la couleur de base
            size_t count_plat;
            ei_point_t* points_plat = rounded_frame(rect, rayon_exterieur, &count_plat, FRAME_FULL);
            if (points_plat && count_plat > 0)
            {
                ei_draw_polygon(surface, points_plat, count_plat, color_centre, clipper_externe);
                free(points_plat);
            }
            return; // Fin du dessin si relief demandé mais épaisseur nulle, on a dessiné plat.
        }
    }

    // ETAPE 3: Définir le rectangle pour le CENTRE du bouton (intérieur au relief)
    ei_rect_t centre_rect = *rect;
    if (relief != ei_relief_none && epaisseur_relief > 0)
    {
        // On ne rétrécit que si du relief avec épaisseur a été dessiné
        centre_rect.top_left.x += epaisseur_relief;
        centre_rect.top_left.y += epaisseur_relief;
        centre_rect.size.width -= 2 * epaisseur_relief;
        centre_rect.size.height -= 2 * epaisseur_relief;
    }
    else if (relief == ei_relief_none)
    {
        // Si pas de relief, le centre occupe tout le 'rect' (pas de bordure séparée dans ce modèle)
        // centre_rect est déjà *rect
    }
    // Si relief demandé mais epaisseur_relief == 0, on a déjà dessiné plat et quitté.


    // S'assurer que centre_rect est valide après rétrécissement
    if (centre_rect.size.width <= 0 || centre_rect.size.height <= 0)
    {
        return; // Plus de place pour dessiner le centre
    }

    float rayon_pour_centre = rayon_exterieur - epaisseur_relief;
    if (rayon_pour_centre < 0.0f) rayon_pour_centre = 0.0f;
    // Ajuster rayon_pour_centre pour ne pas dépasser les dimensions de centre_rect
    if (rayon_pour_centre > (float)centre_rect.size.width / 2.0f)
    {
        rayon_pour_centre = (float)centre_rect.size.width / 2.0f;
    }
    if (rayon_pour_centre > (float)centre_rect.size.height / 2.0f)
    {
        rayon_pour_centre = (float)centre_rect.size.height / 2.0f;
    }

    // ETAPE 4: Dessiner le CENTRE du bouton
    size_t count_centre;
    ei_point_t* points_centre = rounded_frame(&centre_rect, rayon_pour_centre, &count_centre, FRAME_FULL);
    if (points_centre && count_centre > 0)
    {
        ei_draw_polygon(surface, points_centre, count_centre, color_centre, clipper_externe);
        free(points_centre);
    }
}