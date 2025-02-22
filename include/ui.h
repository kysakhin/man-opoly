#ifndef UI_H
#define UI_H
#include <gtk/gtk.h>

void activate(GtkApplication* app, gpointer user_data);
void close_tab(GtkButton *button, gpointer user_data);
void create_new_tab(GtkNotebook *notebook);


#endif // !UI_H
