#ifndef MAP_H_INCLUDED
#define MAP_H_INCLUDED
#include<gtk/gtk.h>



int load_map();
//void open_map_window(GtkWidget *widget,gpointer data);
gboolean map_plot_motion(GtkWidget *widget,GdkEvent *event,gpointer user_data);

#endif
