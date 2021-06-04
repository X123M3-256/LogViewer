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

gtk_init(&argc,&argv);

GtkBuilder* builder=gtk_builder_new();

if(gtk_builder_add_from_file(builder,"mainwindow.glade",NULL)==0)
{
printf("gtk_builder_add_from_file FAILED\n");
return 0;
}

GtkWidget* window=GTK_WIDGET(gtk_builder_get_object(builder,"window1"));
GtkWidget* plot_area=GTK_WIDGET(gtk_builder_get_object(builder,"plot_area"));
gtk_widget_add_events(plot_area,GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK);
gtk_builder_connect_signals(builder,NULL);

gtk_widget_show_all(window);

gtk_main();
return 0;
}
