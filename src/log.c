#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "log.h"

void log_free(log_t* log)
{
free(log->latitude);
free(log->longitude);
free(log->altitude);
free(log->vel_n);
free(log->vel_e);
free(log->vel_d);
}

int log_parse(const char* filename,log_t* log)
{
FILE* file=fopen(filename,"r");
	if(!file)return 1;

char str[1024];

//Count lines in file
log->points=0;
	while(fgets(str,1024,file)!=NULL)log->points++;
//Don't count header lines
log->points-=2;
fseek(file,0,SEEK_SET);


log->time=calloc(log->points,sizeof(int));
log->latitude=calloc(log->points,sizeof(float));
log->longitude=calloc(log->points,sizeof(float));
log->distance=calloc(log->points,sizeof(float));
log->altitude=calloc(log->points,sizeof(float));
log->vel_n=calloc(log->points,sizeof(float));
log->vel_e=calloc(log->points,sizeof(float));
log->vel_d=calloc(log->points,sizeof(float));
log->wind_n=0.0;
log->wind_e=0.0;
//Read header line
fgets(str,1024,file);
fgets(str,1024,file);
//Read data points
int start_time=0;
	for(int i=0;i<log->points;i++)
	{
	int h,m,s,c;
		if(fscanf(file,"%*d-%*d-%*dT%d:%d:%d.%dZ,%f,%f,%f,%f,%f,%f,%*f,%*f,%*f,%*f,%*f,%*d,%*d\n",&h,&m,&s,&c,log->latitude+i,log->longitude+i,log->altitude+i,log->vel_n+i,log->vel_e+i,log->vel_d+i)!=10)
		{
		log_free(log);
		return 1;
		};
	int centiseconds=100*(s+60*m+3600*h)+c;
		if(i==0)start_time=centiseconds;
	//If the current timestamp is less than the current time, that means that midnight has passed. This will fail if the log lasts more than 24 hours, but that seems unlikely
	log->time[i]=centiseconds>start_time?centiseconds-start_time:centiseconds+24*360000;
	}

log->distance[0]=0.0;
	for(int i=1;i<log->points;i++)
	{
	float lat=0.5*M_PI*(log->latitude[i]+log->latitude[i-1])/180.0;
	float k1=111132.09-566.05*cos(2*lat)+1.2*cos(4*lat);
	float k2=111415.13*cos(lat)-94.55*cos(3*lat)+0.12*cos(5*lat);
	float dx=k1*(log->latitude[i]-log->latitude[i-1]);
	float dy=k2*(log->longitude[i]-log->longitude[i-1]);
	log->distance[i]=log->distance[i-1]+sqrt(dx*dx+dy*dy);
	}
		

//Find takeoff time
log->takeoff=0;
//Ignore the first hundred points of the log, they seem to be unreliable
		for(int i=100;i<log->points;i++)
		{
		//Look for where vertical velocity exceeds 15m/s - this should get us near the takeoff point
			if(0.333333*(log->vel_d[i-1]+log->vel_d[i]+log->vel_d[i+1])<-2)
			{
			printf("Takeoff at %d\n",i);
			//Search backwards for point where acceleration starts. Start out with fairly large interval to prevent random noise from terminating the search early, then use progressively smaller intervals to find exact moment of exit.
				for(int j=5;j>0;j--)
				{
					while(i>100&&(log->vel_d[i]-log->vel_d[i-j])/(0.01*(log->time[i]-log->time[i-j]))<-0.1)i--;
				}
			log->takeoff=i;
			break;
			}
		}

//Find exit time
int post_exit=0;
log->exit=-1;
		for(int i=log->takeoff;i<log->points;i++)
		{
		//Look for where vertical velocity exceeds 15m/s - this should get us near the exit point
			if(log->vel_d[i]>15)
			{
			post_exit=i;
			//Search backwards for point where acceleration starts. Start out with fairly large interval to prevent random noise from terminating the search early, then use progressively smaller intervals to find exact moment of exit.
				for(int j=5;j>0;j--)
				{
					while(i>log->takeoff&&(log->vel_d[i]-log->vel_d[i-j])/(0.01*(log->time[i]-log->time[i-j]))>2.0)i--;
				}
			log->exit=i;
			break;
			}
		}
//Find deployment time
log->deployment=-1;
	if(log->exit!=-1)
	{
		for(int i=post_exit;i<log->points;i++)
		{
		//Look for where vertical velocity drops below 15. This should get us near the deployment point
			if(log->vel_d[i]<15)
			{
			//Similar procedure - iterate backward for point where deceleration began.
				for(int j=5;j>0;j--)
				{
					while(i>log->exit&&(log->vel_d[i]-log->vel_d[i-j])/(0.01*(log->time[i]-log->time[i-j]))<-2.0)i--;
				}
			log->deployment=i;
			break;
			}
		}
	}else printf("Failed finding exit point\n");
//Find landing time
log->landing=log->points-1;
	if(log->deployment!=-1)
	{
		for(int i=log->points-1;i>log->deployment;i--)
		{
		//Iterate backward until downward motion is found
			if(log->vel_d[i]>2.0)
			{
			//Iterate forward for the point where all velocity components are close to zero
				while(log->vel_n[i]>0.5||log->vel_e[i]>0.5||log->vel_d[i]>0.5)i++;
			log->landing=i;
			break;
			}
		}
	}else printf("Failed finding deployment point\n");
/*
	for(int i=log->takeoff;i<log->landing;i++)
	{
	float t=0.01*(log->time[i]-log->time[log->takeoff]);
	log->altitude[i]=t;
	float airspeed=4.0+t/200.00;
	log->vel_n[i]=-2.0+4.0*(log->altitude[i]/1000.0)+airspeed*cos(0.02*t);
	log->vel_e[i]=-2.0+4.0*(log->altitude[i]/1000.0)+airspeed*sin(0.02*t);
	}
*/
return 0;
}

