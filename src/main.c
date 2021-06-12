#include<gtk/gtk.h>
#include<math.h>
#include<lapacke.h>
#include "log.h"
#include "plot.h"

plot_t plot; 
log_t cur_log; 

void quit_clicked(GtkWidget *widget,gpointer data)
{
gtk_main_quit();
}


gboolean window_delete(GtkWidget *widget,GdkEvent *event,gpointer data)
{
gtk_main_quit();
return FALSE;
}

void new_clicked(GtkWidget *widget,gpointer data)
{
GtkWidget* dialog=gtk_file_chooser_dialog_new("Import Log File",GTK_WINDOW(data),GTK_FILE_CHOOSER_ACTION_OPEN,"_Cancel",GTK_RESPONSE_CANCEL,"_Import",GTK_RESPONSE_ACCEPT,NULL);

gint res=gtk_dialog_run(GTK_DIALOG(dialog));
	if(res==GTK_RESPONSE_ACCEPT)
	{
	char *filename;
	GtkFileChooser* chooser=GTK_FILE_CHOOSER(dialog);
    	filename=gtk_file_chooser_get_filename(chooser);
		if(log_parse(filename,&cur_log))
		{
		gtk_widget_destroy (dialog);
		char error_text[512];
		sprintf(error_text,"Could not read cur_log file %.400s",filename);
		GtkWidget* error_dialog=gtk_message_dialog_new(GTK_WINDOW(data),GTK_DIALOG_DESTROY_WITH_PARENT,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,error_text);
		gtk_window_set_title(GTK_WINDOW(error_dialog),"Error");
		gtk_dialog_run(GTK_DIALOG(error_dialog));
		gtk_widget_destroy(error_dialog);
		return;
		};
	plot_new(&plot,&cur_log);
	g_free(filename);
	}
gtk_widget_destroy (dialog);
}

gboolean draw_plot(GtkWidget *widget,cairo_t *cr,gpointer data)
{
guint width, height;
GdkRGBA color;
GtkStyleContext *context;

context=gtk_widget_get_style_context(widget);
width=gtk_widget_get_allocated_width(widget);
height=gtk_widget_get_allocated_height(widget);
gtk_render_background(context,cr,0,0,width,height);

plot_set_size(&plot,width,height);//TODO only call when necessary
plot_draw(&plot,cr);
return FALSE;
}

void altitude_toggled(GtkCheckMenuItem* menu_item,gpointer data)
{
	if(gtk_check_menu_item_get_active(menu_item))plot.active_plots|=PLOT_ALTITUDE;
	else plot.active_plots&=~PLOT_ALTITUDE;
plot_recalculate_range(&plot);
gtk_widget_queue_draw(GTK_WIDGET(data));
}

void distance_toggled(GtkCheckMenuItem* menu_item,gpointer data)
{
	if(gtk_check_menu_item_get_active(menu_item))plot.active_plots|=PLOT_DISTANCE;
	else plot.active_plots&=~PLOT_DISTANCE;
plot_recalculate_range(&plot);
gtk_widget_queue_draw(GTK_WIDGET(data));
}

void horizontal_velocity_toggled(GtkCheckMenuItem* menu_item,gpointer data)
{
	if(gtk_check_menu_item_get_active(menu_item))plot.active_plots|=PLOT_VEL_HORZ;
	else plot.active_plots&=~PLOT_VEL_HORZ;
plot_recalculate_range(&plot);
gtk_widget_queue_draw(GTK_WIDGET(data));
}

void vertical_velocity_toggled(GtkCheckMenuItem* menu_item,gpointer data)
{
	if(gtk_check_menu_item_get_active(menu_item))plot.active_plots|=PLOT_VEL_VERT;
	else plot.active_plots&=~PLOT_VEL_VERT;
plot_recalculate_range(&plot);
gtk_widget_queue_draw(GTK_WIDGET(data));
}

void total_velocity_toggled(GtkCheckMenuItem* menu_item,gpointer data)
{
	if(gtk_check_menu_item_get_active(menu_item))plot.active_plots|=PLOT_VEL_TOTAL;
	else plot.active_plots&=~PLOT_VEL_TOTAL;
plot_recalculate_range(&plot);
gtk_widget_queue_draw(GTK_WIDGET(data));
}

void horizontal_acceleration_toggled(GtkCheckMenuItem* menu_item,gpointer data)
{
	if(gtk_check_menu_item_get_active(menu_item))plot.active_plots|=PLOT_ACC_HORZ;
	else plot.active_plots&=~PLOT_ACC_HORZ;
plot_recalculate_range(&plot);
gtk_widget_queue_draw(GTK_WIDGET(data));
}

void vertical_acceleration_toggled(GtkCheckMenuItem* menu_item,gpointer data)
{
	if(gtk_check_menu_item_get_active(menu_item))plot.active_plots|=PLOT_ACC_VERT;
	else plot.active_plots&=~PLOT_ACC_VERT;
plot_recalculate_range(&plot);
gtk_widget_queue_draw(GTK_WIDGET(data));
}

