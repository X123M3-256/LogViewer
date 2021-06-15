#include<stdio.h>
#include<math.h>
#include<lapacke.h>
#include "wind.h"
#include "timeline.h"

float airspeeds[MAX_INTERVALS]={0};
timeline_t wind_timeline;



float diff_x(float* x,int interval,int index)
{
int i=wind_timeline.interval_points[2*interval]+index;
return cur_log.vel_e[i]-(x[0]);
}

float diff_y(float* x,int interval,int index)
{
int i=wind_timeline.interval_points[2*interval]+index;
return cur_log.vel_n[i]-(x[1]);
}

float norm(float x,float y)
{
return sqrt(x*x+y*y);
}


void compute_wind(float* wind_n,float* wind_e,float* airspeeds)
{
int m=2+wind_timeline.intervals;
int n=0;
int interval_row_start[MAX_INTERVALS];
int interval_rows[MAX_INTERVALS];


float* x=calloc(m,sizeof(float));
//Count total points and compute initial estimate based on zero wind assumption
	for(int j=0;j<wind_timeline.intervals;j++)
	{
		for(int i=wind_timeline.interval_points[2*j];i<wind_timeline.interval_points[2*j+1];i++)
		{
		x[2+j]+=norm(cur_log.vel_e[i],cur_log.vel_n[i]);
		}
	interval_rows[j]=wind_timeline.interval_points[2*j+1]-wind_timeline.interval_points[2*j];
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
		for(int i=0;i<wind_timeline.intervals;i++)
		for(int j=0;j<interval_rows[i];j++)
		{
		error[interval_row_start[i]+j]=norm(diff_x(x,i,j),diff_y(x,i,j))-x[2+i];
		}

	//Compute jacobian
		for(int i=0;i<wind_timeline.intervals;i++)
		for(int j=0;j<interval_rows[i];j++)
		{
		float dx=diff_x(x,i,j);
		float dy=diff_y(x,i,j);
		float dnorm=norm(dx,dy);
		int row_index=m*(interval_row_start[i]+j);
		float altitude=cur_log.altitude[wind_timeline.interval_points[i]+j]/1000.0;
		jacobian[row_index]=-dx/dnorm;
		jacobian[row_index+1]=-dy/dnorm;
			for(int k=0;k<wind_timeline.intervals;k++)
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
	for(int i=0;i<wind_timeline.intervals;i++)
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


			
		cairo_set_source_rgba(cr,1.0,0.0,0.0,1.0);
		for(int i=0;i<wind_timeline.intervals;i++)
		{	
		cairo_arc(cr,scale*cur_log.wind_e+width/2,scale*cur_log.wind_n+height/2,scale*airspeeds[i],0,2*M_PI);
		cairo_stroke(cr);
		}
		
	//Draw velocity points
	cairo_set_source_rgba(cr,0.5,0.5,0.5,1.0);
		for(int j=0;j<wind_timeline.intervals;j++)
		for(int i=wind_timeline.interval_points[2*j];i<wind_timeline.interval_points[2*j+1];i++)
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

void wind_init()
{
timeline_init(&wind_timeline,&cur_log,cur_log.takeoff,cur_log.landing);
}

//TODO this is a bit of a hack, look into a custom signal
gboolean update_wind(GtkWidget *widget,GdkEvent *event,gpointer data)
{
	if(wind_timeline.drag_active)
	{
	//Determine current wind speed and airspeeds for each interval
	compute_wind(&(cur_log.wind_n),&(cur_log.wind_e),airspeeds);
	//Redraw velocity plot
	gtk_widget_queue_draw(GTK_WIDGET(data));
	//TODO redraw main plot as well
	}
}


void open_wind_window(GtkWidget *widget,gpointer data)
{
//g_return_if_fail(data != NULL);

GtkBuilder* builder=gtk_builder_new_from_file("windwindow.glade");
GtkWidget* window=GTK_WIDGET(gtk_builder_get_object(builder,"window1"));
GtkWidget* velocity_plot=GTK_WIDGET(gtk_builder_get_object(builder,"velocity_plot"));
GtkWidget* timeline_plot=GTK_WIDGET(gtk_builder_get_object(builder,"timeline"));
timeline_connect_signals(&wind_timeline,timeline_plot);


gtk_widget_add_events(timeline_plot,GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK);
gtk_builder_connect_signals(builder,NULL);

//TODO this is a bit of a hack, look into a custom signal
g_signal_connect(G_OBJECT(timeline_plot),"motion-notify_event",G_CALLBACK(update_wind),velocity_plot);

gtk_widget_show_all(window);
g_object_unref(G_OBJECT(builder));

//Compute wind speed and direction (needed because airspeeds are not saved)
compute_wind(&(cur_log.wind_n),&(cur_log.wind_e),airspeeds);
}


