#include<stdio.h>
#include<math.h>
#include "plot.h"

float max(float a,float b)
{
return a>b?a:b;
}

int unit_plots[5]={PLOT_ALTITUDE|PLOT_DISTANCE,PLOT_VEL_VERT|PLOT_VEL_HORZ|PLOT_VEL_TOTAL,PLOT_ACC_VERT|PLOT_ACC_HORZ,PLOT_TIME,PLOT_LIFT|PLOT_DRAG|PLOT_LD|PLOT_GR};
const char* units[5]={"m","m/s","m/s\u00B2","s",""};
const char* unit_alternate[5]={"ft","mph","G",NULL,NULL};
float unit_alternate_scale[5]={3.28084,2.236936,0.101936799185,0,0};

float (*plot_functions[])(log_t*,int)={log_get_time,log_get_altitude,log_get_distance,log_get_vel_horz,log_get_vel_vert,log_get_vel_total,log_get_acc_horz,log_get_acc_vert,log_get_lift_coefficient,log_get_drag_coefficient,log_get_lift_drag_ratio,log_get_glide_ratio};
float plot_colors[PLOT_NUM][3]={{1,1,1},{0,0,0},{0,0,0},{1,0,0},{0,1,0},{1,1,0},{1,0,1},{0.5,0.5,0},{0,0.5,0},{0.5,0,0},{0,0,1},{0,0,0.5}};
int plot_units[PLOT_NUM]={3,0,0,1,1,1,2,2,4,4,4,4};
const char* plot_names[PLOT_NUM]={"Time","Altitude","Distance","Horizontal Velocity","Vertical Velocity","Total Velocity","Horizontal Acceleration","Vertical Acceleration","Lift Coefficient","Drag Coefficient","L/D Ratio","Glide Ratio"};
const char* plot_range_names[PLOT_NUM]={"Time (Difference)","Altitude (Difference)","Distance (Difference)","Horizontal Velocity (Average)","Vertical Velocity (Average)","Total Velocity (Average)","Horizontal Acceleration (Average)","Vertical Acceleration (Average)","Lift Coefficient (Average)","Drag Coefficient (Average)","L/D Ratio (Average)","Glide Ratio (Average)"};


void plot_set_range(plot_t* plot,int start,int end)
{
plot->start=start;
plot->end=end;
plot->x_start=plot_functions[plot->x_axis_variable](plot->log,plot->start);
plot->x_end=(plot_functions[plot->x_axis_variable](plot->log,plot->end)-plot->x_start);
}


//TODO take account of actual range in calculating this
float calculate_tick_spacing(int ticks,float range)
{
float spacing_candidates[]={0.1,0.2,0.25,0.5,1.0,2.0,2.5,5.0,10.0,20.0,25.0,50.0,100.0,200.0,250.0,500.0,1000.0,2000.0,2500.0,5000.0,10000.0};
int i=0;
	for(;i<21;i++)
	{
		if(ticks*spacing_candidates[i]>range)break;
	}
return spacing_candidates[i];
}


//TODO improve this
void plot_recalculate_range(plot_t* plot)
{
int usable_width=plot->width-plot->left_margin-plot->right_margin;
int usable_height=plot->height-plot->top_margin-plot->bottom_margin;

float target=100.0;
int x_ticks=(int)((usable_width/target)+0.5);
int y_ticks=(int)((usable_height/target)+0.5);

//Calculate x tick spacing and data range
plot->x_tick_spacing=calculate_tick_spacing(x_ticks,plot->x_end-plot->x_start);
plot->x_range=(plot->x_end-plot->x_start)/plot->x_tick_spacing;
printf("%f\n",plot->x_range);

//Calculate y tick spacing and data range
float y_ranges[5]={0,0,0,0,0};
	for(int i=plot->start;i<plot->end;i++)
	{
		for(int j=0;j<PLOT_NUM;j++)
		{
		//Exclude glide ratio in range calculation if anything else is plotted; it's problematic
			if(j==PLOT_NUM-1&&plot->active_plots!=PLOT_GR)continue;
			if(plot->active_plots&(1<<j))y_ranges[plot_units[j]]=max(y_ranges[plot_units[j]],plot_functions[j](plot->log,i));
		}
	}
	for(int i=0;i<5;i++)
	{
	plot->y_tick_spacing[i]=calculate_tick_spacing(y_ticks,y_ranges[i]);
	}
float y_range=0.0;
	for(int i=plot->start;i<plot->end;i++)//TODO avoid this loop
	{
		for(int j=0;j<PLOT_NUM;j++)
		{
		//Exclude glide ratio in range calculation if anything else is plotted; it's problematic
			if(j==PLOT_NUM-1&&plot->active_plots!=PLOT_GR)continue;
			if(plot->active_plots&(1<<j))y_range=max(y_range,plot_functions[j](plot->log,i)/plot->y_tick_spacing[plot_units[j]]);
		}
	}

plot->y_range=ceil(y_range);

plot->x_scale=(plot->width-plot->left_margin-plot->right_margin)/(float)plot->x_range;
plot->y_scale=(plot->height-plot->top_margin-plot->bottom_margin)/(float)plot->y_range;
}

