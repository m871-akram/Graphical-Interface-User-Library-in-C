#include "ei_implementation.h"
#include "hw_interface.h"
#include <stdlib.h>
#include "ei_widget_attributes.h"
#include "assert.h"

// Dessine une ligne entre deux points avec l'algo de Bresenham (ça fait des lignes bien droites !)
void draw_line(ei_surface_t surface, ei_point_t point_1, ei_point_t point_2, ei_color_t couleur,
               const ei_rect_t* clipper)
{
    // On récupère le buffer (l'endroit où on dessine) et la taille de la surface
    uint8_t* pixel_0 = hw_surface_get_buffer(surface);
    ei_size_t taille_surface = hw_surface_get_size(surface);

    // On convertit la couleur en un format que la surface comprend (ça dépend si t'es sur Mac, Windows ou Linux)
#if defined(__APPLE__) || defined(_WIN32)
    uint32_t valeur_pixel = ei_impl_map_rgba(surface, couleur);
#else
    uint32_t valeur_pixel = *((uint32_t*)&couleur);
#endif

    // On définit une zone où on a le droit de dessiner (le "clipper")
    int clip_xmin = 0, clip_ymin = 0, clip_xmax = taille_surface.width - 1, clip_ymax = taille_surface.height - 1;
    if (clipper != NULL)
    {
        clip_xmin = clipper->top_left.x;
        clip_ymin = clipper->top_left.y;
        clip_xmax = clip_xmin + clipper->size.width - 1;
        clip_ymax = clip_ymin + clipper->size.height - 1;
    }

    // Clamp clipper to surface boundaries
    if (clip_xmin < 0) clip_xmin = 0;
    if (clip_ymin < 0) clip_ymin = 0;
    if (clip_xmax >= taille_surface.width) clip_xmax = taille_surface.width - 1;
    if (clip_ymax >= taille_surface.height) clip_ymax = taille_surface.height - 1;

    // On récupère les coordonnées des deux points
    int x1 = point_1.x, y1 = point_1.y;
    int x2 = point_2.x, y2 = point_2.y;

    // On calcule les différences entre les points (pour savoir de combien on bouge)
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x2 >= x1) ? 1 : -1; // Direction : +1 si on va à droite, -1 à gauche
    int sy = (y2 >= y1) ? 1 : -1; // Direction : +1 si on va en bas, -1 en haut

    // On vérifie si la ligne est plus large que haute
    bool dirige_par_x = dx >= dy;

    // On commence au premier point
    int x = x1, y = y1;

    // On initialise une erreur pour l'algo de Bresenham (ça aide à choisir les bons pixels)
    int E = dirige_par_x ? dy : dx;

    // Boucle pour dessiner la ligne pixel par pixel
    while (true)
    {
        // On dessine seulement si le pixel est dans la zone autorisée
        if (x >= 0 && x < taille_surface.width && y >= 0 && y < taille_surface.height &&
            x >= clip_xmin && x <= clip_xmax && y >= clip_ymin && y <= clip_ymax)
        {
            // Calculer la position du pixel de manière sûre
            uint32_t* pixel_ptr = (uint32_t*)(pixel_0 + (y * taille_surface.width + x) * 4);
            *pixel_ptr = valeur_pixel;
        }

        // Si on est arrivé au point final, on arrête
        if (x == x2 && y == y2)
        {
            break;
        }

        // On bouge le pixel suivant selon l'axe dominant
        if (dirige_par_x)
        {
            // Si la ligne est plus horizontale
            x += sx;
            E += dy;
            if (2 * E > dx)
            {
                y += sy;
                E -= dx;
            }
        }
        else
        {
            // Si la ligne est plus verticale
            y += sy;
            E += dx;
            if (2 * E > dy)
            {
                x += sx;
                E -= dy;
            }
        }
    }
}

