#ifndef PLOT_H_INCLUDED
#define PLOT_H_INCLUDED
#include <cairo.h>
#include "log.h"

#define PLOT_NUM 11

enum plots
{
PLOT_ALTITUDE=1,
PLOT_DISTANCE=2,
PLOT_VEL_HORZ=4,
PLOT_VEL_VERT=8,
PLOT_VEL_TOTAL=16,
PLOT_ACC_HORZ=32,
PLOT_ACC_VERT=64,
PLOT_LIFT=128,
PLOT_DRAG=256,
PLOT_LD=512,
PLOT_GR=1024
};



typedef struct
{
int left_margin;
int right_margin;
int top_margin;
int bottom_margin;
float x_tick_spacing;
float y_tick_spacing[4];

float x_start;
float x_range;
float y_start;
float y_range;

float x_scale;
float y_scale;

float cursor_x;

int start;
int end;

int active_plots;
log_t* log;
}plot_t;



plot_t plot_new(plot_t* plot,log_t* log);
void plot_recalculate_range(plot_t* plot);
void plot_draw(plot_t* plot,cairo_t *cr,int width,int height);




#endif
