#include<math.h>
#include<lapacke.h>
#include "polar.h"
#include "timeline.h"
#include "plot.h"

extern log_t cur_log;
timeline_t polar_timeline;

struct
{
float cursor;
int width;
int height;
int left_margin;
int right_margin;
int top_margin;
int bottom_margin;
float scale;
}polar_plot;


gboolean polar_motion(GtkWidget *widget,GdkEvent *event,gpointer user_data)
{
polar_plot.cursor=(event->motion.x-polar_plot.left_margin)/polar_plot.scale;
	if(event->motion.x<polar_plot.left_margin)polar_plot.cursor=0.0;
	else if(event->motion.x>polar_plot.width-polar_plot.right_margin)polar_plot.cursor=(polar_plot.width-polar_plot.right_margin-polar_plot.left_margin)/polar_plot.scale;
gtk_widget_queue_draw(GTK_WIDGET(widget));
}


void draw_data(cairo_t* cr,const char** names,float* values,int* value_units,int num_values)
{
cairo_set_font_size(cr,12);
cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);

//Calculate bounds
int line_width=0;
int line_padding=2;
int row_padding=2;
int row_height=18;
int row_text_y=4;
int value_offset=45;
int alternate_offset=120;

float label_width=0; 
int height=0;
	for(int i=0;i<num_values;i++)
	{
	cairo_text_extents_t extents;
	cairo_text_extents(cr,names[i],&extents);
		if(extents.x_advance>label_width)label_width=extents.x_advance;
	height+=row_height;
	}
label_width=floor(label_width);

int width=line_width+2*line_padding+(int)label_width+152;
//Draw rectangle
cairo_rectangle(cr,0.5,0.5,width,height);
cairo_set_source_rgba(cr,1.0,1.0,1.0,1.0);
cairo_fill(cr);
cairo_rectangle(cr,0.5,0.5,width,height);
cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
cairo_stroke(cr);

int unit_precision[5]={0,1,1,1,2};
float y_offset=0;

	for(int i=0;i<num_values;i++)
	{
	y_offset+=row_height;

	//cairo_move_to(cr,line_padding,y_offset-row_height/2+0.5);
	//cairo_line_to(cr,line_padding+line_width,y_offset-row_height/2+0.5);
	//cairo_set_source_rgba(cr,plot_colors[i][0],plot_colors[i][1],plot_colors[i][2],1.0);
	//cairo_stroke(cr);
	int x_offset=line_width+2*line_padding;
	//Draw label
	cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
	cairo_move_to(cr,x_offset,y_offset-row_text_y);
	cairo_show_text(cr,names[i]);
	cairo_fill(cr);
	x_offset+=label_width;
	
	char str[64];
	int unit=value_units[i];

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
	}

}



