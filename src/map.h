#ifndef MAP_H_INCLUDED
#define MAP_H_INCLUDED
#include<gtk/gtk.h>

typedef struct
{
char* name;
int image_width;
int loaded;
float lon_start,lon_end;
float lat_start,lat_end;
float elevation;
float pixels_per_meter;
cairo_surface_t* images[5];
}map_t;

#define NUM_MAPS 2

int load_map();
int set_map(float lon,float lat);
//void open_map_window(GtkWidget *widget,gpointer data);
gboolean map_plot_motion(GtkWidget *widget,GdkEvent *event,gpointer user_data);

#endif