void lift_coefficient_toggled(GtkCheckMenuItem* menu_item,gpointer data)
{
	if(gtk_check_menu_item_get_active(menu_item))plot.active_plots|=PLOT_LIFT;
	else plot.active_plots&=~PLOT_LIFT;
plot_recalculate_range(&plot);
gtk_widget_queue_draw(GTK_WIDGET(data));
}

void drag_coefficient_toggled(GtkCheckMenuItem* menu_item,gpointer data)
{
	if(gtk_check_menu_item_get_active(menu_item))plot.active_plots|=PLOT_DRAG;
	else plot.active_plots&=~PLOT_DRAG;
plot_recalculate_range(&plot);
gtk_widget_queue_draw(GTK_WIDGET(data));
}

void lift_drag_ratio_toggled(GtkCheckMenuItem* menu_item,gpointer data)
{
	if(gtk_check_menu_item_get_active(menu_item))plot.active_plots|=PLOT_LD;
	else plot.active_plots&=~PLOT_LD;
plot_recalculate_range(&plot);
gtk_widget_queue_draw(GTK_WIDGET(data));
}

void glide_ratio_toggled(GtkCheckMenuItem* menu_item,gpointer data)
{
	if(gtk_check_menu_item_get_active(menu_item))plot.active_plots|=PLOT_GR;
	else plot.active_plots&=~PLOT_GR;
plot_recalculate_range(&plot);
gtk_widget_queue_draw(GTK_WIDGET(data));
}

void axis_time(GtkMenuItem* menu_item,gpointer data)
{
plot.x_axis_variable=0;
plot_recalculate_range(&plot);
gtk_widget_queue_draw(GTK_WIDGET(data));
}

void axis_distance(GtkMenuItem* menu_item,gpointer data)
{
plot.x_axis_variable=2;
plot_recalculate_range(&plot);
gtk_widget_queue_draw(GTK_WIDGET(data));
}

int plot_drag_active=0;

gboolean plot_motion(GtkWidget *widget,GdkEvent *event,gpointer user_data)
{
float pos=plot.x_tick_spacing*(event->motion.x-plot.left_margin)/plot.x_scale;
	if(pos<0.0)pos=0.0;
	if(pos>plot.x_range*plot.x_tick_spacing)pos=plot.x_range*plot.x_tick_spacing;

	if(plot_drag_active==0)
	{
	plot.cursor_x=pos;
	}
	else
	{
	plot.cursor_range=pos-plot.cursor_x;
	}
gtk_widget_queue_draw(GTK_WIDGET(widget));
}

gboolean plot_button_press(GtkWidget *widget,GdkEvent *event,gpointer user_data)
{
plot_drag_active=1;
gtk_widget_queue_draw(GTK_WIDGET(widget));
}

gboolean plot_button_release(GtkWidget *widget,GdkEvent *event,gpointer user_data)
{
plot_drag_active=0;
plot.cursor_range=-1.0;
gtk_widget_queue_draw(GTK_WIDGET(widget));
}

#define MAX_INTERVALS 16


struct
{
int start;
int end;
int intervals;
int interval_points[2*MAX_INTERVALS];
int drag_active;
int drag_target;
float x_scale;
}timeline;



float diff_x(float* x,int interval,int index)
{
int i=timeline.interval_points[2*interval]+index;
return cur_log.vel_e[i]-(x[0]);
}

float diff_y(float* x,int interval,int index)
{
int i=timeline.interval_points[2*interval]+index;
return cur_log.vel_n[i]-(x[1]);
}

float norm(float x,float y)
{
return sqrt(x*x+y*y);
}

