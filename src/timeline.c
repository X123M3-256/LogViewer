#include <math.h>
#include "timeline.h"

float log_get_absolute_time(log_t* log,int i)
{
return 0.01*(log->time[i]);
}

int timeline_get_point(timeline_t* timeline,int x)
{
int l,r;
float u;
log_get_point_by_value(timeline->log,log_get_absolute_time,x/timeline->x_scale+log_get_absolute_time(timeline->log,timeline->start),&l,&r,&u);
	if(u>0.5)return r;
return l;
}

gboolean timeline_button_press(GtkWidget *widget,GdkEvent  *event,gpointer data)
{
timeline_t* timeline=(timeline_t*)data;
timeline->drag_active=1;
timeline->drag_target=-1;
	for(int i=0;i<2*timeline->intervals;i++)
	{
		if(fabs((log_get_absolute_time(timeline->log,timeline->interval_points[i])-log_get_absolute_time(timeline->log,timeline->start))*timeline->x_scale-event->motion.x)<5)timeline->drag_target=i;
	}
	if(timeline->drag_target==-1)
	{
		//Maximum number of intervals already reached
		if(timeline->intervals==MAX_INTERVALS)return FALSE;
	int point=timeline_get_point(timeline,event->motion.x);
	//Find position to insert interval
	int insert_before=-1;
		if(timeline->intervals==0)insert_before=0;
		else if(point<timeline->interval_points[0])insert_before=0;
		else if(point>timeline->interval_points[2*timeline->intervals-1])insert_before=timeline->intervals;
		else for(int i=0;i<timeline->intervals-1;i++)
		{
			if(timeline->interval_points[2*i+1]<point&&timeline->interval_points[2*(i+1)]>point)
			{
			insert_before=i+1;
			break;
			}
		}
		//Point is inside existing interval TODO consider allowing to split an existing interval
		if(insert_before==-1)return FALSE;
	memmove(timeline->interval_points+2*(insert_before+1),timeline->interval_points+2*insert_before,2*(timeline->intervals-insert_before)*sizeof(int));	
	timeline->interval_points[2*insert_before]=point;
	timeline->interval_points[2*insert_before+1]=point+1;
	timeline->drag_target=2*insert_before+1;
	timeline->intervals++;
	}

gtk_widget_queue_draw(widget);
}

gboolean timeline_button_release(GtkWidget *widget,GdkEvent  *event,gpointer data)
{
timeline_t* timeline=(timeline_t*)data;
timeline->drag_active=0;
//Remove redundant intervals
	for(int i=0;i<timeline->intervals;i++)
	{
		//Check for zero length interval
		if(timeline->interval_points[2*i]==timeline->interval_points[2*i+1])
		{
		memmove(timeline->interval_points+2*i,timeline->interval_points+2*(i+1),2*(timeline->intervals-i-1)*sizeof(int));
		timeline->intervals--;
		break;
		}
		//Check if interval starts at the end of the next interval
		else if(i<timeline->intervals-1&&timeline->interval_points[2*i+1]==timeline->interval_points[2*(i+1)])
		{
		timeline->interval_points[2*(i+1)]=timeline->interval_points[2*i];
		memmove(timeline->interval_points+2*i,timeline->interval_points+2*(i+1),2*(timeline->intervals-i-1)*sizeof(int));
		timeline->intervals--;
		break;
		}
	}
gtk_widget_queue_draw(widget);
}

gboolean timeline_motion(GtkWidget *widget,GdkEvent *event,gpointer data)
{
timeline_t* timeline=(timeline_t*)data;
	
	if(timeline->drag_active)
	{
	int point=timeline_get_point(timeline,event->motion.x);
		if(timeline->drag_target!=-1)
		{
		timeline->interval_points[timeline->drag_target]=point;
			if(timeline->drag_target>0&&point<timeline->interval_points[timeline->drag_target-1])timeline->interval_points[timeline->drag_target]=timeline->interval_points[timeline->drag_target-1];
			if(timeline->drag_target<2*timeline->intervals-1&&point>timeline->interval_points[timeline->drag_target+1])timeline->interval_points[timeline->drag_target]=timeline->interval_points[timeline->drag_target+1];
		}
	}
gtk_widget_queue_draw(GTK_WIDGET(widget));
}

