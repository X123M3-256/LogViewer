#ifndef PLOT_H_INCLUDED
#define PLOT_H_INCLUDED
#include <cairo.h>
#include "log.h"

#define PLOT_NUM 12

enum plots
{
PLOT_TIME=1,
PLOT_ALTITUDE=2,
PLOT_DISTANCE=4,
PLOT_VEL_HORZ=8,
PLOT_VEL_VERT=16,
PLOT_VEL_TOTAL=32,
PLOT_ACC_HORZ=64,
PLOT_ACC_VERT=128,
PLOT_LIFT=256,
PLOT_DRAG=512,
PLOT_LD=1024,
PLOT_GR=2048
};



typedef struct
{
int width;
int height;
int x_axis_variable;
float x_scale;
float y_scale;
int left_margin;
int right_margin;
int top_margin;
int bottom_margin;
float x_tick_spacing;
float y_tick_spacing[5];

float x_start;
float x_end;

float x_range;
float y_range;

float cursor_x;
float cursor_range;

int start;
int end;

int active_plots;
log_t* log;
}plot_t;


void plot_set_size(plot_t* plot,int width,int height);

extern const char* units[5];
extern const char* unit_alternate[5];
extern float unit_alternate_scale[5];

extern float (*plot_functions[])(log_t*,int);
extern float plot_colors[PLOT_NUM][3];
extern int plot_units[PLOT_NUM];
extern const char* plot_names[PLOT_NUM];
extern const char* plot_range_names[PLOT_NUM];


plot_t plot_new(plot_t* plot,log_t* log);
void plot_set_range(plot_t* plot,int start,int end);
void plot_recalculate_range(plot_t* plot);
void plot_draw(plot_t* plot,cairo_t *cr);




#endif
