#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

typedef struct
{
int points;
float* time;
float* latitude;
float* longitude;
float* altitude;
float* vel_n;
float* vel_e;
float* vel_d;
int exit;
int deployment;
int landing;
}log_t;



int log_parse(const char* filename,log_t* log);


float log_get_time(log_t* log,int i);
float log_get_altitude(log_t* log,int i);
float log_get_vel_vert(log_t* log,int i);
float log_get_vel_horz(log_t* log,int i);
float log_get_vel_total(log_t* log,int i);
float log_get_acc_vert(log_t* log,int i);
float log_get_acc_horz(log_t* log,int i);
float log_get_lift_coefficient(log_t* log,int i);
float log_get_drag_coefficient(log_t* log,int i);
float log_get_lift_drag_ratio(log_t* log,int i);
float log_get_glide_ratio(log_t* log,int i);


#endif