void compute_wind(float* wind_n,float* wind_e,float* airspeeds)
{
int m=2+timeline.intervals;
int n=0;
int interval_row_start[MAX_INTERVALS];
int interval_rows[MAX_INTERVALS];


float* x=calloc(m,sizeof(float));
//Count total points and compute initial estimate based on zero wind assumption
	for(int j=0;j<timeline.intervals;j++)
	{
		for(int i=timeline.interval_points[2*j];i<timeline.interval_points[2*j+1];i++)
		{
		x[2+j]+=norm(cur_log.vel_e[i],cur_log.vel_n[i]);
		}
	interval_rows[j]=timeline.interval_points[2*j+1]-timeline.interval_points[2*j];
	interval_row_start[j]=n;
	x[2+j]/=interval_rows[j];
	n+=interval_rows[j];
	}
	
	if(n==0)
	{
	*wind_e=0.0;
	*wind_n=0.0;
	return;
	}
	
//Carry out gauss-newton iteration
float* error=calloc(n,sizeof(float));
float* jacobian=calloc(m*n,sizeof(float));
//Matrix for linear solve
float* A=calloc(m*m,sizeof(float));
//Right hand side for linear solve
float* b=calloc(m,sizeof(float));
//Pivot array
int* ipiv=calloc(n,sizeof(int));

	for(int iter=0;iter<20;iter++)
	{
	//Compute error
		for(int i=0;i<timeline.intervals;i++)
		for(int j=0;j<interval_rows[i];j++)
		{
		error[interval_row_start[i]+j]=norm(diff_x(x,i,j),diff_y(x,i,j))-x[2+i];
		}

	//Compute jacobian
		for(int i=0;i<timeline.intervals;i++)
		for(int j=0;j<interval_rows[i];j++)
		{
		float dx=diff_x(x,i,j);
		float dy=diff_y(x,i,j);
		float dnorm=norm(dx,dy);
		int row_index=m*(interval_row_start[i]+j);
		float altitude=cur_log.altitude[timeline.interval_points[i]+j]/1000.0;
		jacobian[row_index]=-dx/dnorm;
		jacobian[row_index+1]=-dy/dnorm;
			for(int k=0;k<timeline.intervals;k++)
			{
			jacobian[row_index+2+k]=(i==k)?-1:0;
			}
		}

	//Set up linear system TODO consider replacing these with LAPACK routines
	memset(A,0,m*m*sizeof(float));
	memset(b,0,m*sizeof(float));
	
		for(int k=0;k<n;k++)
		for(int i=0;i<m;i++)
		{
			for(int j=0;j<m;j++)A[i+m*j]-=jacobian[i+m*k]*jacobian[j+m*k];
		b[i]+=error[k]*jacobian[i+m*k];	
		}
	
	//Solve linear system for increment
		if(LAPACKE_sgesv(LAPACK_COL_MAJOR,m,1,A,m,ipiv,b,m)!=0)
		{
		printf("Gauss-Newton iteration failed\n");
		break;
		}
		for(int i=0;i<m;i++)x[i]+=0.5*b[i];
	}

//Write result
	for(int i=0;i<timeline.intervals;i++)
	{
	airspeeds[i]=x[2+i];
	}

*wind_e=x[0];
*wind_n=x[1];

free(error);
free(jacobian);
free(A);
free(b);
free(ipiv);
free(x);
}


gboolean draw_velocity_plot(GtkWidget *widget,cairo_t *cr,gpointer data)
{
guint width, height;
GdkRGBA color;
GtkStyleContext *context;

context=gtk_widget_get_style_context(widget);
width=gtk_widget_get_allocated_width(widget);
height=gtk_widget_get_allocated_height(widget);
gtk_render_background(context,cr,0,0,width,height);

cairo_set_line_width(cr,1);
cairo_move_to(cr,0,height/2+0.5);
cairo_line_to(cr,width,height/2+0.5);
cairo_move_to(cr,width/2+0.5,0);
cairo_line_to(cr,width/2+0.5,height);
cairo_stroke(cr);

	if(cur_log.points>0)
	{
	//Get bounds
	float x_range=0.0;
	float y_range=0.0;
		for(int i=cur_log.takeoff;i<=cur_log.landing;i++)
		{
		x_range=fmax(x_range,fabs(cur_log.vel_e[i]));
		y_range=fmax(y_range,fabs(cur_log.vel_n[i]));
		}

	float scale=0.45*fmin(width/x_range,height/y_range);


	//Compute average altitude of each interval	
	float airspeeds[MAX_INTERVALS]={0};
	float altitude[MAX_INTERVALS]={0};
		for(int j=0;j<timeline.intervals;j++)
		for(int i=timeline.interval_points[2*j];i<timeline.interval_points[2*j+1];i++)
		{
		altitude[j]+=cur_log.altitude[i]/(timeline.interval_points[2*j+1]-timeline.interval_points[2*j]);
		}

	//Determine current wind speed and airspeeds for each interval
	compute_wind(&(cur_log.wind_n),&(cur_log.wind_e),airspeeds);

			
		cairo_set_source_rgba(cr,1.0,0.0,0.0,1.0);
		for(int i=0;i<timeline.intervals;i++)
		{	
		cairo_arc(cr,scale*cur_log.wind_e+width/2,scale*cur_log.wind_n+height/2,scale*airspeeds[i],0,2*M_PI);
		cairo_stroke(cr);
		}
		
	//Draw velocity points
	cairo_set_source_rgba(cr,0.5,0.5,0.5,1.0);
		for(int j=0;j<timeline.intervals;j++)
		for(int i=timeline.interval_points[2*j];i<timeline.interval_points[2*j+1];i++)
		{
		int x=scale*cur_log.vel_e[i]+width/2;
		int y=scale*cur_log.vel_n[i]+height/2;
		cairo_move_to(cr,x-2,y+0.5);
		cairo_line_to(cr,x+3,y+0.5);
		cairo_move_to(cr,x+0.5,y-2);
		cairo_line_to(cr,x+0.5,y+3);
		cairo_stroke(cr);
		}


	}



return FALSE;
}




float timeline_get_x(int i)
{
return 0.01*timeline.x_scale*(cur_log.time[i]-cur_log.time[cur_log.takeoff]);
}