plot_t plot_new(plot_t* plot,log_t* log)
{
plot->log=log;
plot->left_margin=60;
plot->right_margin=10;
plot->top_margin=10;
plot->bottom_margin=20;

plot->x_axis_variable=0;
plot->active_plots=PLOT_ALTITUDE|PLOT_VEL_HORZ|PLOT_VEL_VERT;

plot_set_range(plot,log->exit,log->deployment);

plot->cursor_x=0.0;
plot->cursor_range=-1.0;

plot_set_size(plot,640,480);

//TODO Calculate margin size
}

float tick_length=5.0;
float tick_label_spacing=2.0;



void plot_set_size(plot_t* plot,int width,int height)
{
plot->width=width;
plot->height=height;
plot_recalculate_range(plot);
}

//TODO consider allowing negative axis
void plot_data(plot_t* plot,cairo_t* cr,float (*xaxis)(log_t*,int),float (*data)(log_t*,int),float tick_spacing,float r,float g,float b)
{
cairo_save(cr);
cairo_rectangle(cr,plot->left_margin,plot->bottom_margin,plot->width-plot->left_margin-plot->right_margin,plot->height-plot->top_margin-plot->bottom_margin);
cairo_clip(cr);
	for(int i=plot->start;i<=plot->end;i++)
	{
	cairo_line_to(cr,plot->left_margin+plot->x_scale*(xaxis(plot->log,i)-plot->x_start)/plot->x_tick_spacing,plot->bottom_margin+plot->y_scale*fabs(data(plot->log,i))/tick_spacing);
	}
cairo_set_source_rgba(cr,r,g,b,1.0);
cairo_stroke(cr);
cairo_restore(cr);
}

