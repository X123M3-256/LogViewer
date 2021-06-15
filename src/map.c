#include<math.h>
#include "map.h"
#include "log.h"
#include "plot.h"

extern GtkWidget* plot_area;
extern plot_t plot;
extern log_t cur_log;

struct
{
int loaded;
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

gboolean map_plot_motion(GtkWidget *widget,GdkEvent *event,gpointer user_data)
{
gtk_widget_queue_draw(GTK_WIDGET(user_data));
}

void open_map_window(GtkWidget *widget,gpointer data)
{

	if((!map.loaded)&&load_map())
	{
	printf("Failed loading map\n");
	}

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