void timeline_plot(cairo_t* cr,float height,float x_scale,float y_scale,float (*data)(log_t*,int),float r,float g,float b)
{
cairo_set_source_rgba(cr,r,g,b,1.0);
	for(int i=cur_log.takeoff;i<=cur_log.landing;i++)
	{
	float x=0.01*x_scale*(cur_log.time[i]-cur_log.time[cur_log.takeoff]);
	float y=height-y_scale*data(&cur_log,i);
		if(i==0)cairo_move_to(cr,x,y);
		else cairo_line_to(cr,x,y);
	}
cairo_stroke(cr);
}



gboolean draw_timeline(GtkWidget *widget,cairo_t *cr,gpointer data)
{
guint width, height;
GdkRGBA color;
GtkStyleContext *context;

context=gtk_widget_get_style_context(widget);
width=gtk_widget_get_allocated_width(widget);
height=gtk_widget_get_allocated_height(widget);
gtk_render_background(context,cr,0,0,width,height);
cairo_set_line_width(cr,1.0);


	if(cur_log.points>0)
	{
	//Get bounds
	float x_range=(0.01*(cur_log.time[cur_log.landing]-cur_log.time[cur_log.takeoff]));	
	float y_range=0.0;
	float v_range=0.0;
		for(int i=cur_log.takeoff;i<cur_log.landing;i++)
		{
		y_range=fmax(y_range,fabs(log_get_altitude(&cur_log,i)));
		v_range=fmax(v_range,fabs(log_get_vel_horz(&cur_log,i)));
		}
	
	float x_scale=width/x_range;
	timeline.x_scale=x_scale;
	float y_scale=(height/y_range);
	float v_scale=(height/v_range);
	cairo_set_source_rgba(cr,0.7,0.7,0.7,1.0);
	cairo_move_to(cr,0,0.5);
	cairo_line_to(cr,width,0.5);
	cairo_move_to(cr,0,height-0.5);
	cairo_line_to(cr,width,height-0.5);
		for(int i=0;i<=(int)(x_range/60);i++)
		{
		cairo_move_to(cr,(int)(60*x_scale*i)+0.5,0);
		cairo_line_to(cr,(int)(60*x_scale*i)+0.5,height);
		}
	cairo_stroke(cr);
	timeline_plot(cr,height,x_scale,0.5*height/M_PI,log_get_heading,0.0,0.5,0.5);	
	timeline_plot(cr,height,x_scale,v_scale,log_get_vel_horz,1.0,0.0,0.0);	
	timeline_plot(cr,height,x_scale,y_scale,log_get_altitude,0.0,0.0,0.0);	

		for(int i=0;i<timeline.intervals;i++)
		{
		float x_start=(int)(0.01*x_scale*(cur_log.time[timeline.interval_points[2*i]]-cur_log.time[cur_log.takeoff]))+0.5;
		float x_end=(int)(0.01*x_scale*(cur_log.time[timeline.interval_points[2*i+1]]-cur_log.time[cur_log.takeoff]))+0.5;
		cairo_set_source_rgba(cr,0.0,0.0,0.0,0.25);
		cairo_rectangle(cr,x_start,0,0.01*x_scale*(cur_log.time[timeline.interval_points[2*i+1]]-cur_log.time[timeline.interval_points[2*i]]),height);
		cairo_fill(cr);
		cairo_set_source_rgba(cr,0.5,0.5,0.5,1.0);
		cairo_move_to(cr,x_start,0);
		cairo_line_to(cr,x_start,height);
		cairo_move_to(cr,x_end,0);
		cairo_line_to(cr,x_end,height);
		cairo_stroke(cr);
		}
	}

return FALSE;
}

void open_wind_window(GtkWidget *widget,gpointer data)
{
//g_return_if_fail(data != NULL);

GtkBuilder* builder=gtk_builder_new_from_file("windwindow.glade");
GtkWidget* window=GTK_WIDGET(gtk_builder_get_object(builder,"window1"));
GtkWidget* timeline_plot=GTK_WIDGET(gtk_builder_get_object(builder,"timeline"));
gtk_widget_add_events(timeline_plot,GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK);
gtk_builder_connect_signals(builder,NULL);

timeline.start=cur_log.takeoff+500;	
timeline.end=cur_log.landing-3000;


gtk_widget_show_all(window);
g_object_unref(G_OBJECT(builder));
}

int timeline_get_point(int x)
{
int l=cur_log.takeoff;
int r=cur_log.landing;
int mid=(l+r)/2;
	while(mid!=l&&mid!=r)
	{
		if(timeline_get_x(mid)<x)l=mid;
		else r=mid;
	mid=(l+r)/2;
	}
return l;
}

