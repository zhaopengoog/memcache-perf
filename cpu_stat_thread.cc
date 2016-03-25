#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "cpu_stat_thread.h"

static int capture_period=1;
static int print_all_cpu_stats=0;
static volatile int g_stop_cpu_stats=0;
static volatile int g_reset_cpu_stats=0;

void stop_cpu_stats() {
	g_stop_cpu_stats=1;
}
void reset_cpu_stats() {
	g_reset_cpu_stats=1;
}

void detail_cpu_stats(int level) {
	print_all_cpu_stats=level;
}
void cpu_stats_interval(int interval) {
	capture_period=interval;
}
static long double get_cpu_load() {
    long double a[4], b[4], loadavg=0;
    FILE *fp;
	int c=0;
	
        fp = fopen("/proc/stat","r");
        c+=fscanf(fp,"%*s %Lf %Lf %Lf %Lf",&a[0],&a[1],&a[2],&a[3]);
        fclose(fp);
        sleep(1);

        fp = fopen("/proc/stat","r");
        c+=fscanf(fp,"%*s %Lf %Lf %Lf %Lf",&b[0],&b[1],&b[2],&b[3]);
        fclose(fp);

		if (c != 8)
			return -1.0;
        loadavg = ((b[0]+b[1]+b[2]) - (a[0]+a[1]+a[2])) / ((b[0]+b[1]+b[2]+b[3]) - (a[0]+a[1]+a[2]+a[3]));
	return loadavg;
}

void *cpu_stat_thread(void *pdata) {
    long double loadavg, total=0, count=0.0;
	long double max,min;
	cpu_info_t *data=(cpu_info_t *)pdata;
	g_stop_cpu_stats=0;
	
    sleep(capture_period);
	loadavg=get_cpu_load();
	if (loadavg < 0.0) {
		printf("Warning! Don't know how to process this /proc/stat!\n");
		data->min=0.0;
		data->max=0.0;
		data->avg=0.0;
		return data;
	}
	max=loadavg;
	min=loadavg;

    for(;;)
    {
        sleep(capture_period);
		if (g_stop_cpu_stats)
			break;
		loadavg=get_cpu_load();
		if (g_reset_cpu_stats) {
			g_reset_cpu_stats=0;
			count=0;
			total=0;
			max=loadavg;
			min=loadavg;
		}
		if (loadavg > max) 
			max=loadavg;
		if (loadavg < min)
			min=loadavg;
		count+=1.0;
		total+=loadavg;
	data->max=	max*100.0;
	data->min=	min*100.0;
	data->avg=	total*100.0 / count;
		if (print_all_cpu_stats)
			printf("The current CPU utilization is : %Lf\n",loadavg);
    }

#ifndef DISABLE_WARNING	
	if (max > .95) 
		printf("Warning! Detected max cpu usage > 95%%\n");
#endif
	
    return data;
}


