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

/* the way to allocate and control cpu resources, e.g. 40% CPU */
void allocate_cpu(virDomainPtr dom, double cpu_bw_percentage)
{
    set_cpu_bw(dom, cpu_bw_percentage);
}

/* e.g. 30% memory */
void allocate_mem(double mem_share_percentage)
{
}

/* e.g. 50% disk bandwidth */
void allocate_disk(double disk_bw_percentage)
{
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
void schedule_cpu(double t_cpu_load, double cpu_cap, double *cpu_tc, double *cpu_est, int nr_vm)
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