gboolean timeline_button_press(GtkWidget *widget,GdkEvent  *event,gpointer data)
{
timeline.drag_active=1;
timeline.drag_target=-1;
	for(int i=0;i<2*timeline.intervals;i++)
	{
		if(fabs(timeline_get_x(timeline.interval_points[i])-event->motion.x)<5)timeline.drag_target=i;
	}
	if(timeline.drag_target==-1)
	{
		//Maximum number of intervals already reached
		if(timeline.intervals==MAX_INTERVALS)return FALSE;
	int point=timeline_get_point(event->motion.x);
	//Find position to insert interval
	int insert_before=-1;
		if(timeline.intervals==0)insert_before=0;
		else if(point<timeline.interval_points[0])insert_before=0;
		else if(point>timeline.interval_points[2*timeline.intervals-1])insert_before=timeline.intervals;
		else for(int i=0;i<timeline.intervals-1;i++)
		{
			if(timeline.interval_points[2*i+1]<point&&timeline.interval_points[2*(i+1)]>point)
			{
			insert_before=i+1;
			break;
			}
		}
		//Point is inside existing interval TODO consider allowing to split an existing interval
		if(insert_before==-1)return FALSE;
	memmove(timeline.interval_points+2*(insert_before+1),timeline.interval_points+2*insert_before,2*(timeline.intervals-insert_before)*sizeof(int));	
	timeline.interval_points[2*insert_before]=point;
	timeline.interval_points[2*insert_before+1]=point+1;
	timeline.drag_target=2*insert_before+1;
	timeline.intervals++;
	}

gtk_widget_queue_draw(widget);
}

gboolean timeline_button_release(GtkWidget *widget,GdkEvent  *event,gpointer data)
{
timeline.drag_active=0;
//Remove redundant intervals
	for(int i=0;i<timeline.intervals;i++)
	{
		//Check for zero length interval
		if(timeline.interval_points[2*i]==timeline.interval_points[2*i+1])
		{
		memmove(timeline.interval_points+2*i,timeline.interval_points+2*(i+1),2*(timeline.intervals-i-1)*sizeof(int));
		timeline.intervals--;
		break;
		}
		//Check if interval starts at the end of the next interval
		else if(i<timeline.intervals-1&&timeline.interval_points[2*i+1]==timeline.interval_points[2*(i+1)])
		{
		timeline.interval_points[2*(i+1)]=timeline.interval_points[2*i];
		memmove(timeline.interval_points+2*i,timeline.interval_points+2*(i+1),2*(timeline.intervals-i-1)*sizeof(int));
		timeline.intervals--;
		break;
		}
	}
gtk_widget_queue_draw(widget);
}

gboolean timeline_motion(GtkWidget *widget,GdkEvent *event,gpointer data)
{
	///Find nearest two points by binary search, then interpolate
	if(timeline.drag_active)
	{
	int point=timeline_get_point(event->motion.x);
		if(timeline.drag_target!=-1)
		{
		timeline.interval_points[timeline.drag_target]=point;
			if(timeline.drag_target>0&&point<timeline.interval_points[timeline.drag_target-1])timeline.interval_points[timeline.drag_target]=timeline.interval_points[timeline.drag_target-1];
			if(timeline.drag_target<2*timeline.intervals-1&&point>timeline.interval_points[timeline.drag_target+1])timeline.interval_points[timeline.drag_target]=timeline.interval_points[timeline.drag_target+1];
		}
	}
gtk_widget_queue_draw(GTK_WIDGET(widget));
//	if(timeline.drag_active)
gtk_widget_queue_draw(GTK_WIDGET(data));
}

void open_polar_window(GtkWidget *widget,gpointer data)
{
//g_return_if_fail(data != NULL);

GtkBuilder* builder=gtk_builder_new_from_file("polarwindow.glade");
GtkWidget* window=GTK_WIDGET(gtk_builder_get_object(builder,"window1"));
GtkWidget* timeline_plot=GTK_WIDGET(gtk_builder_get_object(builder,"timeline"));
gtk_widget_add_events(timeline_plot,GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK);
gtk_builder_connect_signals(builder,NULL);

timeline.start=cur_log.exit;	
timeline.end=cur_log.deployment;


gtk_widget_show_all(window);
g_object_unref(G_OBJECT(builder));
}

gboolean draw_polar_plot(GtkWidget *widget,cairo_t *cr,gpointer data)
{
guint width, height;
GdkRGBA color;
GtkStyleContext *context;

context=gtk_widget_get_style_context(widget);
width=gtk_widget_get_allocated_width(widget);
height=gtk_widget_get_allocated_height(widget);
gtk_render_background(context,cr,0,0,width,height);

cairo_set_line_width(cr,1);
cairo_move_to(cr,0,height/2+0.5);
cairo_line_to(cr,width,height/2+0.5);
cairo_move_to(cr,width/4+0.5,0);
cairo_line_to(cr,width/4+0.5,height);
cairo_stroke(cr);

	if(cur_log.points>0)
	{
	//Get bounds
	float x_range=1.0;
	float y_range=1.0;
		//for(int i=cur_log.takeoff;i<=cur_log.landing;i++)
		//{
		//x_range=fmax(x_range,fabs(cur_log.vel_e[i]));
		//y_range=fmax(y_range,fabs(cur_log.vel_n[i]));
		//}

	float scale=0.45*fmin(width/x_range,height/y_range);

	//Draw velocity points
	cairo_set_source_rgba(cr,0.5,0.5,0.5,1.0);
		for(int j=0;j<timeline.intervals;j++)
		for(int i=timeline.interval_points[2*j];i<timeline.interval_points[2*j+1];i++)
		{
		float drag,lift;
		log_get_drag_lift_coefficient(&cur_log,i,&drag,&lift);
		int x=scale*drag+width/4;
		int y=-scale*lift+height/2;
		cairo_move_to(cr,x-2,y+0.5);
		cairo_line_to(cr,x+3,y+0.5);
		cairo_move_to(cr,x+0.5,y-2);
		cairo_line_to(cr,x+0.5,y+3);
		cairo_stroke(cr);
		}


	}



return FALSE;
}


