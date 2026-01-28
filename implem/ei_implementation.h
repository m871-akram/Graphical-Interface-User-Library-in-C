/**
 *@file  ei_implementation.h
 *
 *@brief Private definitions.
 *
 */

#ifndef EI_IMPLEMENTATION_H
#define EI_IMPLEMENTATION_H

#include "hw_interface.h" // Pour ei_surface_t
#include "ei_types.h"     // Pour ei_widget_t, ei_color_t, ei_rect_t, ei_anchor_t, ei_relief_t
#include "ei_widget.h"    // Pour ei_widget_destructor_t, ei_widgetclass_t (indirectement via ei_widget.h qui inclut ei_widgetclass.h)


#include "ei_event.h"


void ei_frame_register_class();

void ei_button_register_class();

void ei_toplevel_register_class();

/**
 * @brief Global variable to store the currently active widget.
 */
extern ei_widget_t active_widget;

/**
 * @brief Global variable to store the default event handling function.
 */
extern ei_default_handle_func_t default_handle_func;

/**
 * \brief	Returns the picking offscreen surface used for widget identification.
 *
 * @return 			The picking surface.
 */
extern ei_surface_t pick_surface;

/**
 * Structure des arêtes pour l'algorithme scanline utilisé dans draw_polygon.
 */
typedef struct edge_t
{
    int ymax; // Ordonnée maximale de l'arête
    float x; // Abscisse au y minimum
    float inv_m; // Inverse de la pente : dx/dy
    struct edge_t* next; // Pointeur vers l'arête suivante
} edge_t;

/**
 * \brief Dessine un segment de ligne entre deux points sur une surface avec l'algorithme de Bresenham.
 *
 * @param surface La surface où dessiner le segment.
 * @param point_1 Le point de départ du segment.
 * @param point_2 Le point d'arrivée du segment.
 * @param couleur La couleur du segment.
 * @param clipper Si non NULL, restreint le dessin à ce rectangle.
 */
void draw_line(ei_surface_t surface, ei_point_t point_1, ei_point_t point_2, ei_color_t couleur,
               const ei_rect_t* clipper);

/**
 * \brief Calcule l'inverse de la pente de la droite définie par deux points.
 *
 * @param p1 Premier point.
 * @param p2 Deuxième point.
 * @return L'inverse de la pente (dx/dy) ou 0 si dy == 0.
 */
float calcule_inverse_pente(ei_point_t p1, ei_point_t p2);

/**
 * \brief Transforme la liste de points d’un polygone en une table des arêtes, triées par y minimum.
 *
 * @param point_array Tableau de points du polygone.
 * @param point_array_size Nombre de points dans le tableau.
 * @param edge_table Table des arêtes, indexée par y - y_min.
 * @param y_min Ordonnée minimale des arêtes.
 * @param y_max Ordonnée maximale des arêtes pour le clipping.
 */
void creer_table_tc(const ei_point_t* point_array, size_t point_array_size, edge_t* edge_table[], int y_min, int y_max);

/**
 * \brief Supprime les arêtes terminées de la table des côtés actifs (TCA) à l'ordonnée y.
 *
 * @param y Ordonnée actuelle pour vérifier les arêtes terminées.
 * @param tca Pointeur vers la tête de la TCA.
 */
void supprimer_arete_tca(int y, edge_t** tca);

/**
 * \brief Trie la table des côtés actifs (TCA) par coordonnée x croissante.
 *
 * @param tca Pointeur vers la tête de la TCA.
 */
void trier_tca_x(edge_t** tca);

/**
 * \brief Ajoute une arête à la table des côtés actifs (TCA).
 *
 * @param edge Arête à ajouter.
 * @param tca Pointeur vers la tête de la TCA.
 */
void ajoute_arete_tca(edge_t* arete, edge_t** tca);

/**
 * \brief Dessine une ligne horizontale de pixels avec une couleur donnée.
 *
 * @param surface La surface où dessiner.
 * @param x1 Abscisse de début de la ligne.
 * @param x2 Abscisse de fin de la ligne.
 * @param y Ordonnée de la ligne.
 * @param couleur Couleur des pixels.
 * @param clipper Si non NULL, restreint le dessin à ce rectangle.
 */
