/* Attention: In development, still not compilable */
#include "schedule.h"
#include "ring_buffer.h"
#include "monitor.h"

void sum_load(struct mach_load *dsum, struct mach_load **ds, 
        int nr_vm)
{
    int i;
    for (i = 0; i < nr_vm; i++) {
        dsum->cpu_load  += ds[i]->cpu_load;
        dsum->mem_load  += ds[i]->mem_load;
        dsum->disk_load += ds[i]->disk_load;
    }
}

/* 
 * CPU bandwidth allocation, e.g. 40% CPU, cpu_bw_percentage ~ [0 - 100%]
 * TODO: don't support dynamically change the "period" parameter yet, it only 
 * allocate cpu_quota according to the current period and bw_percentage
 * TODO: think about the real situation, say we have 8 pCPUs, and the domain
 * has 2 vCPUs. The the domain take 2 / 8 = 25% of the total physical CPU 
 * resource at most. Does cfs.quota / cfs.period work for only ONE CPU or mean
 * all CPUs ? If it aims at one CPU, the following function works for each vCPU
 * of the domain, then it should never exceed its upper bound.
 */
int allocate_cpu(virDomainPtr domain, double cpu_bw_percentage)
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
            (long long)(cpu_bw_percentage / nr_vcpu * nr_pcpu * cpu_period);
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
int allocate_mem(virDomainPtr domain, double mem_percentage)
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
void allocate_disk(virDomainPtr domain, double disk_bw_percentage)
{
    /* (1). transform disk_bw_percentage to read_bytes_sec and write_bytes_sec
     * throttling, HOW ??
     */
}

double cal_wr(double i_load, double t_load)
{
    return i_load / t_load;
}

double min(double a, double b)
{
    return (a <= b) ? a : b;
}

/* the policy to schedule CPU resources */
void schedule_cpu(double t_cpu_load, double cpu_cap, double *cpu_tc, 
        double *cpu_est, int nr_vm)
{
    int i;
    double avail = 0.0, taken = 0.0;

    for (i = 0; i < nr_vm; i++) {
        taken += min(cpu_est[i], tc[i]);
    }
    avail = cpu_cap - taken;

    /* cpu resource reallocation */
    if (t_cpu_load <= cpu_cap) {
        for (i = 0; i < nr_vm; i++) {
            allocate_cpu(cpu_est[i]); // 按照预测值进行分配
        }
    } else {
        for (i = 0; i < nr_vm; i++) {
            avail = cpu_cap - t_vm_load;
            wr_cpu = cal_wr(ac[i].cpu_load / ac_sum.cpu_load);
            //wr_cpu = ac[i].cpu_load / ac_sum.cpu_load;
            allocate_cpu(tc[i] + avail * wr_cpu);
        }
    }
}

void schedule_memory()
{
    /* memory resource reallocation */
    if (est_sum.mem_load <= hcap.mem_load) {
        for (i = 0; i < nr_vm; i++) {
            allocate_mem(last_vm_est[i].mem_load);
        }
    } else {
        for (i = 0; i < nr_vm; i++) {
            wr_mem = ac[i].mem_load / ac_sum.mem_load;
            allocate_mem(wr_mem);
        }
    }
}

void schedule_disk()
{
    /* disk resource reallocation */
    if (est_sum.disk_load <= hcap.disk_load) {
        for (i = 0; i < nr_vm; i++) {
            allocate_disk(last_vm_est[i].disk_load);
        }
    } else {
        for (i = 0; i < nr_vm; i++) {
            wr_disk = ac[i].disk_load / ac_sum.disk_load;
            allocate_disk(wr_disk);
        }
    }
}

/*
 * @obs/vm_obs: store the observed workload data 
 * @est/vm_est: store the estimated workload data
 * @ac: store the accumulated credit
 * @tc: store the VM template credit
 */
void schedule(struct ring_buffer *obs, struct ring_buffer *est, 
        struct ring_buffer **vm_obs, struct ring_buffer **vm_est,
        struct mach_load *ac, int nr_vm);
{
    int i;
    double wr_cpu, wr_mem, wr_disk;

    struct mach_load hcap = {
        .cpu_load = 1.0,
        .mem_load = 1.0,
        .disk_load = 1.0
    };

    struct mach_load last_vm_est[nr_vm];

    struct mach_load est_sum;
    sum_load(&est_sum, vm_est, nr_vm);

    struct mach_load ac_sum;
    sum_load(&ac_sum, &ac, nr_vm);

    /* read last estimation of all VMs to array as they are frequently used.*/
    for (i = 0; i < nr_vm; i++) {
        rb_read_last(vm_est[i], &last_vm_est[i]);
    }
    
    schedule_cpu();
    schedule_memory();
    schedule_disk();

    for (i = 0; i < nr_vm; i++) {

    }
}