//TODO making this global is ugly hack
GtkWidget* plot_area;

gboolean map_plot_motion(GtkWidget *widget,GdkEvent *event,gpointer user_data)
{
gtk_widget_queue_draw(GTK_WIDGET(user_data));
}

void open_map_window(GtkWidget *widget,gpointer data)
{
//g_return_if_fail(data != NULL);

GtkBuilder* builder=gtk_builder_new_from_file("mapwindow.glade");
GtkWidget* window=GTK_WIDGET(gtk_builder_get_object(builder,"window1"));
GtkWidget* map=GTK_WIDGET(gtk_builder_get_object(builder,"map"));
gtk_widget_add_events(map,GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|GDK_SCROLL_MASK);
gtk_builder_connect_signals(builder,NULL);
//TODO ugly hack
g_signal_connect(G_OBJECT(plot_area),"motion-notify-event",G_CALLBACK(map_plot_motion),map);
gtk_widget_show_all(window);
g_object_unref(G_OBJECT(builder));
}

struct
{
float lat_start,lat_end;
float lon_start,lon_end;
float elevation;
int image_width;
cairo_surface_t* image[5];
float cur_lat;
float cur_lon;
float pixels_per_meter;
float scale;
float rotation;
float inclination;
}map;


cairo_surface_t* load_image(const char* file)
{
GdkPixbuf* pixbuf=gdk_pixbuf_new_from_file(file,NULL);
	if(!pixbuf)return NULL;
cairo_surface_t* image=gdk_cairo_surface_create_from_pixbuf(pixbuf,0,NULL);
	if(!image)return NULL;
g_object_unref(pixbuf);
return image;
}

int load_map()
{
map.image_width=16384;
map.lon_start=-1.185079;
map.lon_end=-1.009292;
map.lat_start=51.495792;
map.lat_end=51.605093;
map.elevation=65;

//TODO account for map distortion
map.pixels_per_meter=map.image_width/get_distance(map.lat_start,map.lon_start,map.lat_end,map.lon_start);

map.image[0]=load_image("data/lps0.png");
	if(map.image[0]==NULL)return 1;
map.image[1]=load_image("data/lps1.png");
	if(map.image[1]==NULL)return 1;
map.image[2]=load_image("data/lps2.png");
	if(map.image[2]==NULL)return 1;
map.image[3]=load_image("data/lps3.png");
	if(map.image[3]==NULL)return 1;
map.image[4]=load_image("data/lps4.png");
	if(map.image[4]==NULL)return 1;

map.cur_lon=0.5*(map.lon_start+map.lon_end);
map.cur_lat=0.5*(map.lat_start+map.lat_end);

map.scale=0.25;


return 0;
}

int map_drag_active=0;
int map_drag_x;
int map_drag_y;
float map_drag_lon;
float map_drag_lat;
float map_drag_rot;
float map_drag_inc;

void map_get_coords(float lon,float lat,float elevation,float* x,float* y)
{
float x1=map.image_width*map.scale*(lon-map.lon_start)/(map.lon_end-map.lon_start);
float y1=map.image_width*map.scale*(1.0-(lat-map.lat_start)/(map.lat_end-map.lat_start));
*x=cos(map.rotation)*x1-sin(map.rotation)*y1;
*y=cos(map.inclination)*(sin(map.rotation)*x1+cos(map.rotation)*y1)-map.pixels_per_meter*map.scale*sin(map.inclination)*(elevation-map.elevation);

}
/*
void map_get_lonlat(float x,float y,float* lon,float* lat)
{
*lon=map.lon_start+(x*(map.lon_end-map.lon_start))/5000.0;
*lat=map.lat_start+(y*(map.lat_end-map.lat_start))/5000.0;
}
*/

