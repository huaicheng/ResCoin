#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Structure for CPU statistics.
 * In activity buffer: First structure is for global CPU utilisation ("all").
 * Following structures are for each individual CPU (0, 1, etc.)
 */
struct stats_cpu {
	unsigned long long cpu_user		__attribute__ ((aligned (16)));
	unsigned long long cpu_nice		__attribute__ ((aligned (16)));
	unsigned long long cpu_sys		__attribute__ ((aligned (16)));
	unsigned long long cpu_idle		__attribute__ ((aligned (16)));
	unsigned long long cpu_iowait		__attribute__ ((aligned (16)));
	unsigned long long cpu_steal		__attribute__ ((aligned (16)));
	unsigned long long cpu_hardirq		__attribute__ ((aligned (16)));
	unsigned long long cpu_softirq		__attribute__ ((aligned (16)));
	unsigned long long cpu_guest		__attribute__ ((aligned (16)));
	unsigned long long cpu_guest_nice	__attribute__ ((aligned (16)));
};


long get_HZ()
{
    return sysconf(_SC_CLK_TCK);
}


/* read cpu related time info from /proc/stat */
void rd_proc_stat(struct stats_cpu *sc[], int cores_nr)
{
    FILE *fp = NULL;
    char line[8192]; 

    if (NULL == (fp = fopen("/proc/stat", "r"))) {
        fprintf(stderr, "fopen /proc/stat error");
        exit(1);
    }

    while (fgets(line, sizeof(line), fp)) {
        if (!strncmp(line, "cpu ", 4)) {
            sscanf(line, "%*s%llu%llu%llu%llu%llu%llu%llu%llu%llu%llu", 
                    &sc[0]->cpu_user, 
                    &sc[0]->cpu_nice, 
                    &sc[0]->cpu_sys,
                    &sc[0]->cpu_idle,
                    &sc[0]->cpu_iowait, 
                    &sc[0]->cpu_steal, 
                    &sc[0]->cpu_hardirq, 
                    &sc[0]->cpu_softirq, 
                    &sc[0]->cpu_guest, 
                    &sc[0]->cpu_guest_nice
                  );
        }
        else if (!strncmp(line, "cpu0", 4)) {
            sscanf(line, "%*s%llu%llu%llu%llu%llu%llu%llu%llu%llu%llu", 
                    &sc[1]->cpu_user, 
                    &sc[1]->cpu_nice, 
                    &sc[1]->cpu_sys,
                    &sc[1]->cpu_idle,
                    &sc[1]->cpu_iowait, 
                    &sc[1]->cpu_steal, 
                    &sc[1]->cpu_hardirq, 
                    &sc[1]->cpu_softirq, 
                    &sc[1]->cpu_guest, 
                    &sc[1]->cpu_guest_nice
                  );
        }
        else if (!strncmp(line, "cpu1", 4)) {
            sscanf(line, "%*s%llu%llu%llu%llu%llu%llu%llu%llu%llu%llu", 
                    &sc[2]->cpu_user, 
                    &sc[2]->cpu_nice, 
                    &sc[2]->cpu_sys,
                    &sc[2]->cpu_idle,
                    &sc[2]->cpu_iowait, 
                    &sc[2]->cpu_steal, 
                    &sc[2]->cpu_hardirq, 
                    &sc[2]->cpu_softirq, 
                    &sc[2]->cpu_guest, 
                    &sc[2]->cpu_guest_nice
                  );
        }
    }
}


#define nr_cores 3

