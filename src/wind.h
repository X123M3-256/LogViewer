#ifndef WIND_H_INCLUDED
#define WIND_H_INCLUDED
#include<gtk/gtk.h>
#include "log.h"

extern log_t cur_log;
void wind_init();
void open_wind_window(GtkWidget *widget,gpointer data);




#endif
