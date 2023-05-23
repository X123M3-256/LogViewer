#include<math.h>
#include "map.h"
#include "log.h"
#include "plot.h"

extern GtkWidget* plot_area;
extern plot_t plot;
extern log_t cur_log;


map_t maps[]={
{"lps",16384,0,-1.185079,-1.009292,51.495792,51.605093,65},
{"langar",16384,0,-0.989810,-0.814037,52.838665,52.944716,37},
};

struct
{
int map_index;
float cur_lat;
float cur_lon;
float scale;
float rotation;
float inclination;
}map_state;

gboolean map_plot_motion(GtkWidget *widget,GdkEvent *event,gpointer user_data)
{
gtk_widget_queue_draw(GTK_WIDGET(user_data));
}

/*
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
*/





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
	for(int i=0;i<NUM_MAPS;i++)
	{
	//TODO account for map distortion
	maps[i].pixels_per_meter=maps[i].image_width/get_distance(maps[i].lat_start,maps[i].lon_start,maps[i].lat_end,maps[i].lon_start);
		for(int j=2;j<5;j++)
		{
		char str[256];
		sprintf(str,"data/%s%d.png",maps[i].name,j);
	printf("Loading %s\n",maps[i].name);
		maps[i].images[j]=load_image(str);
			if(maps[i].images[j]==NULL)return 1;
		}
	maps[i].loaded=1;
	}

return 0;
}

int set_map(float lon,float lat)
{
printf("Searching maps for location %f %f\n",lon,lat);
map_state.map_index=-1;
	for(int i=0;i<NUM_MAPS;i++)
	{
	printf("Testing map  %s\n",maps[i].name);
	printf("Map  lon %f %f\n",maps[i].lon_start,maps[i].lon_end);
		if(lon>maps[i].lon_start&&lon<maps[i].lon_end&&lat>maps[i].lat_start&&lat<maps[i].lat_end)
		{
		map_state.map_index=i;
		break;
		}
	}
	
	if(map_state.map_index>=0)
	{
	map_state.scale=0.25;
	map_state.cur_lon=0.5*(maps[map_state.map_index].lon_start+maps[map_state.map_index].lon_end);
	map_state.cur_lat=0.5*(maps[map_state.map_index].lat_start+maps[map_state.map_index].lat_end);
	}
	else
	{
	printf("Unrecognized location; map disabled\n");
	}
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
map_t* map=maps+map_state.map_index;
float x1=map->image_width*map_state.scale*(lon-map->lon_start)/(map->lon_end-map->lon_start);
float y1=map->image_width*map_state.scale*(1.0-(lat-map->lat_start)/(map->lat_end-map->lat_start));
*x=cos(map_state.rotation)*x1-sin(map_state.rotation)*y1;
*y=cos(map_state.inclination)*(sin(map_state.rotation)*x1+cos(map_state.rotation)*y1)-map->pixels_per_meter*map_state.scale*sin(map_state.inclination)*(elevation-map->elevation);
}
/*
void map_get_lonlat(float x,float y,float* lon,float* lat)
{
*lon=map_state.lon_start+(x*(map_state.lon_end-map_state.lon_start))/5000.0;
*lat=map_state.lat_start+(y*(map_state.lat_end-map_state.lat_start))/5000.0;
}
*/