void draw_horizontal_line(ei_surface_t surface, int x1, int x2, int y, ei_color_t couleur, const ei_rect_t* clipper);

/**
 * @brief   Computes the intersection of two rectangles.
 *
 * @param   dest    The rectangle to store the intersection result.
 * @param   a       The first input rectangle.
 * @param   b       The second input rectangle.
 *
 * @return  true if the rectangles intersect (non-empty intersection), false otherwise.
 */
bool intersection_rect(ei_rect_t* dest, const ei_rect_t* a, const ei_rect_t* b);

/**
 * @brief   Clamps a rectangle to surface boundaries to prevent out-of-bounds drawing.
 *
 * @param   rect        The rectangle to clamp (modified in place).
 * @param   surface     The surface whose boundaries to use for clamping.
 */
void clamp_rect_to_surface(ei_rect_t* rect, ei_surface_t surface);


/**
 * \brief	A structure storing the placement parameters of a widget.
 *		You have to define this structure: no suggestion provided.
 */

typedef struct ei_impl_placer_params_t
{
    ei_anchor_t anchor;
    int x;
    int y;
    int width;
    int height;
    float rel_x;
    float rel_y;
    float rel_width;
    float rel_height;
} ei_impl_placer_params_t;

/**
 * \brief	Tells the placer to recompute the geometry of a widget.
 *		The widget must have been previously placed by a call to \ref ei_place.
 *		Geometry re-computation is necessary for example when the text label of
 *		a widget has changed, and thus the widget "natural" size has changed.
 *
 * @param	widget		The widget which geometry must be re-computed.
 */
void ei_impl_placer_run(ei_widget_t widget);

/**
 * \brief	Fields common to all types of widget. Every widget classes specializes this base
 *		class by adding its own fields.
 */
typedef struct ei_impl_widget_t
{
    ei_widgetclass_t* wclass;
    ///< The class of widget of this widget. Avoids the field name "class" which is a keyword in C++.
    uint32_t pick_id; ///< Id of this widget in the picking offscreen.
    ei_color_t pick_color; ///< pick_id encoded as a color.
    void* user_data; ///< Pointer provided by the programmer for private use. May be NULL.
    ei_widget_destructor_t destructor;
    ///< Pointer to the programmer's function to call before destroying this widget. May be NULL.

    /* Widget Hierarchy Management */
    ei_widget_t parent; ///< Pointer to the parent of this widget.
    ei_widget_t children_head;
    ///< Pointer to the first child of this widget. Children are chained with the "next_sibling" field.
    ei_widget_t children_tail; ///< Pointer to the last child of this widget.
    ei_widget_t next_sibling; ///< Pointer to the next child of this widget's parent widget.

    /* Geometry Management */
    ei_impl_placer_params_t* placer_params;
    ///< Pointer to the placer parameters for this widget. If NULL, the widget is not currently managed and thus, is not displayed on the screen.
    ei_size_t requested_size; ///< See \ref ei_widget_get_requested_size.
    ei_rect_t screen_location; ///< See \ref ei_widget_get_screen_location.
    ei_rect_t* content_rect; ///< See ei_widget_get_content_rect. By defaults, points to the screen_location.
} ei_impl_widget_t;

typedef struct ei_impl_button_t
{
    ei_impl_widget_t widget; // Attributs communs
    ei_color_t color; // Couleur de fond
    int border_width; // Largeur de la bordure
    int corner_radius; // Rayon des coins
    ei_relief_t relief; // Relief (enfoncé/relevé)
    ei_string_t text; // Texte du bouton
    ei_font_t text_font; // Police du texte
    ei_color_t text_color; // Couleur du texte
    ei_anchor_t text_anchor; // Ancre du texte
    ei_surface_t img; // Image (si utilisée)
    ei_rect_t* img_rect; // Sous-rectangle de l’image
    ei_anchor_t img_anchor; // Ancre de l’image
    ei_callback_t callback; // Traitant externe
    ei_user_param_t user_param; // Paramètre utilisateur
    bool is_pressed; // État du bouton (enfoncé ou non)
} ei_impl_button_t;

/**
 * \brief Structure representing a widget of the "frame" class.
 *        Inherits from ei_impl_widget_t and adds specific attributes for frames.
 */