float log_get_time(log_t* log,int i)
{
return 0.01*(log->time[i]-log->time[log->exit]);
}

float log_get_distance(log_t* log,int i)
{
return log->distance[i]-log->distance[log->exit];
}

float log_get_altitude(log_t* log,int i)
{
return log->altitude[i];
}

float log_get_vel_vert(log_t* log,int i)
{
return log->vel_d[i];
}

float log_get_vel_horz(log_t* log,int i)
{
return sqrt((log->vel_n[i]-log->wind_n)*(log->vel_n[i]-log->wind_n)+(log->vel_e[i]-log->wind_e)*(log->vel_e[i]-log->wind_e));
}

float log_get_vel_total(log_t* log,int i)
{
return sqrt(log->vel_n[i]*log->vel_n[i]+log->vel_e[i]*log->vel_e[i]+log->vel_d[i]*log->vel_d[i]);
}

void log_get_acc_difference_points(log_t* log,int i,int* left,int* right)
{
int interval=10;
int l=i-interval>=0?i-interval:0;
	if(i>=log->exit&&i-interval<log->exit)l=log->exit;
	if(i>=log->deployment&&i-interval<log->deployment)l=log->deployment;
int r=i+interval<=log->points-1?i+interval:log->points-1;
	if(i<log->exit&&i+interval>=log->exit)r=log->exit-1;
	if(i<log->deployment&&i+interval>=log->deployment)r=log->deployment-1;
*left=l;
*right=r;
}

float log_get_acc_n(log_t* log,int i)
{
int left,right;
log_get_acc_difference_points(log,i,&left,&right);
return (log->vel_n[right]-log->vel_n[left])/(0.01*(log->time[right]-log->time[left]));
}

float log_get_acc_d(log_t* log,int i)
{
int left,right;
log_get_acc_difference_points(log,i,&left,&right);
return (log->vel_d[right]-log->vel_d[left])/(0.01*(log->time[right]-log->time[left]));
}

float log_get_acc_e(log_t* log,int i)
{
int left,right;
log_get_acc_difference_points(log,i,&left,&right);
return (log->vel_e[right]-log->vel_e[left])/(0.01*(log->time[right]-log->time[left]));
}

float log_get_acc_vert(log_t* log,int i)
{
return fabs(log_get_acc_d(log,i));
}

float log_get_acc_horz(log_t* log,int i)
{
int left,right;
log_get_acc_difference_points(log,i,&left,&right);
return fabs(log_get_vel_horz(log,right)-log_get_vel_horz(log,left))/(0.01*(log->time[right]-log->time[left]));
}

float log_get_drag_lift_coefficient(log_t* log,int i,float* drag,float* lift)
{
float vn=log->vel_n[i]-log->wind_n;
float ve=log->vel_e[i]-log->wind_e;
float vd=log->vel_d[i];
//Total velocity
float v_mag=sqrt(vn*vn+ve*ve+vd*vd);

//Normalized direction vector
float nn=vn/v_mag;
float ne=ve/v_mag;
float nd=vd/v_mag;

//Compute specific aerodynamic force (acceleration minus gravity)
float an=log_get_acc_n(log,i);
float ae=log_get_acc_e(log,i);
float ad=log_get_acc_d(log,i)-9.81;

//Compute longitudinal acceleration (drag)
float acc_l=(an*nn+ae*ne+ad*nd);

//Subtract longitudinal acceleration from acceleration vector
an-=acc_l*nn;
ae-=acc_l*ne;
ad-=acc_l*nd;

//Compute perpendicular acceleration (lift)
float acc_p=sqrt(an*an+ae*ae+ad*ad);

float mass=80;
float A=0.5;

float pressure=101325*pow(1-2.25569e-5*log->altitude[i],5.25616);
float temp=288.15-0.0065*log->altitude[i];
float density=0.00348367875*pressure/temp;

*drag=-mass*acc_l/(0.5*density*A*v_mag*v_mag);
*lift=mass*acc_p/(0.5*density*A*v_mag*v_mag);
}

float log_get_drag_coefficient(log_t* log,int i)
{
float drag,lift;
log_get_drag_lift_coefficient(log,i,&drag,&lift);
return drag;
}

float log_get_lift_coefficient(log_t* log,int i)
{
float drag,lift;
log_get_drag_lift_coefficient(log,i,&drag,&lift);
return lift;
}

float log_get_lift_drag_ratio(log_t* log,int i)
{
float drag,lift;
log_get_drag_lift_coefficient(log,i,&drag,&lift);
return lift/drag;
}

float log_get_glide_ratio(log_t* log,int i)
{
return log_get_vel_horz(log,i)/log_get_vel_vert(log,i);
}

float log_get_heading(log_t* log,int i)
{
return M_PI+atan2(log->vel_e[i],log->vel_n[i]);
}

