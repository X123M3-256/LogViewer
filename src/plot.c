#include<stdio.h>
#include<math.h>
#include "plot.h"

float max(float a,float b)
{
return a>b?a:b;
}

int unit_plots[4]={PLOT_ALTITUDE|PLOT_DISTANCE,PLOT_VEL_VERT|PLOT_VEL_HORZ|PLOT_VEL_TOTAL,PLOT_ACC_VERT|PLOT_ACC_HORZ,PLOT_LIFT|PLOT_DRAG|PLOT_LD|PLOT_GR};
const char* units[4]={"m","m/s","m/s\u00B2",""};
const char* unit_alternate[4]={"ft","mph","G",NULL};
float unit_alternate_scale[4]={3.28084,2.236936,0.101936799185,0};

float (*plot_functions[])(log_t*,int)={log_get_altitude,NULL,log_get_vel_horz,log_get_vel_vert,log_get_vel_total,log_get_acc_horz,log_get_acc_vert,log_get_lift_coefficient,log_get_drag_coefficient,log_get_lift_drag_ratio,log_get_glide_ratio};
float plot_colors[PLOT_NUM][3]={{0,0,0},{0,0,0},{1,0,0},{0,1,0},{1,1,0},{1,0,1},{0.5,0.5,0},{0,0.5,0},{0.5,0,0},{0,0,1},{0,0,0.5}};
int plot_units[PLOT_NUM]={0,0,1,1,1,2,2,3,3,3,3};
const char* plot_names[PLOT_NUM]={"Altitude","Distance","Horizontal Velocity","Vertical Velocity","Total Velocity","Horizontal Acceleration","Vertical Acceleration","Lift coefficient","Drag coefficient","L/D Ratio","Glide Ratio"};


plot_t plot_new(plot_t* plot,log_t* log)
{
plot->log=log;
plot->left_margin=60;
plot->right_margin=20;
plot->top_margin=10;
plot->bottom_margin=20;

plot->x_tick_spacing=5;
plot->y_tick_spacing[0]=500;
plot->y_tick_spacing[1]=10;
plot->y_tick_spacing[2]=2;
plot->y_tick_spacing[3]=0.2;


plot->start=log->exit!=-1?log->exit:0;
plot->end=log->landing!=-1?log->deployment:plot->log->points-1;
plot->active_plots=PLOT_ALTITUDE|PLOT_VEL_HORZ|PLOT_VEL_VERT;

plot_recalculate_range(plot);

//TODO Calculate margin size
}

float tick_length=5.0;
float tick_label_spacing=2.0;

