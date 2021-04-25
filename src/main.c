#include <gtk/gtk.h>
#include "log.h"
#include "plot.h"

plot_t plot; 
log_t log; 

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
		if(log_parse(filename,&log))
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
	plot_new(&plot,&log);
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

plot_draw(&plot,cr,width,height);
return FALSE;
}

void altitude_toggled(GtkCheckMenuItem* menu_item,gpointer data)
{
	if(gtk_check_menu_item_get_active(menu_item))plot.active_plots|=PLOT_ALTITUDE;
	else plot.active_plots&=~PLOT_ALTITUDE;
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

gboolean update_cursor(GtkWidget *widget,GdkEvent *event,gpointer user_data)
{
plot.cursor_x=event->motion.x;
gtk_widget_queue_draw(GTK_WIDGET(widget));
}

int main(int argc,char **argv)
{
GtkBuilder *builder = NULL;

log.points=0;
	if(argc>1)
	{
		if(log_parse(argv[1],&log))
		{
		printf("Could not read log file %s\n",argv[1]);
		return 1;
		}
	plot_new(&plot,&log);
	}

gtk_init(&argc,&argv);

builder=gtk_builder_new();

	if(gtk_builder_add_from_file(builder,"mainwindow.glade",NULL)==0)
	{
	printf("gtk_builder_add_from_file FAILED\n");
	return 0;
	}

GtkWidget* window=GTK_WIDGET(gtk_builder_get_object(builder,"window1"));
GtkWidget* plot_area=GTK_WIDGET(gtk_builder_get_object(builder,"plot_area"));
gtk_widget_add_events(plot_area,GDK_POINTER_MOTION_MASK);
gtk_builder_connect_signals(builder,NULL);

gtk_widget_show_all(window);
gtk_main();
return 0;
}
