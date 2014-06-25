/* Attention: In development, still not compilable */
#include "schedule.h"
#include "ring_buffer.h"
#include "monitor.h"

void sum_vm_est(struct mach_load *est_sum, struct ring_buffer **vm_est, 
        int nr_vm)
{
    int i;
    for (i = 0; i < nr_vm; i++) {
        struct mach_load vmtmp;
        read_rb_last(vm_est[i], &vmtmp);

        est_sum->cpu_load += vmtmp->cpu_load;
        est_sum->mem_load += vmtmp->mem_load;
        est_sum->disk_load += vmtmp->disk_load;
    }
}

void allocate_cpu(double cpu_share_percentage)
{
}

void allocate_mem(double mem_share_percentage)
{
}

void allocate_disk(double disk_share_percentage)
{
}

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
    sum_vm_est(&est_sum, vm_est, nr_vm);

    struct mach_load ac_sum;
    for (i = 0; i< nr_vm; i++) {
        ac_sum.cpu_load = ac[i].cpu_load;
        ac_sum.mem_load = ac[i].mem_load;
        ac_sum.disk_load = ac[i].disk_load;
    }

    /* read last estimation of all VMs to array as they are frequently used.*/
    for (i = 0; i < nr_vm; i++) {
        rb_read_last(vm_est[i], &last_vm_est[i]);
    }

    /* cpu resource reallocation */
    if (est_sum.cpu_load <= hcap.cpu_load) {
        for (i = 0; i < nr_vm; i++) {
            allocate_cpu(last_vm_est[i].cpu_load);
        }
    } else {
        for (i = 0; i < nr_vm; i++) {
            wr_cpu = ac[i].cpu_load / ac_sum.cpu_load;
            allocate_cpu(wr_cpu);
    }
    
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

    for (i = 0; i < nr_vm; i++) {

    }
}
