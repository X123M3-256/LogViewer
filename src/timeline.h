#ifndef TIMELINE_H_INCLUDED
#define TIMELINE_H_INCLUDED
#include<gtk/gtk.h>
#include "log.h"

#define MAX_INTERVALS 16

typedef struct
{
int start;
int end;
int intervals;
int interval_points[2*MAX_INTERVALS];
int drag_active;
int drag_target;
float x_scale;
log_t* log;
}timeline_t;

void timeline_init(timeline_t* timeline,log_t* log,int start,int end);
void timeline_connect_signals(timeline_t* timeline,GtkWidget* widget);

#endif