gboolean draw_polar_plot(GtkWidget *widget,cairo_t *cr,gpointer data)
{
guint width, height;
GdkRGBA color;
GtkStyleContext *context;

context=gtk_widget_get_style_context(widget);
polar_plot.width=gtk_widget_get_allocated_width(widget);
polar_plot.height=gtk_widget_get_allocated_height(widget);
gtk_render_background(context,cr,0,0,width,height);

cairo_set_line_width(cr,1);
cairo_set_source_rgba(cr,0,0,0,1.0);
cairo_move_to(cr,polar_plot.width-polar_plot.right_margin-0.5,polar_plot.height-polar_plot.bottom_margin-0.5);
cairo_line_to(cr,polar_plot.left_margin+0.5,polar_plot.height-polar_plot.bottom_margin-0.5);
cairo_line_to(cr,polar_plot.left_margin+0.5,polar_plot.top_margin+0.5);
cairo_stroke(cr);

cairo_set_source_rgba(cr,0.5,0.5,0.5,1.0);
cairo_move_to(cr,polar_plot.left_margin+0.5,polar_plot.top_margin+0.5);
cairo_line_to(cr,polar_plot.width-polar_plot.right_margin-0.5,polar_plot.top_margin+0.5);
cairo_line_to(cr,polar_plot.width-polar_plot.right_margin-0.5,polar_plot.height-polar_plot.bottom_margin-0.5);
cairo_stroke(cr);


	if(cur_log.points>0)
	{
	//Get bounds
	float x_range=0.0;
	float y_range=0.0;
		for(int i=0;i<polar_timeline.intervals;i++)
		for(int j=polar_timeline.interval_points[2*i];j<polar_timeline.interval_points[2*i+1];j++)
		{
		float drag,lift;
		log_get_drag_lift_coefficient(&cur_log,j,&drag,&lift);
		x_range=fmax(x_range,lift);
		y_range=fmax(y_range,drag);
		}
	polar_plot.scale=0.95*fmin((polar_plot.width-polar_plot.left_margin-polar_plot.right_margin)/x_range,(polar_plot.height-polar_plot.top_margin-polar_plot.bottom_margin)/y_range);

	//Draw grid lines (TODO ticks)
	int x_ticks=(int)(10*(polar_plot.width-polar_plot.left_margin-polar_plot.right_margin)/polar_plot.scale);
	int y_ticks=(int)(10*(polar_plot.height-polar_plot.top_margin-polar_plot.bottom_margin)/polar_plot.scale);
	cairo_set_source_rgba(cr,0.5,0.5,0.5,1.0);
		for(int i=1;i<=y_ticks;i++)
		{
		int pos=(int)(0.1*polar_plot.scale*i);
		cairo_move_to(cr,polar_plot.left_margin+0.5,polar_plot.height-pos-polar_plot.bottom_margin-0.5);
		cairo_line_to(cr,polar_plot.width-polar_plot.right_margin-0.5,polar_plot.height-pos-polar_plot.bottom_margin-0.5);
		}
		for(int i=1;i<=x_ticks;i++)
		{
		int pos=(int)(0.1*polar_plot.scale*i);
		cairo_move_to(cr,polar_plot.left_margin+pos+0.5,polar_plot.top_margin+0.5);
		cairo_line_to(cr,polar_plot.left_margin+pos+0.5,polar_plot.height-polar_plot.bottom_margin-0.5);
		}
	cairo_stroke(cr);
	float tick_length=5.0;
	float tick_label_spacing=2.0;
	cairo_set_source_rgba(cr,0,0,0,1.0);
	cairo_set_font_size(cr,12);
		for(int i=0;i<=y_ticks;i++)
		{
		int pos=(int)(0.1*polar_plot.scale*i);
		cairo_move_to(cr,polar_plot.left_margin-tick_length+0.5,polar_plot.height-pos-polar_plot.bottom_margin-0.5);
		cairo_line_to(cr,polar_plot.left_margin+0.5,polar_plot.height-pos-polar_plot.bottom_margin-0.5);
		cairo_stroke(cr);
		char label[256];
		sprintf(label,"%.1f",0.1*i);
		cairo_text_extents_t extents;
		cairo_text_extents(cr,label,&extents);		
		cairo_move_to(cr,(int)(polar_plot.left_margin-extents.x_bearing-extents.width-tick_length-tick_label_spacing),(int)(polar_plot.height-polar_plot.bottom_margin-pos-extents.y_bearing-extents.height/2));
		cairo_show_text(cr,label);
		cairo_fill(cr);	
		}
		for(int i=0;i<=x_ticks;i++)
		{
		int pos=(int)(0.1*polar_plot.scale*i);
		cairo_move_to(cr,polar_plot.left_margin+pos+0.5,polar_plot.height-polar_plot.bottom_margin+tick_length-0.5);
		cairo_line_to(cr,polar_plot.left_margin+pos+0.5,polar_plot.height-polar_plot.bottom_margin-0.5);
		cairo_stroke(cr);

		char label[256];
		sprintf(label,"%.1f",0.1*i);
		cairo_text_extents_t extents;
		cairo_text_extents(cr,label,&extents);		
		cairo_move_to(cr,(int)(polar_plot.left_margin+pos-extents.x_bearing-extents.width/2),(int)(polar_plot.height-polar_plot.bottom_margin-extents.y_bearing+tick_length+tick_label_spacing));
		cairo_show_text(cr,label);
		cairo_fill(cr);	
		}

	//Draw points
	cairo_set_source_rgba(cr,0.5,0.5,0.5,1.0);
		for(int j=0;j<polar_timeline.intervals;j++)
		for(int i=polar_timeline.interval_points[2*j];i<polar_timeline.interval_points[2*j+1];i++)
		{
		float drag,lift;
		log_get_drag_lift_coefficient(&cur_log,i,&drag,&lift);
		int x=polar_plot.left_margin+polar_plot.scale*lift;
		int y=polar_plot.height-polar_plot.bottom_margin-polar_plot.scale*drag;
		cairo_move_to(cr,x-2,y+0.5);
		cairo_line_to(cr,x+3,y+0.5);
		cairo_move_to(cr,x+0.5,y-2);
		cairo_line_to(cr,x+0.5,y+3);
		cairo_stroke(cr);
		}



	//Calculate regression line (TODO move this to separate routine)
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
	
		if(LAPACKE_sgesv(LAPACK_COL_MAJOR,m,1,A,m,ipiv,b,m)!=0)
		{
		printf("Least squares solution failed\n");
		}

	//Plot regression line
	cairo_set_source_rgba(cr,0,0,1.0,1.0);
		for(int i=0;i<=100;i++)
		{
		float lift=(polar_plot.width-polar_plot.left_margin-polar_plot.right_margin)*i/(polar_plot.scale*100.0);
		float drag=b[0]+b[2]*lift+b[1]*lift*lift;
		float x=polar_plot.left_margin+polar_plot.scale*lift;
		float y=polar_plot.height-polar_plot.bottom_margin-polar_plot.scale*drag;
		cairo_line_to(cr,x,y);
		}
	cairo_stroke(cr);

	cairo_save(cr);
	
	//Draw tangent
	float ld_max_lift=sqrt(b[0]/b[1]);
	float ld_max_drag=b[0]+b[2]*ld_max_lift+b[1]*ld_max_lift*ld_max_lift;
	cairo_set_source_rgba(cr,0,0,0,1.0);
	double dashes[]={10.0,5.0,1,5.0};
	cairo_set_dash (cr,dashes,4,0);
	cairo_move_to(cr,polar_plot.left_margin,polar_plot.height-polar_plot.bottom_margin);
	float mult=(polar_plot.width-polar_plot.left_margin-polar_plot.right_margin)/(polar_plot.scale*ld_max_lift);
	cairo_line_to(cr,polar_plot.left_margin+mult*polar_plot.scale*ld_max_lift,polar_plot.height-polar_plot.bottom_margin-mult*polar_plot.scale*ld_max_drag);
	cairo_stroke(cr);
	float ld_x=polar_plot.left_margin+polar_plot.scale*ld_max_lift;
	float ld_y=polar_plot.height-polar_plot.bottom_margin-polar_plot.scale*ld_max_drag;
	cairo_arc(cr,ld_x,ld_y,2,0,2*M_PI);
	cairo_fill(cr);	
	char str[64];
	sprintf(str,"L/D max %.2f",ld_max_lift/ld_max_drag);
	cairo_text_extents_t extents;
	cairo_set_font_size(cr,14);
	cairo_text_extents(cr,str,&extents);
	cairo_set_source_rgba(cr,1,1,1,1.0);
	cairo_rectangle(cr,ld_x+6+extents.x_bearing,ld_y+10+extents.y_bearing,extents.width,extents.height);
	cairo_fill(cr);
	cairo_move_to(cr,ld_x+6,ld_y+10);
	cairo_set_source_rgba(cr,0,0,0,1.0);
	cairo_show_text(cr,str);
	cairo_fill(cr);

	//Draw cursor
	double dashes2[]={5.0,5.0};
	cairo_set_dash (cr,dashes2,2,0);
	float lift=polar_plot.cursor;
	float drag=b[0]+b[2]*polar_plot.cursor+b[1]*polar_plot.cursor*polar_plot.cursor;

	float cursor_x=polar_plot.left_margin+polar_plot.scale*lift;
	float cursor_y=polar_plot.height-polar_plot.bottom_margin-polar_plot.scale*drag;

	cairo_move_to(cr,cursor_x,polar_plot.height-polar_plot.bottom_margin-0.5);
	cairo_line_to(cr,cursor_x,polar_plot.top_margin+0.5);
	cairo_move_to(cr,polar_plot.left_margin+0.5,cursor_y);
	cairo_line_to(cr,polar_plot.width-polar_plot.right_margin-0.5,cursor_y);
	cairo_stroke(cr);
	cairo_arc(cr,cursor_x,cursor_y,2.5,0,2*M_PI);
	cairo_fill(cr);	

	cairo_restore(cr);

	float mass=80;
	float area=0.5;
	float density=1.0;	

	float total=sqrt(lift*lift+drag*drag);	
	float vel=sqrt(9.81*mass/(0.5*density*area*total));
	float hvel=vel*lift/total;
	float vvel=vel*drag/total;

	const char* labels[]={"Lift coefficient","Drag Coefficient","L/D Ratio","Horizontal Velocity","Vertical Velocity","Total Velocity"};
	int units[]={4,4,4,1,1,1};
	float values[]={lift,drag,lift/drag,hvel,vvel,vel};
	cairo_translate(cr,polar_plot.left_margin+20,polar_plot.top_margin+20);
	draw_data(cr,labels,values,units,6);

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
polar_plot.left_margin=30;
polar_plot.right_margin=5;
polar_plot.top_margin=5;
polar_plot.bottom_margin=20;
}

void open_polar_window(GtkWidget *widget,gpointer data)
{
//g_return_if_fail(data != NULL);

GtkBuilder* builder=gtk_builder_new_from_file("polarwindow.glade");
GtkWidget* window=GTK_WIDGET(gtk_builder_get_object(builder,"window1"));
GtkWidget* timeline_plot=GTK_WIDGET(gtk_builder_get_object(builder,"timeline"));
GtkWidget* polar_plot=GTK_WIDGET(gtk_builder_get_object(builder,"polar_plot"));
gtk_widget_add_events(polar_plot,GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK);
gtk_widget_add_events(timeline_plot,GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK);
gtk_builder_connect_signals(builder,NULL);

timeline_connect_signals(&polar_timeline,timeline_plot);

//TODO this is a bit of a hack, look into a custom signal
g_signal_connect(G_OBJECT(timeline_plot),"motion-notify_event",G_CALLBACK(update_polar),polar_plot);

gtk_widget_show_all(window);
g_object_unref(G_OBJECT(builder));
}