void plot_recalculate_range(plot_t* plot)
{
//Calculate x data range
plot->x_start=0.0;
plot->x_range=log_get_time(plot->log,plot->end)/plot->x_tick_spacing;

//Calculate y data range
float y_range=0.0;
	for(int i=plot->start;i<plot->end;i++)
	{
		for(int j=0;j<PLOT_NUM;j++)
		{
		//Exclude glide ratio in range calculation if anything else is plotted; it's problematic
			if(j==PLOT_NUM&&plot->active_plots!=PLOT_GR)continue;
			if(plot->active_plots&(1<<j))y_range=max(y_range,plot_functions[j](plot->log,i)/plot->y_tick_spacing[plot_units[j]]);
		}
	}
plot->y_start=0.0;
plot->y_range=ceil(y_range);
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

void draw_y_label(plot_t* plot,cairo_t* cr,int i)
{
int height=0;
int width=0;
int x_offset=-1;
int y_offset=-6;
int unit_precision[]={0,0,0,1};
	for(int j=0;j<4;j++)
	{
		if(unit_plots[j]&plot->active_plots||(plot->active_plots==0&&j==0))
		{
		char label[256];
		sprintf(label,"%.*f%s",unit_precision[j],i*plot->y_tick_spacing[j],units[j]);
		cairo_text_extents_t extents;
		cairo_text_extents(cr,label,&extents);
			if(x_offset==-1)x_offset=-((int)(extents.width)-(int)(extents.width/2))-tick_length-tick_label_spacing;
			else if(i==0)break;//Only show first unit for very bottom tick, otherwise it runs off the screen
		cairo_move_to(cr,(int)(x_offset-(int)(extents.width/2)-extents.x_bearing),(int)(height-extents.y_bearing+y_offset));
		cairo_show_text(cr,label);
		cairo_fill(cr);
		width=max(extents.width,width);
		height+=extents.height+3;
		}
	}

}



//TODO move this to log.c
float get_plot_value(plot_t* plot,float x,int i)
{
///Find nearest two points by binary search, then interpolate
int l=0;
int r=plot->log->points-1;
int mid=(l+r)/2;
	while(mid!=l&&mid!=r)
	{
		if(log_get_time(plot->log,mid)<x)l=mid;
		else r=mid;
	mid=(l+r)/2;
	}
float value=0.0;
	if(l==r)value=plot_functions[i](plot->log,l);//TODO I don't think this can happen
	else
	{	
	float u=(x-log_get_time(plot->log,l))/(log_get_time(plot->log,r)-log_get_time(plot->log,l));
	value=plot_functions[i](plot->log,l)*(1-u)+u*plot_functions[i](plot->log,r);
	}

return value;
}


void draw_cursor(plot_t* plot,cairo_t* cr,float x,float x_scale,float y_scale)
{
//Get values of all plotted quantities at cursor point
float values[PLOT_NUM];
	for(int i=0;i<PLOT_NUM;i++)
	{
		if(plot->active_plots&(1<<i))
		{
		values[i]=get_plot_value(plot,x,i);
		}
	}

//Draw vertical dashed line
float x_coord=(int)(plot->cursor_x)+0.5;
cairo_save(cr);
cairo_set_line_width(cr,1.2);
double dashes[]={5.0,5.0};
cairo_set_dash (cr,dashes,2,0);
cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
//Draw dots at each y value
cairo_move_to(cr,x_coord,plot->bottom_margin);
cairo_line_to(cr,x_coord,plot->bottom_margin+y_scale*plot->y_range);
cairo_stroke(cr);
cairo_restore(cr);
	for(int i=0;i<PLOT_NUM;i++)
	{
		if(plot->active_plots&(1<<i))
		{
		float y_coord=(int)(plot->bottom_margin+y_scale*values[i]/plot->y_tick_spacing[plot_units[i]])+0.5;

		cairo_set_source_rgba(cr,plot_colors[i][0],plot_colors[i][1],plot_colors[i][2],1.0);
		cairo_arc(cr,x_coord,y_coord,2.5,0,2*M_PI);
		cairo_fill(cr);
		}
	}

//Draw text box
cairo_save(cr);
cairo_translate(cr,plot->left_margin+(int)(plot->x_range*x_scale-30),plot->bottom_margin+(int)(plot->y_range*y_scale)-20);
cairo_scale(cr,1,-1);
cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);

//Calculate bounds
int line_width=15;
int line_padding=10;
int row_padding=2;
int row_height=18;
int row_text_y=4;
int value_offset=45;
int alternate_offset=120;

float label_width=0; 
int height=0;
	for(int i=0;i<PLOT_NUM;i++)
	{
		if(plot->active_plots&(1<<i))
		{
		cairo_text_extents_t extents;
		cairo_text_extents(cr,plot_names[i],&extents);
		label_width=max(label_width,extents.x_advance);
		height+=row_height;
		}
	}
label_width=floor(label_width);

int width=line_width+2*line_padding+(int)label_width+152;

//Draw rectangle
cairo_translate(cr,-width,0);
cairo_rectangle(cr,0.5,0.5,width,height);
cairo_set_source_rgba(cr,1.0,1.0,1.0,1.0);
cairo_fill(cr);
cairo_rectangle(cr,0.5,0.5,width,height);
cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
cairo_stroke(cr);




int unit_precision[4]={0,1,1,2};
float y_offset=0;

	for(int i=0;i<PLOT_NUM;i++)
	{
		if(plot->active_plots&(1<<i))
		{
		y_offset+=row_height;

		cairo_move_to(cr,line_padding,y_offset-row_height/2+0.5);
		cairo_line_to(cr,line_padding+line_width,y_offset-row_height/2+0.5);
		cairo_set_source_rgba(cr,plot_colors[i][0],plot_colors[i][1],plot_colors[i][2],1.0);
		cairo_stroke(cr);
		int x_offset=line_width+2*line_padding;
		//Draw label
		cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
		cairo_move_to(cr,x_offset,y_offset-row_text_y);
		cairo_show_text(cr,plot_names[i]);
		cairo_fill(cr);
		x_offset+=label_width;
		
		char str[64];
		int unit=plot_units[i];
		cairo_text_extents_t extents;
		sprintf(str,"%.*f",unit_precision[unit],values[i]);
		cairo_text_extents(cr,str,&extents);
		sprintf(str,"%.*f%s",unit_precision[unit],values[i],units[unit]);
		cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
		cairo_move_to(cr,x_offset+value_offset-extents.x_advance,y_offset-row_text_y);
		cairo_show_text(cr,str);
		cairo_fill(cr);
		
			if(unit_alternate[unit])
			{	
			cairo_text_extents_t extents;
			sprintf(str,"%.*f",unit_precision[unit],unit_alternate_scale[unit]*values[i]);
			cairo_text_extents(cr,str,&extents);
			sprintf(str,"%.*f%s",unit_precision[unit],unit_alternate_scale[unit]*values[i],unit_alternate[unit]);
			cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
			cairo_move_to(cr,x_offset+alternate_offset-extents.x_advance,y_offset-row_text_y);
			cairo_show_text(cr,str);
			cairo_fill(cr);
			}
		//x_offset+=label_width+2*label_padding;
		//Draw value
		//cairo_move_to(cr,x_offset,y_offset-row_text_y);
		//cairo_show_text(cr,value[i]);
		//cairo_fill(cr);
		}
	}
cairo_restore(cr);
}