typedef struct ei_impl_frame_t
{
    ei_impl_widget_t widget; ///< Common widget attributes (wclass, pick_id, pick_color, etc.)
    ei_size_t requested_size; ///< Requested size for the widget, including borders
    ei_color_t color; ///< Background color of the frame (defaults to ei_default_background_color)
    int border_width; ///< Width of the border decoration in pixels (defaults to 0)
    ei_relief_t relief;
    ///< Appearance of the border (ei_relief_none, ei_relief_raised, ei_relief_sunken; defaults to ei_relief_none)
    ei_string_t text; ///< Text to display in the frame, or NULL if none
    ei_font_t text_font; ///< Font used for the text (defaults to ei_default_font)
    ei_color_t text_color; ///< Color of the text (defaults to ei_font_default_color)
    ei_anchor_t text_anchor; ///< Anchor point for the text within the frame (defaults to ei_anc_center)
    ei_surface_t img; ///< Image to display in the frame, or NULL if none
    ei_rect_t* img_rect; ///< Sub-rectangle of the image to display, or NULL for the whole image
    ei_anchor_t img_anchor; ///< Anchor point for the image within the frame (defaults to ei_anc_center)
} ei_impl_frame_t;

/**
 * @file ei_implementation.h
 */

/**
 * \brief Structure representing a widget of the "toplevel" class.
 *        Inherits from ei_impl_widget_t and adds specific attributes for toplevel windows.
 */
typedef struct ei_impl_toplevel_t
{
    ei_impl_widget_t widget; ///< Common widget attributes (wclass, pick_id, pick_color, etc.)

    // Configuration attributes (from ei_toplevel_configure)
    ei_color_t color; ///< Background color of the content area (defaults to ei_default_background_color)
    int border_width; ///< Width of the border in pixels (defaults to 4)
    ei_string_t title; ///< Title displayed in the title bar (defaults to "Toplevel")
    bool closable; ///< If true, shows a close button in the title bar (defaults to true)
    ei_axis_set_t resizable; ///< Axes along which the toplevel can be resized (defaults to ei_axis_both)
    ei_size_t min_size; ///< Minimum content size for resizable toplevels (defaults to 160x120)

    // Geometry and interaction attributes
    int title_bar_height; ///< Height of the title bar (fixed or computed, e.g., 30 pixels)
    ei_rect_t title_bar_rect; ///< Rectangle of the title bar for dragging (computed during geometry management)
    ei_rect_t close_button_rect; ///< Rectangle of the close button, if closable (computed during geometry management)
    ei_rect_t resize_handle_rect;
    ///< Rectangle of the resize handle, if resizable (computed during geometry management)

    // Internal states for drag and resize operations
    bool is_moving; ///< True if the toplevel is being dragged
    bool is_resizing; ///< True if the toplevel is being resized
    ei_point_t drag_start_pos; ///< Mouse position at the start of drag/resize
    ei_point_t widget_start_pos; ///< Widget position at the start of drag
    ei_size_t widget_start_size; ///< Widget content size at the start of resize
} ei_impl_toplevel_t;


/**
 * @brief	Draws the children of a widget.
 * 		The children are drawn within the limits of the clipper and
 * 		the widget's content_rect.
 *
 * @param	widget		The widget whose children are drawn.
 * @param	surface		A locked surface where to draw the widget's children.
 * @param	pick_surface	The picking offscreen.
* @param	clipper		If not NULL, the drawing is restricted within this rectangle
 * (expressed in the surface reference frame).
 */
void ei_impl_widget_draw_children(ei_widget_t widget,
                                  ei_surface_t surface,
                                  ei_surface_t pick_surface,
                                  ei_rect_t* clipper);

/**
 * \brief	Converts the red, green, blue and alpha components of a color into a 32 bits integer
 * 		than can be written directly in the memory returned by \ref hw_surface_get_buffer.
 * 		The surface parameter provides the channel order.
 *
 * @param	surface		The surface where to store this pixel, provides the channels order.
 * @param	color		The color to convert.
 *
* @return 			The 32 bit integer corresponding to the color. The alpha component
 * of the color is ignored in the case of surfaces that don't have an
 * alpha channel.
 */
uint32_t ei_impl_map_rgba(ei_surface_t surface, ei_color_t color);


#endif