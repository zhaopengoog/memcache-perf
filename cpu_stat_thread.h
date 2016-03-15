#ifndef _cpu_stat_thread_h_
#define _cpu_stat_thread_h_

/* Quick cpu stats thread

	Author: Shay Gal-On
	
	Usage: 
	
	Allocate a cpu_info_t and create a cpu_stat_thread() thread with that structure as parameter. 
	Stop the thread with stop_cpu_stats().
	Set detail_cpu_stats_level to 1 to get cpu stats output at every cpu_stats_interval.
	Set interval to N to cpature cpu stats every N seconds.
	
	Example:
	
  cpu_info_t cpustat;
  pthread_create(&(cpustat.tid), 0, cpu_stat_thread, &cpustat);
  stop_cpu_stats();
	.. DO SOMETHING ..
  pthread_join(cpustat.tid,0);
  
*/


typedef struct cpu_info_s {
	long double max,min,avg;
	pthread_t tid;
} cpu_info_t;

/* A thread to monitor cpu stats, returns a cpu_info_s containing info about the cpu load while running when stop_cpu_stats() is called.
   Will output a warning if cpu usage max went over 95% during monitored period.
 */
void *cpu_stat_thread(void *pdata);
void stop_cpu_stats();
void cpu_stats_detail(int level);
void cpu_stats_interval(int interval);

#endif