void map_draw_trace(cairo_t* cr,int start,int finish,float r,float g,float b)
{
float* top_x=calloc(finish-start+1,sizeof(float));
float* top_y=calloc(finish-start+1,sizeof(float));
float* bottom_x=calloc(finish-start+1,sizeof(float));
float* bottom_y=calloc(finish-start+1,sizeof(float));
	for(int i=start;i<=finish;i++)
	{
	map_get_coords(cur_log.longitude[i],cur_log.latitude[i],cur_log.altitude[i],top_x+(i-start),top_y+(i-start));
	map_get_coords(cur_log.longitude[i],cur_log.latitude[i],map.elevation,bottom_x+(i-start),bottom_y+(i-start));
	}

cairo_set_line_width(cr,1.0);
cairo_move_to(cr,top_x[0],top_y[0]);
	for(int i=1;i<=finish-start;i++)
	{
	cairo_line_to(cr,top_x[i],top_y[i]);
	}
cairo_set_source_rgba(cr,0.2,0.2,0.2,1);
cairo_stroke(cr);


//TODO use fixed time interval instead of fixed number of points

cairo_set_source_rgba(cr,0.8,0.8,0.8,0.2);
	for(int i=0;i<finish-start-9;i+=10)
	{
	cairo_move_to(cr,top_x[i],top_y[i]);
		for(int j=1;j<=10;j++)cairo_line_to(cr,top_x[i+j],top_y[i+j]);
		for(int j=10;j>=0;j--)cairo_line_to(cr,bottom_x[i+j],bottom_y[i+j]);
	cairo_close_path(cr);
	cairo_fill(cr);
	}

cairo_set_source_rgba(cr,0.1,0.1,0.1,0.5);
	for(int i=0;i<=finish-start;i+=10)
	{
	cairo_move_to(cr,top_x[i],top_y[i]);
	cairo_line_to(cr,bottom_x[i],bottom_y[i]);
	cairo_stroke(cr);
	}


cairo_set_line_width(cr,1.0);
cairo_move_to(cr,top_x[0],top_y[0]);
	for(int i=1;i<=finish-start;i++)
	{
	cairo_line_to(cr,top_x[i],top_y[i]);
	}
cairo_set_source_rgba(cr,r,g,b,1);
cairo_stroke(cr);

free(top_x);
free(top_y);
free(bottom_x);
free(bottom_y);
}

gboolean map_draw(GtkWidget *widget,cairo_t *cr,gpointer data)
{
guint width, height;
GdkRGBA color;
GtkStyleContext *context;

context=gtk_widget_get_style_context(widget);
width=gtk_widget_get_allocated_width(widget);
height=gtk_widget_get_allocated_height(widget);
gtk_render_background(context,cr,0,0,width,height);

cairo_rectangle(cr,0,0,width,height);
cairo_set_source_rgba(cr,0.1,0.6,1,1);
cairo_fill(cr);


cairo_translate(cr,width/2,height/2);
float x,y;
map_get_coords(map.cur_lon,map.cur_lat,0,&x,&y);
cairo_translate(cr,-x,-y);

cairo_save(cr);
cairo_scale(cr,map.scale,map.scale);
cairo_scale(cr,1.0,cos(map.inclination));
cairo_rotate(cr,map.rotation);



int level=0;
	if(map.scale<0.075)level=4;
	else if(map.scale<0.15)level=3;
	else if(map.scale<0.3)level=2;
	else if(map.scale<0.6)level=1;
cairo_scale(cr,(float)(1<<level),(float)(1<<level));

cairo_set_source_surface (cr,map.image[level],0,0);
cairo_paint(cr);
cairo_restore(cr);

map_draw_trace(cr,cur_log.deployment,cur_log.landing,1,0,0);
map_draw_trace(cr,cur_log.exit,cur_log.deployment,0,1,0);
map_draw_trace(cr,cur_log.exit-100,cur_log.exit,0.5,0,1);


int left,right;
float u;
log_get_point_by_value(&cur_log,plot_functions[plot.x_axis_variable],plot.cursor_x,&left,&right,&u);
float longitude=(1-u)*cur_log.longitude[left]+u*cur_log.longitude[right];
float latitude=(1-u)*cur_log.latitude[left]+u*cur_log.latitude[right];
float altitude=(1-u)*cur_log.altitude[left]+u*cur_log.altitude[right];
map_get_coords(longitude,latitude,altitude,&x,&y);
cairo_set_source_rgba(cr,0,0,0,1);
cairo_arc(cr,x,y,2.0,0,2*M_PI);
cairo_fill(cr);

	if(plot.cursor_range>0)
	{
	log_get_point_by_value(&cur_log,plot_functions[plot.x_axis_variable],plot.cursor_x+plot.cursor_range,&left,&right,&u);
	float longitude2=(1-u)*cur_log.longitude[left]+u*cur_log.longitude[right];
	float latitude2=(1-u)*cur_log.latitude[left]+u*cur_log.latitude[right];
	float altitude2=(1-u)*cur_log.altitude[left]+u*cur_log.altitude[right];
	//Draw measurement lines
	cairo_move_to(cr,x,y);
	map_get_coords(longitude,latitude,altitude2,&x,&y);
	cairo_line_to(cr,x,y);
	map_get_coords(longitude2,latitude2,altitude2,&x,&y);
	cairo_line_to(cr,x,y);
	double dashes[]={5.0,5.0};
	cairo_set_dash(cr,dashes,2,0);
	cairo_set_source_rgba(cr,1,1,1,1);
	cairo_stroke(cr);
	//Draw second point
	cairo_set_source_rgba(cr,0,0,0,1);
	cairo_arc(cr,x,y,2.0,0,2*M_PI);
	cairo_fill(cr);
	//Draw measurement text
	float dist=get_distance(latitude,longitude,latitude2,longitude2);
	cairo_text_extents_t extents;
	char str[64];
		if(dist>1000)sprintf(str,"%.2fkm",dist/1000.0);
		else sprintf(str,"%.0fm",dist);
	//cairo_text_extents(cr,str,&extents);
	map_get_coords(0.5*(longitude+longitude2),0.5*(latitude+latitude2),altitude2,&x,&y);
	cairo_move_to(cr,x,y);
	cairo_set_source_rgba(cr,1,1,1,1);
	cairo_show_text(cr,str);
	
		if(fabs(altitude-altitude2)>1000)sprintf(str,"%.2fkm",(altitude-altitude2)/1000.0);
		else sprintf(str,"%.0fm",altitude-altitude2);
	map_get_coords(longitude,latitude,0.5*(altitude+altitude2),&x,&y);
	//cairo_text_extents(cr,str,&extents);
	cairo_move_to(cr,x,y);
	cairo_set_source_rgba(cr,1,1,1,1);
	cairo_show_text(cr,str);
	}






return FALSE;
}


