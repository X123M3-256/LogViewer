#ifndef PLOT_H_INCLUDED
#define PLOT_H_INCLUDED
#include <cairo.h>
#include "log.h"

enum plots
{
PLOT_ALTITUDE=1,
PLOT_DISTANCE=2,
PLOT_VEL_VERT=4,
PLOT_VEL_HORZ=8,
PLOT_ACC_VERT=16,
PLOT_ACC_HORZ=32,
PLOT_LIFT=64,
PLOT_DRAG=128,
PLOT_LD=256,
PLOT_GR=512
};



typedef struct
{
int left_margin;
int right_margin;
int top_margin;
int bottom_margin;
float x_tick_spacing;
float y_tick_spacing;
float v_tick_spacing;
float a_tick_spacing;
float c_tick_spacing;

float x_start;
float x_range;
float y_start;
float y_range;

float x_scale;
float y_scale;
float v_scale;

int start;
int end;

int active_plots;
log_t* log;
}plot_t;



plot_t plot_new(plot_t* plot,log_t* log);
void plot_draw(plot_t* plot,cairo_t *cr,int width,int height);




#endif