void map_draw_trace(cairo_t* cr,int start,int finish)
{
float* top_x=calloc(finish-start+1,sizeof(float));
float* top_y=calloc(finish-start+1,sizeof(float));
float* bottom_x=calloc(finish-start+1,sizeof(float));
float* bottom_y=calloc(finish-start+1,sizeof(float));
	for(int i=start;i<=finish;i++)
	{
	map_get_coords(cur_log.longitude[i],cur_log.latitude[i],cur_log.altitude[i],top_x+(i-start),top_y+(i-start));
	map_get_coords(cur_log.longitude[i],cur_log.latitude[i],maps[map_state.map_index].elevation,bottom_x+(i-start),bottom_y+(i-start));
	}

cairo_set_line_width(cr,1.0);
cairo_set_source_rgba(cr,0.2,0.2,0.2,1);
cairo_move_to(cr,bottom_x[0],bottom_y[0]);
	for(int i=1;i<=finish-start;i++)
	{
	cairo_line_to(cr,bottom_x[i],bottom_y[i]);
	}
cairo_stroke(cr);


//TODO use fixed time interval instead of fixed number of points
float dist=log_get_distance(&cur_log,start);
int i=start;
int next_i=0;

	while(i<finish)
	{
	//Get next index
	dist+=50.0;
	int l,r;
	float u;
	log_get_point_by_value(&cur_log,log_get_distance,dist,&l,&r,&u);
		if(u>0.5)next_i=r;	
		else next_i=l;
		if(next_i>finish)next_i=finish;
	//Draw shaded area
	cairo_set_source_rgba(cr,0.8,0.8,0.8,0.2);
	cairo_move_to(cr,top_x[i-start],top_y[i-start]);
		for(int j=i;j<=next_i;j++)cairo_line_to(cr,top_x[j-start],top_y[j-start]);
		for(int j=next_i;j>=i;j--)cairo_line_to(cr,bottom_x[j-start],bottom_y[j-start]);
	cairo_close_path(cr);
	cairo_fill(cr);

	//Draw vertical line
	cairo_set_source_rgba(cr,0.1,0.1,0.1,0.5);
	cairo_move_to(cr,top_x[i-start],top_y[i-start]);
	cairo_line_to(cr,bottom_x[i-start],bottom_y[i-start]);
	cairo_stroke(cr);
	
	i=next_i;
	}

cairo_set_line_width(cr,1.0);


//Draw run in
cairo_set_source_rgba(cr,0.5,0,1,1);
cairo_move_to(cr,top_x[0],top_y[0]);
	for(int i=1;i<=cur_log.exit-start;i++)
	{
	cairo_line_to(cr,top_x[i],top_y[i]);
	}
cairo_stroke(cr);
//Draw freefall
cairo_set_source_rgba(cr,0,1,0,1);
cairo_move_to(cr,top_x[cur_log.exit-start],top_y[cur_log.exit-start]);
	for(int i=cur_log.exit-start+1;i<=cur_log.deployment-start;i++)
	{
	cairo_line_to(cr,top_x[i],top_y[i]);
	}
cairo_stroke(cr);
//Draw canopy
cairo_set_source_rgba(cr,1,0,0,1);
cairo_move_to(cr,top_x[cur_log.deployment-start],top_y[cur_log.deployment-start]);
	for(int i=cur_log.deployment-start+1;i<=finish-start;i++)
	{
	cairo_line_to(cr,top_x[i],top_y[i]);
	}
cairo_stroke(cr);

free(top_x);
free(top_y);
free(bottom_x);
free(bottom_y);
}

gboolean map_draw(GtkWidget *widget,cairo_t *cr,gpointer data)
{
	if(map_state.map_index<0)return FALSE;
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

cairo_save(cr);
cairo_translate(cr,width/2,height/2);
float x,y;
map_get_coords(map_state.cur_lon,map_state.cur_lat,0,&x,&y);
cairo_translate(cr,-x,-y);


cairo_save(cr);
cairo_scale(cr,map_state.scale,map_state.scale);
cairo_scale(cr,1.0,cos(map_state.inclination));
cairo_rotate(cr,map_state.rotation);



int level=2;
	if(map_state.scale<0.075)level=4;
	else if(map_state.scale<0.15)level=3;
	else if(map_state.scale<0.3)level=2;
	//else if(map_state.scale<0.6)level=1;
cairo_scale(cr,(float)(1<<level),(float)(1<<level));

cairo_set_source_surface (cr,maps[map_state.map_index].images[level],0,0);
cairo_paint(cr);
cairo_restore(cr);

//Show last 500m of the run in
int left,right;
float u;
log_get_point_by_value(&cur_log,log_get_distance,-500.0,&left,&right,&u);
map_draw_trace(cr,left,cur_log.landing);


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
	cairo_fill(cr);
	}
cairo_restore(cr);

cairo_save(cr);
cairo_translate(cr,width-60,60);
int compass_radius=40;
int compass_inner_radius=32;
int compass_tick_length=4;
int compass_pointer_width=4;
int compass_pointer_height=27;

cairo_set_line_width(cr,1.0);
cairo_set_source_rgba(cr,0.8,0.8,0.8,1);
cairo_arc(cr,0,0,compass_radius,0,2*M_PI);
cairo_stroke(cr);
cairo_arc(cr,0,0,compass_inner_radius,0,2*M_PI);
cairo_stroke(cr);
	for(int i=0;i<36;i++)
	{
	float dir_x=sin(-map_state.rotation+M_PI*i/18.0);	
	float dir_y=cos(-map_state.rotation+M_PI*i/18.0);
	cairo_move_to(cr,compass_inner_radius*dir_x,compass_inner_radius*dir_y);
		if(i&1)cairo_line_to(cr,(compass_inner_radius+compass_tick_length)*dir_x,(compass_inner_radius+compass_tick_length)*dir_y);
		else cairo_line_to(cr,compass_radius*dir_x,compass_radius*dir_y);
	}
cairo_stroke(cr);