gboolean map_button_press(GtkWidget *widget,GdkEvent *event,gpointer user_data)
{
map_drag_x=event->button.x;
map_drag_y=event->button.y;
	if(event->button.button==1)
	{
	map_drag_active=1;
	map_drag_lon=map.cur_lon;
	map_drag_lat=map.cur_lat;
	}
	if(event->button.button==3)
	{
	map_drag_active=2;
	map_drag_rot=map.rotation;
	map_drag_inc=map.inclination;
	}
gtk_widget_queue_draw(GTK_WIDGET(widget));
}

gboolean map_button_release(GtkWidget *widget,GdkEvent *event,gpointer user_data)
{
map_drag_active=0;
gtk_widget_queue_draw(GTK_WIDGET(widget));
}

gboolean map_motion(GtkWidget *widget,GdkEvent *event,gpointer user_data)
{
	if(map_drag_active==1)
	{
	int x_motion=(event->motion.x-map_drag_x);
	int y_motion=(event->motion.y-map_drag_y)/cos(map.inclination);
	map.cur_lon=map_drag_lon-((cos(map.rotation)*x_motion+sin(map.rotation)*y_motion)*(map.lon_end-map.lon_start))/(map.scale*map.image_width);
	map.cur_lat=map_drag_lat+((-sin(map.rotation)*x_motion+cos(map.rotation)*y_motion)*(map.lat_end-map.lat_start))/(map.scale*map.image_width);
	gtk_widget_queue_draw(GTK_WIDGET(widget));
	}
	else if(map_drag_active==2)
	{
	map.rotation=map_drag_rot+0.005*(event->motion.x-map_drag_x);
	map.inclination=map_drag_inc+0.005*(event->motion.y-map_drag_y);
		if(map.inclination<0)map.inclination=0;
		else if(map.inclination>0.475*M_PI)map.inclination=0.475*M_PI;
	gtk_widget_queue_draw(GTK_WIDGET(widget));
	}
}

gboolean map_scroll(GtkWidget *widget,GdkEvent *event,gpointer user_data)
{
	if(event->scroll.direction==GDK_SCROLL_UP&&map.scale<0.9)map.scale*=2.0;
	else if(event->scroll.direction==GDK_SCROLL_DOWN)map.scale*=0.5;
gtk_widget_queue_draw(GTK_WIDGET(widget));
return TRUE;  
}

int main(int argc,char **argv)
{
cur_log.points=0;
	if(argc>1)
	{
		if(log_parse(argv[1],&cur_log))
		{
		printf("Could not read cur_log file %s\n",argv[1]);
		return 1;
		}
	plot_new(&plot,&cur_log);
	}
	else 
	{
	printf("Usage: LogViewer <filename>\n");
	return 1;
	}

gtk_init(&argc,&argv);

GtkBuilder* builder=gtk_builder_new();

	if(gtk_builder_add_from_file(builder,"mainwindow.glade",NULL)==0)
	{
	printf("gtk_builder_add_from_file FAILED\n");
	return 0;
	}

	if(load_map())
	{
	printf("Failed loading map\n");
	}


GtkWidget* window=GTK_WIDGET(gtk_builder_get_object(builder,"window1"));
//gtk_window_fullscreen(GTK_WINDOW(window));
plot_area=GTK_WIDGET(gtk_builder_get_object(builder,"plot_area"));
gtk_widget_add_events(plot_area,GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK);
gtk_builder_connect_signals(builder,NULL);

gtk_widget_show_all(window);

//open_map_window(window,NULL);

gtk_main();
return 0;
}
