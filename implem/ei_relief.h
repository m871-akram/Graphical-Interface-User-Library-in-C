/**
 * @file  ei_relief.h
 *
 * @brief Définitions privées, genre le backstage du code !
 *
 */

#ifndef EI_RELIEF_H
#define EI_RELIEF_H

#include "hw_interface.h"
#include "ei_types.h"


/**
 * \brief donne les points dans un tableau pour tracer un arc.
 *
 * @param rayon : le rayon de l'arc, faut qu'il soit positif !
 * @param centre: le centre de l'arc, là où tout commence.
 * @param angle_deb : l'angle de départ, où on commence à dessiner.
 * @param angle_fin : l'angle de fin, où on s'arrête.
 * @param count : combien de points on a dans le tableau final.
 */
ei_point_t* arc(ei_point_t centre, float rayon, float angle_deb, float angle_fin, size_t* count);

// Une petite énum pour choisir quelle partie du cadre on veut
/**
 * \brief une enumeration pour savoir quel partie de frame prendre
 */
typedef enum
{
    FRAME_FULL, // Tout le cadre, on prend tout !
    FRAME_TOP, // Juste le haut, pour un style partiel
    FRAME_BOTTOM, // Juste le bas, pareil mais en bas
} ei_frame_part_t;

/**
 * Génère un tableau de points représentant un cadre arrondi ou une partie de celui-ci.
 * @param rect Rectangle définissant le cadre, la base quoi.
 * @param rayon Rayon des coins arrondis (faut pas qu'il soit trop grand sinon ça marche pas).
 * @param count Pointeur pour stocker le nombre de points générés, histoire de savoir combien on en a.
 * @param part Partie du cadre à générer (FRAME_FULL, FRAME_TOP, FRAME_BOTTOM), pour choisir le style.
 * @return Pointeur vers le tableau de points, ou NULL si y'a un souci.
 */
ei_point_t* rounded_frame(ei_rect_t* rect, float rayon, size_t* count, ei_frame_part_t part);

// Une petite structure pour organiser le dessin
typedef struct
{
    ei_frame_part_t part; // Quelle partie on dessine
    ei_color_t color; // La couleur, pour que ça pète
    ei_rect_t* clipper; // Le clipper, pour pas déborder
} dessin_frame;

/**
 * @brief   Draws a button with rounded corners, optional border, and 3D relief effect.
 *
 * @param   surface     Destination surface (must be locked).
 * @param   rect        Button rectangle.
 * @param   rayon       Corner radius.
 * @param   color       Base color of the button.
 * @param   border_width Border width in pixels (0 for no border).
 * @param   relief      Relief effect (raised, sunken, or none).
 * @param   clipper     If not NULL, restricts drawing to this rectangle.
 */
void draw_button(ei_surface_t surface,
                 ei_rect_t* rect,
                 float rayon,
                 ei_color_t color,
                 int border_width,
                 ei_relief_t relief,
                 const ei_rect_t* clipper);

#endif