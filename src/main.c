#include<gtk/gtk.h>
#include<math.h>
#include "log.h"
#include "plot.h"
#include "polar.h"
#include "map.h"
#include "wind.h"
#include "serialization.h"

//TODO making this global is ugly hack
GtkWidget* plot_area;
GtkWidget* map_area;

plot_t plot; 
log_t cur_log; 
char cur_filename[512];


int load_log(const char* filename)
{
	if(cur_log.points>0)
	{
		if(save_data(cur_filename))printf("Failed saving metadata for %s\n",cur_filename);
	}
	if(log_parse(filename,&cur_log))return 1;
strcpy(cur_filename,filename);
plot_new(&plot,&cur_log);
polar_init();
wind_init();
load_data(filename);
return 0;
}

void quit_clicked(GtkWidget *widget,gpointer data)
{
gtk_main_quit();
}


gboolean window_delete(GtkWidget *widget,GdkEvent *event,gpointer data)
{
gtk_main_quit();
return FALSE;
}

void open_clicked(GtkWidget *widget,gpointer data)
{
GtkWidget* dialog=gtk_file_chooser_dialog_new("Import Log File",GTK_WINDOW(data),GTK_FILE_CHOOSER_ACTION_OPEN,"_Cancel",GTK_RESPONSE_CANCEL,"_Import",GTK_RESPONSE_ACCEPT,NULL);

gint res=gtk_dialog_run(GTK_DIALOG(dialog));
	if(res==GTK_RESPONSE_ACCEPT)
	{
	char *filename;
	GtkFileChooser* chooser=GTK_FILE_CHOOSER(dialog);
    	filename=gtk_file_chooser_get_filename(chooser);
		if(load_log(filename))
		{
		gtk_widget_destroy (dialog);
		char error_text[512];
		sprintf(error_text,"Could not read log file %.400s",filename);
		GtkWidget* error_dialog=gtk_message_dialog_new(GTK_WINDOW(data),GTK_DIALOG_DESTROY_WITH_PARENT,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,error_text);
		gtk_window_set_title(GTK_WINDOW(error_dialog),"Error");
		gtk_dialog_run(GTK_DIALOG(error_dialog));
		gtk_widget_destroy(error_dialog);
		return;
		};
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
	if(cur_log.points>0)
	{
	plot_set_size(&plot,width,height);//TODO only call when necessary
	plot_draw(&plot,cr);
	}
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

void map_toggled(GtkCheckMenuItem* menu_item,gpointer data)
{
	if(gtk_check_menu_item_get_active(menu_item))gtk_widget_show(map_area);
	else gtk_widget_hide(map_area);
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



int main(int argc,char **argv)
{
cur_log.points=0;
	if(argc>1)
	{
		if(load_log(argv[1]))
		{
		printf("Could not read log file %s\n",argv[1]);
		return 1;
		}
	}

gtk_init(&argc,&argv);

GtkBuilder* builder=gtk_builder_new();

	if(gtk_builder_add_from_file(builder,"mainwindow.glade",NULL)==0)
	{
	printf("gtk_builder_add_from_file FAILED\n");
	return 0;
	}


GtkWidget* window=GTK_WIDGET(gtk_builder_get_object(builder,"window1"));
//gtk_window_fullscreen(GTK_WINDOW(window));
plot_area=GTK_WIDGET(gtk_builder_get_object(builder,"plot_area"));
map_area=GTK_WIDGET(gtk_builder_get_object(builder,"map"));
gtk_widget_add_events(map_area,GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|GDK_SCROLL_MASK);
gtk_widget_add_events(plot_area,GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK);
gtk_builder_connect_signals(builder,NULL);

	if(load_map())
	{
	printf("Failed loading map\n");
	}

gtk_widget_show_all(window);

gtk_main();
	

	if(cur_log.points>0)
	{
		if(save_data(cur_filename))printf("Failed saving metadata for %s\n",cur_filename);
		else printf("Metadata saved\n");
	}
return 0;
}