int main()
{
    struct stats_cpu *sc_prev[nr_cores], *sc_curr[nr_cores];
    struct timeval tv_prev, tv_curr;
    double itv;
    char line[8192];
    int i;

    for (i = 0; i < nr_cores; i++) {
        sc_prev[i] = (struct stats_cpu *)malloc(sizeof(struct stats_cpu));
        sc_curr[i] = (struct stats_cpu *)malloc(sizeof(struct stats_cpu));
    }

    gettimeofday(&tv_prev, NULL);
    rd_proc_stat(sc_prev, nr_cores);

    sleep(10);
    gettimeofday(&tv_curr, NULL);
    rd_proc_stat(sc_curr, nr_cores);


    itv = (tv_curr.tv_sec * 1000000 + tv_curr.tv_usec) - (tv_prev.tv_sec * 1000000 + tv_prev.tv_usec);  // in microseconds 
    itv /= 1000000; // in seconds
    long HZ = get_HZ();
    itv *= HZ;
    printf("%4s%10s%10s%10s%10s%10s%10s%10s%10s%10s%12s\n", "    ", "user%", "nice%", "sys%", "idle", "iowait%", "irq%", "soft%", "steal%", "guest%", "guest_nice%");

    // calculation of cpu usage
    for (i = 0; i < nr_cores; i++)
        printf("cpu%d: %10.2f%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f%12.2f\n", i,
                (sc_curr[i]->cpu_user - sc_prev[i]->cpu_user)/itv*100, 
                (sc_curr[i]->cpu_nice - sc_prev[i]->cpu_nice)/itv*100,
                (sc_curr[i]->cpu_sys - sc_prev[i]->cpu_sys)/itv*100,
                (sc_curr[i]->cpu_idle - sc_prev[i]->cpu_idle)/itv*100,
                (sc_curr[i]->cpu_iowait - sc_prev[i]->cpu_iowait)/itv*100,
                (sc_curr[i]->cpu_steal - sc_prev[i]->cpu_steal)/itv*100,
                (sc_curr[i]->cpu_hardirq - sc_prev[i]->cpu_hardirq)/itv*100,
                (sc_curr[i]->cpu_softirq - sc_prev[i]->cpu_softirq)/itv*100,
                (sc_curr[i]->cpu_guest - sc_prev[i]->cpu_guest)/itv*100,
                (sc_curr[i]->cpu_guest_nice - sc_prev[i]->cpu_guest_nice)/itv*100
              );
    printf("time interval=%.lf\n", itv);
    for (i = 0; i < nr_cores; i++)
        printf("cpu%d interval without guest: %lld\n", i, 
                (sc_curr[i]->cpu_user - sc_prev[i]->cpu_user) + 
                (sc_curr[i]->cpu_nice - sc_prev[i]->cpu_nice) +
                (sc_curr[i]->cpu_sys - sc_prev[i]->cpu_sys) +
                (sc_curr[i]->cpu_idle - sc_prev[i]->cpu_idle) +
                (sc_curr[i]->cpu_iowait - sc_prev[i]->cpu_iowait) +
                (sc_curr[i]->cpu_steal - sc_prev[i]->cpu_steal) +
                (sc_curr[i]->cpu_hardirq - sc_prev[i]->cpu_hardirq) +
                (sc_curr[i]->cpu_softirq - sc_prev[i]->cpu_softirq) /*+
                (sc_curr[i]->cpu_guest - sc_prev[i]->cpu_guest) +
                (sc_curr[i]->cpu_guest_nice - sc_prev[i]->cpu_guest_nice)*/
              );
    for (i = 0; i < nr_cores; i++)
        printf("cpu%d interval with guest: %lld\n", i, 
                (sc_curr[i]->cpu_user - sc_prev[i]->cpu_user) + 
                (sc_curr[i]->cpu_nice - sc_prev[i]->cpu_nice) +
                (sc_curr[i]->cpu_sys - sc_prev[i]->cpu_sys) +
                (sc_curr[i]->cpu_idle - sc_prev[i]->cpu_idle) +
                (sc_curr[i]->cpu_iowait - sc_prev[i]->cpu_iowait) +
                (sc_curr[i]->cpu_steal - sc_prev[i]->cpu_steal) +
                (sc_curr[i]->cpu_hardirq - sc_prev[i]->cpu_hardirq) +
                (sc_curr[i]->cpu_softirq - sc_prev[i]->cpu_softirq) +
                (sc_curr[i]->cpu_guest - sc_prev[i]->cpu_guest) +
                (sc_curr[i]->cpu_guest_nice - sc_prev[i]->cpu_guest_nice)
              );

    return 0;
}
