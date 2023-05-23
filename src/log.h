#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

typedef struct
{
int points;
float* time;
float* distance;
float* latitude;
float* longitude;
float* altitude;
float* vel_n;
float* vel_e;
float* vel_d;
float wind_n;
float wind_e;
int takeoff;
int exit;
int deployment;
int landing;
}log_t;



int log_parse(const char* filename,log_t* log);


float log_get_time(log_t* log,int i);
float log_get_distance(log_t* log,int i);
float log_get_altitude(log_t* log,int i);
float log_get_vel_vert(log_t* log,int i);
float log_get_vel_horz(log_t* log,int i);
float log_get_vel_total(log_t* log,int i);
float log_get_acc_vert(log_t* log,int i);
float log_get_acc_horz(log_t* log,int i);
float log_get_g_force(log_t* log,int i);
void log_get_drag_lift_coefficient(log_t* log,int i,float* drag,float* lift);
float log_get_lift_coefficient(log_t* log,int i);
float log_get_drag_coefficient(log_t* log,int i);
float log_get_lift_drag_ratio(log_t* log,int i);
float log_get_glide_ratio(log_t* log,int i);
float log_get_heading(log_t* log,int i);
float log_get_point_by_value(log_t* log,float (*xaxis)(log_t*,int),float x,int* left,int* right,float* u);
float get_distance(float lat1,float lon1,float lat2,float lon2);


#endif