// Dessine une ligne droite horizontale (super simple !)
void draw_horizontal_line(ei_surface_t surface, int x1, int x2, int y, ei_color_t couleur, const ei_rect_t* clipper)
{
    // Si x1 est plus grand que x2, on les échange pour dessiner dans le bon sens
    if (x1 > x2)
    {
        int a = x1;
        x1 = x2;
        x2 = a;
    }

    // On récupère le buffer et la taille de la surface
    uint8_t* pixel_0 = hw_surface_get_buffer(surface);
    ei_size_t taille_surface = hw_surface_get_size(surface);

    // On convertit la couleur (pareil, ça dépend de la plateforme)
#if defined(__APPLE__) || defined(_WIN32)
    uint32_t valeur_pixel = ei_impl_map_rgba(surface, couleur);
#else
    uint32_t valeur_pixel = *((uint32_t*)&couleur);
#endif

    // On définit la zone où on peut dessiner
    int clip_xmin = 0, clip_xmax = taille_surface.width - 1, clip_ymin = 0, clip_ymax = taille_surface.height - 1;
    if (clipper)
    {
        clip_xmin = clipper->top_left.x;
        clip_xmax = clipper->top_left.x + clipper->size.width - 1;
        clip_ymin = clipper->top_left.y;
        clip_ymax = clipper->top_left.y + clipper->size.height - 1;
    }

    // Clamp clipper to surface boundaries
    if (clip_xmin < 0) clip_xmin = 0;
    if (clip_ymin < 0) clip_ymin = 0;
    if (clip_xmax >= taille_surface.width) clip_xmax = taille_surface.width - 1;
    if (clip_ymax >= taille_surface.height) clip_ymax = taille_surface.height - 1;

    // Si y est hors de la zone, on ne dessine rien
    if (y < clip_ymin || y > clip_ymax || y < 0 || y >= taille_surface.height) return;

    // On ajuste x1 et x2 pour qu'ils restent dans les limites
    x1 = (x1 > clip_xmin) ? x1 : clip_xmin;
    x1 = (x1 < 0) ? 0 : x1;
    x2 = (x2 < clip_xmax) ? x2 : clip_xmax;
    x2 = (x2 >= taille_surface.width) ? taille_surface.width - 1 : x2;

    // Si x1 > x2 après ajustement, rien à dessiner
    if (x1 > x2) return;

    // On trouve le premier pixel à dessiner
    uint32_t* ptr_pixel = (uint32_t*)(pixel_0 + (y * taille_surface.width + x1) * 4);
    // On boucle pour colorier chaque pixel
    for (int x = x1; x <= x2; x++)
    {
        *ptr_pixel++ = valeur_pixel;
    }
}

// Calcule l'inverse de la pente entre deux points (pour savoir comment une ligne bouge)
float calcule_inverse_pente(ei_point_t p1, ei_point_t p2)
{
    // Si les y sont égaux, pas de pente, on renvoie 0
    return ((p2.y - p1.y) != 0) ? (float)(p2.x - p1.x) / (p2.y - p1.y) : 0;
}

// Crée une table avec les arêtes d’un polygone (pour dessiner des formes comme des octogones)
void creer_table_tc(const ei_point_t* point_array, size_t point_array_size, edge_t* edge_table[], int y_min, int y_max)
{
    // On boucle sur tous les points du polygone
    for (size_t i = 0; i < point_array_size; i++)
    {
        ei_point_t p1 = point_array[i];
        ei_point_t p2 = point_array[(i + 1) % point_array_size];

        // Si les points sont à la même hauteur, on skip (pas d’arête utile)
        if (p1.y == p2.y) continue;

        // On met le point le plus bas en p_min et l’autre en p_max
        ei_point_t p_min = (p1.y < p2.y) ? p1 : p2;
        ei_point_t p_max = (p1.y < p2.y) ? p2 : p1;

        // Si l’arête est hors de la zone qu’on veut, on skip
        if (p_max.y <= y_min || p_min.y >= y_max) continue;

        // On crée une nouvelle arête
        edge_t* edge = (edge_t*)malloc(sizeof(edge_t));
        if (!edge) continue; // Skip this edge on allocation failure

        // On remplit les infos de l’arête
        edge->ymax = p_max.y;
        edge->inv_m = calcule_inverse_pente(p_min, p_max);

        // Si le point est trop bas, on ajuste x pour commencer à y_min
        if (p_min.y < y_min)
        {
            edge->x = p_min.x + edge->inv_m * (y_min - p_min.y);
            p_min.y = y_min;
        }
        else
        {
            edge->x = p_min.x;
        }

        // On calcule où mettre l’arête dans la table
        int index = p_min.y - y_min;
        if (index >= 0 && index <= (y_max - y_min))
        {
            edge->next = edge_table[index];
            edge_table[index] = edge; // On ajoute l’arête à la table
        }
        else
        {
            free(edge); // Si c’est hors limite, on libère
        }
    }
}

// Ajoute une arête à la liste des arêtes actives (TCA, c’est comme une liste de travail)
void ajoute_arete_tca(edge_t* arete, edge_t** tca)
{
    // On met l’arête au début de la liste
    arete->next = *tca;
    *tca = arete;
}

// Supprime les arêtes qui sont finies dans la liste TCA
void supprimer_arete_tca(int y, edge_t** tca)
{
    edge_t* current = *tca;
    edge_t* prev = NULL;

    // On parcourt la liste
    while (current != NULL)
    {
        // Si l’arête se termine à cette ligne (y), on la vire
        if (current->ymax == y)
        {
            edge_t* u = current;
            if (prev == NULL)
            {
                // Si c’est le début de la liste
                *tca = current->next;
                current = *tca;
            }
            else
            {
                prev->next = current->next;
                current = prev->next;
            }
            free(u); // On libère la mémoire
        }
        else
        {
            prev = current;
            current = current->next;
        }
    }
}