void plot_draw(plot_t* plot,cairo_t *cr,int width,int height)
{
float x_scale=(width-plot->left_margin-plot->right_margin)/(float)plot->x_range;
float y_scale=(height-plot->top_margin-plot->bottom_margin)/(float)plot->y_range;

cairo_set_font_size(cr,12);
cairo_scale(cr,1,-1);
cairo_translate(cr,0,-height);

cairo_set_line_width(cr,1.0);
cairo_move_to(cr,plot->left_margin+0.5,height-plot->top_margin+0.5);
cairo_line_to(cr,plot->left_margin+0.5,plot->bottom_margin+0.5);
cairo_line_to(cr,width-plot->right_margin+0.5,plot->bottom_margin+0.5);
cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
cairo_stroke(cr);
cairo_move_to(cr,width-plot->right_margin+0.5,plot->bottom_margin+0.5);
cairo_line_to(cr,width-plot->right_margin+0.5,height-plot->top_margin+0.5);
cairo_line_to(cr,plot->left_margin+0.5,height-plot->top_margin+0.5);
cairo_set_source_rgba(cr,0.5,0.5,0.5,1.0);
cairo_stroke(cr);


//Draw y labels
cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
	for(int i=0;i<=(int)plot->y_range;i++)
	{
	cairo_save(cr);
	cairo_translate(cr,plot->left_margin,(int)(plot->bottom_margin+i*y_scale));
	cairo_scale(cr,1,-1);
	draw_y_label(plot,cr,i);
	cairo_restore(cr);
	}
//Draw y ticks and grid lines
	for(int i=1;i<=(int)plot->y_range;i++)
	{
	float tick_height=(int)(plot->bottom_margin+y_scale*i)+0.5;
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
	cairo_save(cr);
	cairo_translate(cr,plot->left_margin+(int)(i*x_scale),plot->bottom_margin);
	cairo_scale(cr,1,-1);
	
	char label[256];
	sprintf(label,"%.0f",i*plot->x_tick_spacing);
	cairo_text_extents_t extents;
	cairo_text_extents(cr,label,&extents);	
	
	cairo_move_to(cr,(int)(-extents.x_bearing-extents.width/2),(int)(-extents.y_bearing+tick_length+tick_label_spacing));
	sprintf(label,"%.0fs",i*plot->x_tick_spacing);
	cairo_show_text(cr,label);
	cairo_fill(cr);	
	cairo_restore(cr);
	}
//Draw x ticks and grid lines
	for(int i=1;i<=(int)plot->x_range;i++)
	{
	float tick_x=(int)(plot->left_margin+x_scale*i)+0.5;
	cairo_move_to(cr,tick_x,plot->bottom_margin-tick_length);
	cairo_line_to(cr,tick_x,plot->bottom_margin);
	cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
	cairo_stroke(cr);

	cairo_move_to(cr,tick_x,plot->bottom_margin);
	cairo_line_to(cr,tick_x,height-plot->top_margin);
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

	for(int i=0;i<PLOT_NUM;i++)
	{
	if(plot->active_plots&(1<<i))plot_data(plot,cr,x_scale,y_scale,log_get_time,plot_functions[i],plot->y_tick_spacing[plot_units[i]],plot_colors[i][0],plot_colors[i][1],plot_colors[i][2]);
	}
draw_cursor(plot,cr,plot->x_tick_spacing*(plot->cursor_x-plot->left_margin)/x_scale,x_scale,y_scale);
}



