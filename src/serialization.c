#include <jansson.h>
#include "serialization.h"
#include "log.h"
#include "timeline.h"

extern log_t cur_log;
extern timeline_t wind_timeline;
extern timeline_t polar_timeline;


json_t* serialize_timeline(timeline_t* timeline)
{
json_t* intervals=json_array();
	for(int i=0;i<timeline->intervals;i++)
	{
	json_t* interval=json_array();
	json_array_append_new(interval,json_integer(timeline->interval_points[2*i]));
	json_array_append_new(interval,json_integer(timeline->interval_points[2*i+1]));
	json_array_append_new(intervals,interval);
	}
return intervals;
}

void deserialize_timeline(json_t* json,timeline_t* timeline)
{
int num_intervals=json_array_size(json);
	if(num_intervals>MAX_INTERVALS)num_intervals=MAX_INTERVALS;

int i;
	for(i=0;i<num_intervals;i++)
	{
	json_t* interval=json_array_get(json,i);
		if(interval&&json_is_array(interval))
		{
		json_t* start=json_array_get(interval,0);
		json_t* end=json_array_get(interval,1);
			if(start&&json_is_integer(start)&&end&&json_is_integer(end))
			{
			timeline->interval_points[2*i]=json_integer_value(start);
			timeline->interval_points[2*i+1]=json_integer_value(end);
			}
			else break;
		}
		else break;
	}
timeline->intervals=i;
}


json_t* serialize_polar()
{
json_t* polar=json_object();

//json_t* coefficients=json_array();
//json_array_append_new(coefficients,json_real(polar_coefficients[0]));
//json_array_append_new(coefficients,json_real(polar_coefficients[0]));
json_object_set_new(polar,"intervals",serialize_timeline(&polar_timeline));
return polar;
}

void deserialize_polar(json_t* polar)
{
json_t* intervals=json_object_get(polar,"intervals");
	if(intervals&&json_is_array(intervals))deserialize_timeline(intervals,&polar_timeline);
}



json_t* serialize_wind()
{
json_t* wind=json_object();
json_t* velocity=json_array();
json_array_append_new(velocity,json_real(cur_log.wind_n));
json_array_append_new(velocity,json_real(cur_log.wind_e));
json_object_set_new(wind,"velocity",velocity);
json_object_set_new(wind,"intervals",serialize_timeline(&wind_timeline));
return wind;
}

void deserialize_wind(json_t* wind)
{
json_t* velocity=json_object_get(wind,"velocity");
	if(velocity&&json_is_array(velocity))
	{
	json_t* wind_n=json_array_get(velocity,0);
		if(wind_n&&json_is_real(wind_n))cur_log.wind_n=json_real_value(wind_n);
	json_t* wind_e=json_array_get(velocity,1);
		if(wind_e&&json_is_real(wind_e))cur_log.wind_e=json_real_value(wind_e);
	}
json_t* intervals=json_object_get(wind,"intervals");
	if(intervals&&json_is_array(intervals))deserialize_timeline(intervals,&wind_timeline);
}

json_t* serialize_data()
{
json_t* data=json_object();
json_object_set_new(data,"wind",serialize_wind());
json_object_set_new(data,"polar",serialize_polar());
return data;
}

void deserialize_data(json_t* data)
{
json_t* wind=json_object_get(data,"wind");
	if(wind&&json_is_object(wind))deserialize_wind(wind);
json_t* polar=json_object_get(data,"polar");
	if(polar&&json_is_object(polar))deserialize_polar(polar);
}


int load_data(const char* filename)
{
char path[512];
sprintf(path,"%s.json",filename);
json_error_t error;
json_t* data=json_load_file(path,0,&error);
	if(!data)return 1;
deserialize_data(data);
json_decref(data);
return 0;
}

int save_data(const char* filename)
{
char path[512];
sprintf(path,"%s.json",filename);
json_t* data=serialize_data();
int err=json_dump_file(data,path,JSON_INDENT(4));
json_decref(data);
return err;
}
