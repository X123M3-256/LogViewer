#include<stdio.h>
#include<math.h>
#include "plot.h"

float max(float a,float b)
{
return a>b?a:b;
}

plot_t plot_new(plot_t* plot,log_t* log)
{
plot->log=log;
plot->left_margin=60;
plot->right_margin=20;
plot->top_margin=10;
plot->bottom_margin=20;

plot->x_tick_spacing=5;
plot->y_tick_spacing=500;
plot->v_tick_spacing=10;
plot->a_tick_spacing=2;
plot->c_tick_spacing=0.2;


plot->start=log->exit!=-1?log->exit:0;
plot->end=log->landing!=-1?log->deployment:plot->log->points-1;

//Calculate x data range
plot->x_start=log_get_time(plot->log,plot->start);
plot->x_range=0.2*(plot->end-plot->start)/plot->x_tick_spacing;
//Calculate y data range
float y_range=0.0;
	for(int i=plot->start;i<plot->end;i++)
	{
	//TODO include all plotted quantities and only plotted quantities
	y_range=max(y_range,plot->log->altitude[i]/plot->y_tick_spacing);
	y_range=max(y_range,plot->log->vel_d[i]/plot->v_tick_spacing);
	}
plot->y_start=0.0;
plot->y_range=ceil(y_range);

//TODO Calculate margin size
plot->active_plots=PLOT_ALTITUDE|PLOT_VEL_HORZ|PLOT_VEL_VERT;
}

//TODO allow distance travelled as X axis
void plot_data(plot_t* plot,cairo_t* cr,float x_scale,float y_scale,float (*xaxis)(log_t*,int),float (*data)(log_t*,int),float tick_spacing,float r,float g,float b)
{
cairo_move_to(cr,plot->left_margin,plot->bottom_margin+y_scale*data(plot->log,plot->start)/tick_spacing);
	for(int i=plot->start+1;i<plot->end;i++)
	{
	cairo_line_to(cr,plot->left_margin+x_scale*(xaxis(plot->log,i)-plot->x_start)/plot->x_tick_spacing,plot->bottom_margin+y_scale*data(plot->log,i)/tick_spacing);
	}
cairo_set_source_rgba(cr,r,g,b,1.0);
cairo_stroke(cr);
}

