/* Attention: In development, still not compilable */
#include <math.h>
#include "schedule.h"
#include "ring_buffer.h"
#include "monitor.h"

void sum_load(struct mach_load *dsum, struct mach_load **ds, 
        int nrvm)
{
    int i;
    for (i = 0; i < nrvm; i++) {
        dsum->cpu_load  += ds[i]->cpu_load;
        dsum->mem_load  += ds[i]->mem_load;
        dsum->disk_load += ds[i]->disk_load;
    }
}

/* 
 * CPU bandwidth allocation, e.g. 40% CPU, cpu_bw_perc ~ [0 - 100%]
 * TODO: don't support dynamically change the "period" parameter yet, it only 
 * allocate cpu_quota according to the current period and bw_percentage
 * TODO: think about the real situation, say we have 8 pCPUs, and the domain
 * has 2 vCPUs. The the domain take 2 / 8 = 25% of the total physical CPU 
 * resource at most. Does cfs.quota / cfs.period work for only ONE CPU or mean
 * all CPUs ? If it aims at one CPU, the following function works for each vCPU
 * of the domain, then it should never exceed its upper bound.
 */
int alloccpu(virDomainPtr domain, double cpu_bw_perc)
{
    int ret = -1;
    unsigned long long cpu_period;
    long long cpu_quota = -1;
    virNodeInfo nodeinfo;
    virDomainInfo dominfo;
    unsigned int nr_pcpu = 0, nr_vcpu = 0;

    if (-1 == virNodeGetInfo(domain, &nodeinfo))
        goto cleanup;
    nr_pcpu = nodeinfo.cpus;

    if (-1 == virDomainGetInfo(domain, &dominfo))
        goto cleanup;
    nr_vcpu = dominfo.nrVirtCpu;

    if (cpu_bw_percentage <= (double)nr_vcpu / nr_pcpu) {

        if (-1 == get_vcpu_period(domain, &cpu_period))
            goto cleanup;

        /* 
         * Compute the new quota which should be allocated to the domain, the 
         * quota is applied to each vcpu, thus the cpu_bw_percentage should be
         * divided by nr_vcpu.
         */
        cpu_quota = 
            (long long)(cpu_bw_perc / nr_vcpu * nr_pcpu * cpu_period);
    } else {
        /* 
         * allocate at most (nr_vcpu / nr_pcpu) bandwidth for the domain
         */
        cpu_quota = (long long)(cpu_period);
    }

    if (-1 == set_vcpu_quota(domain, cpu_quota))
        goto cleanup;

    ret = 0;

cleanup:
    return ret;
}

/* 
 * runtime memory allocate for domain
 */
int allocmem(virDomainPtr domain, double mem_percentage)
{
    int ret = -1;
    struct sysinfo pinfo;
    unsigned long vmemsz;
    virDomainInfo dominfo;

    if (-1 == sysinfo(&pinfo))
        goto cleanup;
    vmemsz = (unsigned long)(mem_percentage * pinfo.totalram);

    if (-1 == virDomainGetInfo(domain, dominfo)) 
        goto cleanup;

    /* memory size can't be too small, or OOM would kill the VM process */
    if (vmemsz < VIR_MEM_LOW_BOUND_IN_BYTES) {
        vmemsz = VIR_MEM_LOW_BOUND_IN_BYTES;
    } else if (vmemsz > dominfo.maxMem) {
    /* memory size < maxMem, for qemu, maxMem can't be changed at runtime */ 
        fprintf(stderr, "allocate %lu memory to domain which is larger" 
                "than the maxMem [%lu]\n", vmemsz, dominfo.maxMem);
        goto cleanup;
    }

    /* should also set a lower memory bound for the domain */

    if (-1 == virDomainSetMemory(domain, vmemsz))
        goto cleanup;
    
    ret = 0;

cleanup:
    return ret;
}

/*
 * given the disk bandwidth, disk read_bytes_sec and write_bytes_sec should be
 * throttled respectively
 */
int allocdsk(virDomainPtr domain, const char *dsk, 
        double disk_rbps, double disk_wbps)
{
    /* (1). transform disk_bw_percentage to read_bytes_sec and write_bytes_sec
     * throttling, HOW ?? ==> Can't be done, read and write throttling must be
     * given respectively.
     */
    int ret = -1;
    char str_rbps[64], str_wbps[64];

    snprintf(str_rbps, "%s,%lf", dsk, disk_rbps);
    snprintf(str_wbps, "%s,%lf", dsk, disk_wbps);

    if (!(set_blkio_read_bytes(domain, str_rbs) 
                || set_blkio_write_bytes(domain, str_wbs))) 
        goto cleanup;
    
    ret = 0;

cleanup:
    ret return;

}

double cal_wr(double i_load, double t_load)
{
    if (fabs(t_load) < 1e-6)
        return 0.0;
    return i_load / t_load;
}