// Trie la liste TCA pour mettre les arêtes dans l’ordre des x (de gauche à droite)
void trier_tca_x(edge_t** tca)
{
    // Si la liste est vide ou a un seul élément, rien à faire
    if (*tca == NULL || (*tca)->next == NULL) return;

    edge_t* sorted = NULL;
    edge_t* current = *tca;

    // On parcourt chaque arête
    while (current != NULL)
    {
        edge_t* next = current->next;

        // On insère l’arête dans la liste triée
        if (sorted == NULL || current->x <= sorted->x)
        {
            current->next = sorted;
            sorted = current;
        }
        else
        {
            edge_t* u = sorted;
            while (u->next != NULL && u->next->x < current->x)
            {
                u = u->next;
            }
            current->next = u->next;
            u->next = current;
        }

        current = next;
    }

    // On met la liste triée à la place de l’ancienne
    *tca = sorted;
}

// Convertit une couleur en un format que la surface peut utiliser
uint32_t ei_impl_map_rgba(ei_surface_t surface, ei_color_t color)
{
    // On récupère l’ordre des couleurs (rouge, vert, bleu, alpha)
    int ir, ig, ib, ia;
    hw_surface_get_channel_indices(surface, &ir, &ig, &ib, &ia);
    uint32_t pixel = 0;
    // On met chaque composante de couleur au bon endroit
    ((uint8_t*)&pixel)[ir] = color.red;
    ((uint8_t*)&pixel)[ig] = color.green;
    ((uint8_t*)&pixel)[ib] = color.blue;
    if (ia >= 0)
    {
        ((uint8_t*)&pixel)[ia] = color.alpha;
    }
    return pixel;
}


bool intersection_rect(ei_rect_t* dest, const ei_rect_t* a, const ei_rect_t* b)
{
    assert(dest != NULL && a != NULL && b != NULL);


    int x1 = a->top_left.x > b->top_left.x ? a->top_left.x : b->top_left.x;
    int y1 = a->top_left.y > b->top_left.y ? a->top_left.y : b->top_left.y;
    int x2 = (a->top_left.x + a->size.width) < (b->top_left.x + b->size.width)
                 ? (a->top_left.x + a->size.width)
                 : (b->top_left.x + b->size.width);
    int y2 = (a->top_left.y + a->size.height) < (b->top_left.y + b->size.height)
                 ? (a->top_left.y + a->size.height)
                 : (b->top_left.y + b->size.height);


    if (x2 > x1 && y2 > y1)
    {
        dest->top_left.x = x1;
        dest->top_left.y = y1;
        dest->size.width = x2 - x1;
        dest->size.height = y2 - y1;
        return true;
    }


    *dest = (ei_rect_t)
    {
        {
            0, 0
        }
        ,
        {
            0, 0
        }
    };
    return false;
}

void clamp_rect_to_surface(ei_rect_t* rect, ei_surface_t surface)
{
    assert(rect != NULL && surface != NULL);

    ei_size_t surface_size = hw_surface_get_size(surface);


    if (rect->top_left.x < 0)
    {
        rect->size.width += rect->top_left.x;
        rect->top_left.x = 0;
    }
    if (rect->top_left.y < 0)
    {
        rect->size.height += rect->top_left.y;
        rect->top_left.y = 0;
    }


    if (rect->top_left.x + rect->size.width > surface_size.width)
    {
        rect->size.width = surface_size.width - rect->top_left.x;
    }
    if (rect->top_left.y + rect->size.height > surface_size.height)
    {
        rect->size.height = surface_size.height - rect->top_left.y;
    }


    if (rect->size.width < 0) rect->size.width = 0;
    if (rect->size.height < 0) rect->size.height = 0;
}


void ei_impl_widget_draw_children(ei_widget_t widget, ei_surface_t surface, ei_surface_t pick_surface,
                                  ei_rect_t* clipper)
{
    assert(widget != NULL && "Widget cannot be NULL");
    assert(surface != NULL && "Surface cannot be NULL");
    assert(pick_surface != NULL && "Pick surface cannot be NULL");

    ei_impl_widget_t* impl_widget = widget;


    ei_rect_t parent_content_rect = *impl_widget->content_rect;


    ei_widget_t child = impl_widget->children_head;
    while (child != NULL)
    {
        ei_impl_widget_t* child_impl = (ei_impl_widget_t*)child;


        ei_rect_t effective_clipper;
        if (clipper != NULL)
        {
            if (!intersection_rect(&effective_clipper, &parent_content_rect, clipper))
            {
                child = child_impl->next_sibling;
                continue;
            }
        }
        else
        {
            effective_clipper = parent_content_rect;
        }


        ei_rect_t child_rect = child_impl->screen_location;
        ei_rect_t intersection;
        if (intersection_rect(&intersection, &child_rect, &effective_clipper))
        {
            child_impl->wclass->drawfunc(child, surface, pick_surface, &intersection);
        }


        child = child_impl->next_sibling;
    }
}