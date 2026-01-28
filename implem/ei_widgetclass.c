#include "ei_widgetclass.h"
#include "ei_implementation.h"
#include <assert.h>
#include <string.h>

/**
 * Global list of registered widget classes (linked list).
 */
static ei_widgetclass_t* class_list = NULL;

size_t ei_widget_struct_size(void)
{
    return sizeof(ei_impl_widget_t);
}

void ei_widgetclass_register(ei_widgetclass_t* widgetclass)
{
    assert(widgetclass != NULL && widgetclass->name[0] != '\0');
    widgetclass->next = class_list;
    class_list = widgetclass;
}

ei_widgetclass_t* ei_widgetclass_from_name(ei_const_string_t name)
{
    ei_widgetclass_t* current = class_list;
    while (current != NULL)
    {
        if (strcmp(current->name, name) == 0)
        {
            return current;
        }
        current = current->next;
    }
    return NULL; // Classe non trouvée
}