double min(double a, double b)
{
    return (a <= b) ? a : b;
}

/* the policy to schedule CPU resources */
void schedcpu(struct vm_info *vinfo, double capcpu, double *tccpu, double *wr, 
        double *cpu_est, int nrvm)
{
    int i;
    double availcpu = 0.0, ttaken = 0.0;

    for (i = 0; i < nrvm; i++) {
        ttaken += min(cpu_est[i], tccpu[i]);
    }
    availcpu = capcpu - ttaken;
    if (availcpu < 0.0)
        availcpu = 0.0;

    /* cpu resource reallocation */
    if (availcpu > 0.0) {
        for (i = 0; i < nrvm; i++) {
            alloccpu(cpu_est[i]);
        }
    } else {
        for (i = 0; i < nrvm; i++) {
            alloccpu(vinfo[i].dp, tc[i] + availcpu * wr[i]);
        }
    }
}

/*
 * attention: should have some upper/lower bound for memory allocation 
 * TODO: slice memory allocation, each slice takes up some amount of 
 * memory, which should be allocated accordingly.
 */
void schedmem(struct vm_info *vinfo, double capmem, double *tcmem, double *wr,
        double *mem_est, int nrvm)
{
    int i;
    double availmem = 0.0, ttaken = 0.0;

    for (i = 0; i < nrvm; i++) {
        ttaken += min(mem_est[i], tcmem[i]);
    }
    availmem = capmem - ttaken;
    if (availmem < 0.0)
        availmem = 0.0;

    /* memory resource reallocation */
    if (availmem > 0.0) {
        for (i = 0; i < nrvm; i++) {
            allocmem(mem_est[i]); 
        }
    } else {
        for (i = 0; i < nrvm; i++) {
            allocmem(vinfo[i].dp, tc[i] + availmem * wr[i]);
        }
    }
}

/*
 * different from cpu scheduling, have to schedule read/write respectively.
 */
void scheddsk(struct vm_info *vinfo, double capdsk, double *tcdsk, double *wr,
        double *dsk_rest, double *dsk_west, int nrvm)
{
#define DSKNAME "/dev/sda"
    int i;
    double availdsk = 0.0, ttaken = 0.0;

    for (i = 0; i < nrvm; i++) {
        ttaken += min(dsk_rest[i]+dsk_west[i], tcdsk[i]);
    }
    availdsk = capdsk - ttaken;
    if (availdsk < 0.0)
        availdsk = 0.0;

    /* disk resource reallocation */
    if (availdsk > 0.0) {
        for (i = 0; i < nrvm; i++) {
            allocdsk(vinfo[i].dp, DSKNAME, dsk_rest[i], dsk_west[i]); 
        }
    } else {
        for (i = 0; i < nrvm; i++) {
            allocdsk(vinfo[i].dp, DSKNAME, tc[i] + availdsk * wr[i], 
                    tc[i] + availdsk * wr[i]);
        }
    }
}

/* 
 * Update the accumulated credit for the virtual machine
 */
void update_ac(struct mach_laod *ac, struct mach_load *tc, 
        struct ring_buffer *vmobs)
{
    struct mach_load lload, kload;

    rb_read_last(vmobs, &lload);
    /* 
     * TODO: need to implement this API in ring_buffer.h 
     * it reads the k-th history load data from the ring buffer
     */
    rb_read_ith(vmobs, &kload); 
    /* ac(t) += tc(t-1) - au(t-1) - [tc(t-k-a) - au(t-k-1)] */
    *ac += lload - kload; /* suppose the ac doesn't change, so it's reduced */
}

/*
 * @obs/vm_obs: store the observed workload data 
 * @est/vm_est: store the estimated workload data
 * @ac: store the accumulated credit
 * @tc: store the VM template credit
 */
void schedule(struct ring_buffer *obs, struct ring_buffer *est, 
        struct ring_buffer **vm_obs, struct ring_buffer **vm_est,
        struct mach_load *ac, int nrvm)
{
    int i;
    double wr_cpu, wr_mem, wr_disk;

    struct mach_load hcap = {
        .cpu_load = 1.0,
        .mem_load = 1.0,
        .disk_load = 1.0
    };

    struct mach_load last_vm_est[nrvm];

    struct mach_load est_sum;
    sum_load(&est_sum, vm_est, nrvm);

    struct mach_load ac_sum;
    sum_load(&ac_sum, &ac, nrvm);

    /* read last estimation of all VMs to array as they are frequently used.*/
    for (i = 0; i < nr_vm; i++) {
        rb_read_last(vm_est[i], &last_vm_est[i]);
        update_ac();
    }

    /* schedule cpu, memory and disk respectively */
    schedcpu();
    schedmem();
    scheddsk();

    for (i = 0; i < nr_vm; i++) {

    }
}
