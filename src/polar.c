#include<math.h>
#include<lapacke.h>
#include "polar.h"
#include "timeline.h"

extern log_t cur_log;
timeline_t polar_timeline;

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

	float A[9]={0,0,0,0,0,0,0,0,0};
	float b[3]={0,0,0};
	int ipiv[3];
	int m=2;	
		for(int i=0;i<polar_timeline.intervals;i++)
		for(int j=polar_timeline.interval_points[2*i];j<polar_timeline.interval_points[2*i+1];j++)
		{
		float drag,lift;
		log_get_drag_lift_coefficient(&cur_log,j,&drag,&lift);
		
		float phi_k[3];
		phi_k[0]=1.0;
		phi_k[1]=lift*lift;
		phi_k[2]=lift;
			for(int i=0;i<m;i++)
			{
				for(int j=0;j<m;j++)A[i+m*j]+=phi_k[i]*phi_k[j];
			b[i]+=phi_k[i]*drag;	
			}
		}
	
	//Solve linear system for increment
		if(LAPACKE_sgesv(LAPACK_COL_MAJOR,m,1,A,m,ipiv,b,m)!=0)
		{
		printf("Least squares solution failed\n");
		}

	cairo_set_source_rgba(cr,0,0,1.0,1.0);
		for(int i=0;i<=200;i++)
		{
		float lift=(i-100)/100.0;
		float drag=b[0]+b[2]*lift+b[1]*lift*lift;
		float x=scale*drag+width/4;
		float y=-scale*lift+height/2;
		cairo_line_to(cr,x,y);
		}
	cairo_stroke(cr);

	//Draw tangent
	float ld_max=sqrt(b[0]/b[1]);
	cairo_set_source_rgba(cr,0,0,0,1.0);
	cairo_move_to(cr,width/4,height/2);
	cairo_line_to(cr,5*scale*(b[0]+b[2]*ld_max+b[1]*ld_max*ld_max)+width/4,-5*scale*ld_max+height/2);
	cairo_stroke(cr);	


	printf("L/D max: %f.2\n",ld_max/(b[0]+b[2]*ld_max+b[1]*ld_max*ld_max));

	//Draw points
	cairo_set_source_rgba(cr,0.5,0.5,0.5,1.0);
		for(int j=0;j<polar_timeline.intervals;j++)
		for(int i=polar_timeline.interval_points[2*j];i<polar_timeline.interval_points[2*j+1];i++)
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

//TODO this is a bit of a hack, look into a custom signal
gboolean update_polar(GtkWidget *widget,GdkEvent *event,gpointer data)
{
	if(polar_timeline.drag_active)
	{
	//Redraw polar plot
	gtk_widget_queue_draw(GTK_WIDGET(data));
	}
return FALSE;
}


void polar_init()
{
timeline_init(&polar_timeline,&cur_log,cur_log.exit,cur_log.deployment);
}

void open_polar_window(GtkWidget *widget,gpointer data)
{
//g_return_if_fail(data != NULL);

GtkBuilder* builder=gtk_builder_new_from_file("polarwindow.glade");
GtkWidget* window=GTK_WIDGET(gtk_builder_get_object(builder,"window1"));
GtkWidget* timeline_plot=GTK_WIDGET(gtk_builder_get_object(builder,"timeline"));
GtkWidget* polar_plot=GTK_WIDGET(gtk_builder_get_object(builder,"polar_plot"));
gtk_widget_add_events(timeline_plot,GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK);
gtk_builder_connect_signals(builder,NULL);

timeline_connect_signals(&polar_timeline,timeline_plot);

//TODO this is a bit of a hack, look into a custom signal
g_signal_connect(G_OBJECT(timeline_plot),"motion-notify_event",G_CALLBACK(update_polar),polar_plot);

gtk_widget_show_all(window);
g_object_unref(G_OBJECT(builder));
}