void timeline_plot(timeline_t* timeline,cairo_t* cr,float height,float y_scale,float (*data)(log_t*,int),float r,float g,float b)
{
cairo_set_source_rgba(cr,r,g,b,1.0);
	for(int i=timeline->log->takeoff;i<=timeline->log->landing;i++)
	{
	float x=0.01*timeline->x_scale*(timeline->log->time[i]-timeline->log->time[timeline->start]);
	float y=height-y_scale*data(timeline->log,i);
		if(i==0)cairo_move_to(cr,x,y);
		else cairo_line_to(cr,x,y);
	}
cairo_stroke(cr);
}

gboolean timeline_draw(GtkWidget *widget,cairo_t *cr,gpointer data)
{
guint width, height;
GdkRGBA color;
GtkStyleContext *context;

context=gtk_widget_get_style_context(widget);
width=gtk_widget_get_allocated_width(widget);
height=gtk_widget_get_allocated_height(widget);
gtk_render_background(context,cr,0,0,width,height);
cairo_set_line_width(cr,1.0);

timeline_t* timeline=(timeline_t*)data;


	if(timeline->log->points>0)
	{
	//Get bounds
	float x_range=(0.01*(timeline->log->time[timeline->end]-timeline->log->time[timeline->start]));	
	float y_range=0.0;
	float v_range=0.0;
		for(int i=timeline->start;i<timeline->end;i++)
		{
		y_range=fmax(y_range,fabs(log_get_altitude(timeline->log,i)));
		v_range=fmax(v_range,fabs(log_get_vel_horz(timeline->log,i)));
		}
	
	float x_scale=width/x_range;
	timeline->x_scale=x_scale;
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
	timeline_plot(timeline,cr,height,0.5*height/M_PI,log_get_heading,0.0,0.5,0.5);	
	timeline_plot(timeline,cr,height,v_scale,log_get_vel_horz,1.0,0.0,0.0);	
	timeline_plot(timeline,cr,height,y_scale,log_get_altitude,0.0,0.0,0.0);	

		for(int i=0;i<timeline->intervals;i++)
		{
		float x_start=(int)(0.01*x_scale*(timeline->log->time[timeline->interval_points[2*i]]-timeline->log->time[timeline->start]))+0.5;
		float x_end=(int)(0.01*x_scale*(timeline->log->time[timeline->interval_points[2*i+1]]-timeline->log->time[timeline->start]))+0.5;
		cairo_set_source_rgba(cr,0.0,0.0,0.0,0.25);
		cairo_rectangle(cr,x_start,0,0.01*x_scale*(timeline->log->time[timeline->interval_points[2*i+1]]-timeline->log->time[timeline->interval_points[2*i]]),height);
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

void timeline_init(timeline_t* timeline,log_t* log,int start,int end)
{
timeline->start=start;
timeline->end=end;
timeline->intervals=0;
timeline->drag_active=0;
timeline->drag_target=0;
timeline->log=log;
}
void timeline_connect_signals(timeline_t* timeline,GtkWidget* widget)
{
g_signal_connect(G_OBJECT(widget),"button-press-event",G_CALLBACK(timeline_button_press),timeline);
g_signal_connect(G_OBJECT(widget),"button-release-event",G_CALLBACK(timeline_button_release),timeline);
g_signal_connect(G_OBJECT(widget),"motion-notify_event",G_CALLBACK(timeline_motion),timeline);
g_signal_connect(G_OBJECT(widget),"draw",G_CALLBACK(timeline_draw),timeline);
}