cairo_set_font_size(cr,12);
const char* labels[]={"N","E","S","W"};
	for(int i=0;i<4;i++)
	{
	cairo_save(cr);
	cairo_rotate(cr,map_state.rotation+0.5*i*M_PI);
	cairo_text_extents_t extents;
	cairo_text_extents(cr,labels[i],&extents);
	cairo_move_to(cr,-extents.x_bearing-extents.width/2,-20);
	cairo_show_text(cr,labels[i]);
	cairo_fill(cr);

	cairo_restore(cr);
	}


cairo_set_source_rgba(cr,0.9,0,0,1);
cairo_move_to(cr,-compass_pointer_width,0);
cairo_line_to(cr,compass_pointer_width,0);
cairo_line_to(cr,0,-compass_pointer_height);
cairo_close_path(cr);
cairo_fill(cr);
cairo_set_source_rgba(cr,0.3,0.3,0.3,1);
cairo_move_to(cr,-compass_pointer_width,0);
cairo_line_to(cr,compass_pointer_width,0);
cairo_line_to(cr,0,compass_pointer_height);
cairo_close_path(cr);
cairo_fill(cr);
cairo_restore(cr);

float wind=sqrt(cur_log.wind_n*cur_log.wind_n+cur_log.wind_e*cur_log.wind_e);
float bearing=atan2(-cur_log.wind_e,-cur_log.wind_n);
cairo_set_source_rgba(cr,0.9,0.9,0.9,1);
cairo_set_font_size(cr,13);
cairo_move_to(cr,30,30);
char str[256];
sprintf(str,"Wind %.1fm/s (%.0f mph) %.0f\u00B0",wind,wind*2.236936,(bearing>=0?0.0:360.0)+180*bearing/M_PI);
cairo_show_text(cr,str);


cairo_translate(cr,110,75);
cairo_set_source_rgba(cr,1.0,0.5,0.0,1);
cairo_rotate(cr,map_state.rotation+bearing);
float wind_scale=3.0;
cairo_move_to(cr,0,-wind_scale*wind);
cairo_line_to(cr,0,wind_scale*wind);
cairo_stroke(cr);
int arrow_width=5;
int arrow_height=8;

cairo_move_to(cr,0,wind_scale*wind);
cairo_line_to(cr,-arrow_width,wind_scale*wind);
cairo_line_to(cr,0,wind_scale*wind+arrow_height);
cairo_line_to(cr,arrow_width,wind_scale*wind);
cairo_close_path(cr);
cairo_fill(cr);

return FALSE;
}


gboolean map_button_press(GtkWidget *widget,GdkEvent *event,gpointer user_data)
{
map_drag_x=event->button.x;
map_drag_y=event->button.y;
	if(event->button.button==1)
	{
	map_drag_active=1;
	map_drag_lon=map_state.cur_lon;
	map_drag_lat=map_state.cur_lat;
	}
	if(event->button.button==3)
	{
	map_drag_active=2;
	map_drag_rot=map_state.rotation;
	map_drag_inc=map_state.inclination;
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
	map_t* map=maps+map_state.map_index;
	int x_motion=(event->motion.x-map_drag_x);
	int y_motion=(event->motion.y-map_drag_y)/cos(map_state.inclination);
	map_state.cur_lon=map_drag_lon-((cos(map_state.rotation)*x_motion+sin(map_state.rotation)*y_motion)*(map->lon_end-map->lon_start))/(map_state.scale*map->image_width);
	map_state.cur_lat=map_drag_lat+((-sin(map_state.rotation)*x_motion+cos(map_state.rotation)*y_motion)*(map->lat_end-map->lat_start))/(map_state.scale*map->image_width);
	gtk_widget_queue_draw(GTK_WIDGET(widget));
	}
	else if(map_drag_active==2)
	{
	map_state.rotation=map_drag_rot+0.005*(event->motion.x-map_drag_x);
	map_state.inclination=map_drag_inc+0.005*(event->motion.y-map_drag_y);
		if(map_state.inclination<0)map_state.inclination=0;
		else if(map_state.inclination>0.475*M_PI)map_state.inclination=0.475*M_PI;
	gtk_widget_queue_draw(GTK_WIDGET(widget));
	}
}

gboolean map_scroll(GtkWidget *widget,GdkEvent *event,gpointer user_data)
{
	if(event->scroll.direction==GDK_SCROLL_UP&&map_state.scale<0.9)map_state.scale*=2.0;
	else if(event->scroll.direction==GDK_SCROLL_DOWN)map_state.scale*=0.5;
gtk_widget_queue_draw(GTK_WIDGET(widget));
return TRUE;  
}