void draw_y_label(plot_t* plot,cairo_t* cr,int i)
{
int height=0;
int width=0;
int x_offset=-1;
int y_offset=-6;
int unit_precision[]={0,0,0,0,1};
	for(int j=0;j<5;j++)
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


void draw_cursor(plot_t* plot,cairo_t* cr,float x,float* values)
{
//Get values of all quantities at cursor point
int l,r;
float u;
log_get_point_by_value(plot->log,plot_functions[plot->x_axis_variable],x,&l,&r,&u);
	for(int i=0;i<PLOT_NUM;i++)
	{
	values[i]=plot_functions[i](plot->log,l)*(1-u)+u*plot_functions[i](plot->log,r);
	}

//Draw vertical dashed line
float x_coord=(int)(plot->left_margin+plot->x_scale*(x-plot->x_start)/plot->x_tick_spacing)+0.5;
cairo_save(cr);
cairo_set_line_width(cr,1.2);
double dashes[]={5.0,5.0};
cairo_set_dash (cr,dashes,2,0);
cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
//Draw dots at each y value
cairo_move_to(cr,x_coord,plot->bottom_margin);
cairo_line_to(cr,x_coord,plot->bottom_margin+plot->y_scale*plot->y_range);
cairo_stroke(cr);
cairo_restore(cr);
	for(int i=0;i<PLOT_NUM;i++)
	{
		if(plot->active_plots&(1<<i))
		{
		float y_coord=(int)(plot->bottom_margin+plot->y_scale*values[i]/plot->y_tick_spacing[plot_units[i]])+0.5;

		cairo_set_source_rgba(cr,plot_colors[i][0],plot_colors[i][1],plot_colors[i][2],1.0);
		cairo_arc(cr,x_coord,y_coord,2.5,0,2*M_PI);
		cairo_fill(cr);
		}
	}
}



void draw_legend(plot_t* plot,cairo_t* cr,const char** names,float* values)
{
//Draw text box
cairo_save(cr);
cairo_translate(cr,plot->left_margin+(int)(plot->x_range*plot->x_scale-30),plot->bottom_margin+(int)(plot->y_range*plot->y_scale)-20);
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
		if(i==0||plot->active_plots&(1<<i))
		{
		cairo_text_extents_t extents;
		cairo_text_extents(cr,names[i],&extents);
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

int unit_precision[5]={0,1,1,1,2};
float y_offset=0;

	for(int i=0;i<PLOT_NUM;i++)
	{
		if(i==0||plot->active_plots&(1<<i))
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
		cairo_show_text(cr,names[i]);
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

void plot_draw(plot_t* plot,cairo_t *cr)
{
cairo_set_font_size(cr,12);
cairo_scale(cr,1,-1);
cairo_translate(cr,0,-plot->height);

cairo_set_line_width(cr,1.0);

//Draw y labels
cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
	for(int i=0;i<=(int)plot->y_range;i++)
	{
	cairo_save(cr);
	cairo_translate(cr,plot->left_margin,(int)(plot->bottom_margin+i*plot->y_scale));
	cairo_scale(cr,1,-1);
	draw_y_label(plot,cr,i);
	cairo_restore(cr);
	}
//Draw y ticks and grid lines
	for(int i=1;i<=(int)plot->y_range;i++)
	{
	float tick_height=(int)(plot->bottom_margin+plot->y_scale*i)+0.5;
	cairo_move_to(cr,plot->left_margin-tick_length,tick_height);
	cairo_line_to(cr,plot->left_margin,tick_height);
	cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
	cairo_stroke(cr);
	
	cairo_move_to(cr,plot->left_margin,tick_height);
	cairo_line_to(cr,plot->width-plot->right_margin,tick_height);
	cairo_set_source_rgba(cr,0.5,0.5,0.5,1.0);
	cairo_stroke(cr);
	}

//Draw x labels
int tick_start=(int)ceil(plot->x_start/plot->x_tick_spacing);
int tick_end=(int)floor(plot->x_start/plot->x_tick_spacing+plot->x_range);
cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
	for(int i=tick_start;i<=(int)plot->x_range+tick_end;i++)
	{
	cairo_save(cr);
	cairo_translate(cr,plot->left_margin+(int)((i-plot->x_start/plot->x_tick_spacing)*plot->x_scale),plot->bottom_margin);
	cairo_scale(cr,1,-1);
	
	char label[256];
	sprintf(label,"%.0f",i*plot->x_tick_spacing);
	cairo_text_extents_t extents;
	cairo_text_extents(cr,label,&extents);	
	
	cairo_move_to(cr,(int)(-extents.x_bearing-extents.width/2),(int)(-extents.y_bearing+tick_length+tick_label_spacing));
	sprintf(label,"%.0f%s",i*plot->x_tick_spacing,units[plot_units[plot->x_axis_variable]]);
	cairo_show_text(cr,label);
	cairo_fill(cr);	
	cairo_restore(cr);
	}
//Draw x ticks and grid lines
	for(int i=tick_start;i<=(int)plot->x_range+tick_end;i++)
	{
		//Don't draw line for 0 because it coincides with the y axis
		if(i==0)continue;
	float tick_x=(int)(plot->left_margin+(i-plot->x_start/plot->x_tick_spacing)*plot->x_scale)+0.5;
	cairo_move_to(cr,tick_x,plot->bottom_margin-tick_length);
	cairo_line_to(cr,tick_x,plot->bottom_margin);
	cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
	cairo_stroke(cr);

	cairo_move_to(cr,tick_x,plot->bottom_margin);
	cairo_line_to(cr,tick_x,plot->height-plot->top_margin);
	cairo_set_source_rgba(cr,0.5,0.5,0.5,1.0);
	cairo_stroke(cr);
	}
//Draw axes
cairo_move_to(cr,plot->left_margin+0.5,plot->height-plot->top_margin+0.5);
cairo_line_to(cr,plot->left_margin+0.5,plot->bottom_margin+0.5);
cairo_line_to(cr,plot->width-plot->right_margin+0.5,plot->bottom_margin+0.5);
cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
cairo_stroke(cr);
cairo_move_to(cr,plot->width-plot->right_margin+0.5,plot->bottom_margin+0.5);
cairo_line_to(cr,plot->width-plot->right_margin+0.5,plot->height-plot->top_margin+0.5);
cairo_line_to(cr,plot->left_margin+0.5,plot->height-plot->top_margin+0.5);
cairo_set_source_rgba(cr,0.5,0.5,0.5,1.0);
cairo_stroke(cr);



//Plot data
	for(int i=0;i<PLOT_NUM;i++)
	{
	if(plot->active_plots&(1<<i))plot_data(plot,cr,plot_functions[plot->x_axis_variable],plot_functions[i],plot->y_tick_spacing[plot_units[i]],plot_colors[i][0],plot_colors[i][1],plot_colors[i][2]);
	}

float values[PLOT_NUM];
draw_cursor(plot,cr,plot->cursor_x,values);

	if(plot->cursor_range>0.0)
	{
	//Calculate range quantities
	
	float values2[PLOT_NUM];
	draw_cursor(plot,cr,plot->cursor_x+plot->cursor_range,values2);
	float display_values[PLOT_NUM];

	//Calculate start and end time
	int l1,r1,l2,r2;
	float u1,u2;
	log_get_point_by_value(plot->log,plot_functions[plot->x_axis_variable],plot->cursor_x,&l1,&r1,&u1);
	log_get_point_by_value(plot->log,plot_functions[plot->x_axis_variable],plot->cursor_x+plot->cursor_range,&l2,&r2,&u2);
	float start_time=(1.0-u1)*log_get_time(plot->log,l1)+u1*log_get_time(plot->log,r1);
	float end_time=(1.0-u2)*log_get_time(plot->log,l2)+u2*log_get_time(plot->log,r2);

	//Position and distance display difference, not average
	display_values[0]=values2[0]-values[0];
	display_values[1]=values[1]-values2[1];
	display_values[2]=values2[2]-values[2];
	//For horizontal and vertical acceleration, use finite difference over entire range
	display_values[6]=(values[3]-values2[3])/(end_time-start_time);	
	display_values[7]=(values[4]-values2[4])/(end_time-start_time);
	
	//For other quantities use the trapezium rule to compute the average
	int averaged_quantities[]={3,4,5,8,9};	
		for(int i=0;i<5;i++)
		{
		int index=averaged_quantities[i];
			if(l1==l2)
			{
			display_values[index]=0.5*(values[index]+values2[index]);
			}
			else
			{
			display_values[index]=0.5*(values[index]+plot_functions[index](plot->log,r1))*(log_get_time(plot->log,r1)-start_time);
			display_values[index]+=0.5*(values2[index]+plot_functions[index](plot->log,l2))*(end_time-log_get_time(plot->log,l2));
				for(int j=r1;j<l2;j++)
				{
				display_values[index]+=0.5*(plot_functions[index](plot->log,j)+plot_functions[index](plot->log,j+1))*(log_get_time(plot->log,j+1)-log_get_time(plot->log,j));
				}
			display_values[index]/=end_time-start_time;
			}
		}
	//Averages of L/D and glide ratios calculated from averaged L/D coefficients and velocities
	display_values[10]=display_values[8]/display_values[9];	
	display_values[11]=display_values[3]/display_values[4];	

	cairo_set_source_rgba(cr,0.6,0.6,0.6,0.5);
	cairo_rectangle(cr,plot->left_margin+plot->x_scale*(plot->cursor_x-plot->x_start)/plot->x_tick_spacing,plot->bottom_margin,plot->x_scale*plot->cursor_range/plot->x_tick_spacing,plot->height-plot->bottom_margin-plot->top_margin);
	cairo_fill(cr);
	draw_legend(plot,cr,plot_range_names,display_values);
	}
	else draw_legend(plot,cr,plot_names,values);



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

}