void plot_draw(plot_t* plot,cairo_t *cr,int width,int height)
{
float tick_length=5.0;
float tick_label_spacing=3.0;


float x_scale=(width-plot->left_margin-plot->right_margin)/(float)plot->x_range;
float y_scale=(height-plot->top_margin-plot->bottom_margin)/(float)plot->y_range;


cairo_set_line_width(cr,1.0);
cairo_move_to(cr,plot->left_margin+0.5,plot->top_margin+0.5);
cairo_line_to(cr,plot->left_margin+0.5,height-plot->bottom_margin+0.5);
cairo_line_to(cr,width-plot->right_margin+0.5,height-plot->bottom_margin+0.5);
cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
cairo_stroke(cr);
cairo_move_to(cr,width-plot->right_margin+0.5,height-plot->bottom_margin+0.5);
cairo_line_to(cr,width-plot->right_margin+0.5,plot->top_margin+0.5);
cairo_line_to(cr,plot->left_margin+0.5,plot->top_margin+0.5);
cairo_set_source_rgba(cr,0.5,0.5,0.5,1.0);
cairo_stroke(cr);

cairo_set_font_size(cr,12);
//Draw y labels
cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
	for(int i=0;i<=(int)plot->y_range;i++)
	{
	char label[256];
	sprintf(label,"%.0fm",i*plot->y_tick_spacing);
	cairo_text_extents_t extents;
	cairo_text_extents(cr,label,&extents);	
	cairo_move_to(cr,(int)(plot->left_margin-extents.width-extents.x_bearing-tick_length-tick_label_spacing),(int)(height-plot->bottom_margin-y_scale*i-extents.y_bearing-extents.height/2));
	cairo_show_text(cr,label);
	cairo_fill(cr);
	}
//Draw y ticks and grid lines
	for(int i=1;i<=(int)plot->y_range;i++)
	{
	float tick_height=(int)(height-plot->bottom_margin-y_scale*i)+0.5;
	cairo_move_to(cr,plot->left_margin-tick_length,tick_height);
	cairo_line_to(cr,plot->left_margin,tick_height);
	cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
	cairo_stroke(cr);
	
	cairo_move_to(cr,plot->left_margin,tick_height);
	cairo_line_to(cr,width-plot->right_margin,tick_height);
	cairo_set_source_rgba(cr,0.5,0.5,0.5,1.0);
	cairo_stroke(cr);
	}
//Draw x labels
cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
	for(int i=0;i<=(int)plot->x_range;i++)
	{
	char label[256];
	sprintf(label,"%.0f",i*plot->x_tick_spacing);
	cairo_text_extents_t extents;
	cairo_text_extents(cr,label,&extents);	
	cairo_move_to(cr,(int)(plot->left_margin+i*x_scale-extents.x_bearing-extents.width/2),(int)(height-plot->bottom_margin-extents.y_bearing+tick_length+tick_label_spacing));
	sprintf(label,"%.0fs",i*plot->x_tick_spacing);
	cairo_show_text(cr,label);
	cairo_fill(cr);	
	}
//Draw x ticks and grid lines
	for(int i=1;i<=(int)plot->x_range;i++)
	{
	float tick_x=(int)(plot->left_margin+x_scale*i)+0.5;
	cairo_move_to(cr,tick_x,height-plot->bottom_margin+tick_length);
	cairo_line_to(cr,tick_x,height-plot->bottom_margin);
	cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
	cairo_stroke(cr);

	cairo_move_to(cr,tick_x,height-plot->bottom_margin);
	cairo_line_to(cr,tick_x,plot->top_margin);
	cairo_set_source_rgba(cr,0.5,0.5,0.5,1.0);
	cairo_stroke(cr);
	}


/*
cairo_move_to(cr,plot->left_margin-x_scale*plot->x_start/plot->x_tick_spacing,0);
cairo_line_to(cr,plot->left_margin-x_scale*plot->x_start/plot->x_tick_spacing,height);
cairo_move_to(cr,plot->left_margin+x_scale*(log_get_time(plot->log,plot->log->deployment)-plot->x_start)/plot->x_tick_spacing,0);
cairo_line_to(cr,plot->left_margin+x_scale*(log_get_time(plot->log,plot->log->deployment)-plot->x_start)/plot->x_tick_spacing,height);
cairo_move_to(cr,plot->left_margin+x_scale*(log_get_time(plot->log,plot->log->landing)-plot->x_start)/plot->x_tick_spacing,0);
cairo_line_to(cr,plot->left_margin+x_scale*(log_get_time(plot->log,plot->log->landing)-plot->x_start)/plot->x_tick_spacing,height);
cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
cairo_stroke(cr);
*/

cairo_scale(cr,1,-1);
cairo_translate(cr,0,-height);
	if(plot->active_plots&PLOT_ALTITUDE)plot_data(plot,cr,x_scale,y_scale,log_get_time,log_get_altitude,plot->y_tick_spacing,0,0,0);
	if(plot->active_plots&PLOT_VEL_VERT)plot_data(plot,cr,x_scale,y_scale,log_get_time,log_get_vel_vert,plot->v_tick_spacing,0,1,0);
	if(plot->active_plots&PLOT_VEL_HORZ)plot_data(plot,cr,x_scale,y_scale,log_get_time,log_get_vel_horz,plot->v_tick_spacing,1,0,0);
	if(plot->active_plots&PLOT_ACC_VERT)plot_data(plot,cr,x_scale,y_scale,log_get_time,log_get_acc_vert,plot->a_tick_spacing,1,1,0);
	if(plot->active_plots&PLOT_ACC_HORZ)plot_data(plot,cr,x_scale,y_scale,log_get_time,log_get_acc_horz,plot->a_tick_spacing,1,0,1);
	if(plot->active_plots&PLOT_LIFT)plot_data(plot,cr,x_scale,y_scale,log_get_time,log_get_lift_coefficient,plot->c_tick_spacing,0,0.5,0);
	if(plot->active_plots&PLOT_DRAG)plot_data(plot,cr,x_scale,y_scale,log_get_time,log_get_drag_coefficient,plot->c_tick_spacing,0.5,0,0);
	if(plot->active_plots&PLOT_LD)plot_data(plot,cr,x_scale,y_scale,log_get_time,log_get_lift_drag_ratio,plot->c_tick_spacing,0,0,1);
	if(plot->active_plots&PLOT_GR)plot_data(plot,cr,x_scale,y_scale,log_get_time,log_get_glide_ratio,plot->c_tick_spacing,0,0,0.5);
}



