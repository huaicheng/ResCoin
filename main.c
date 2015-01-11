/*
 * ResCoin - Multiple type Resource Scheduling of VMs in pursuit of fairness 
 * Copyright (c) 2014, Coperd <lhcwhu@gmail.com>
 * All rights reserved.
 *
 * Author: Coperd <lhcwhu@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <sys/time.h>           /* gettimeofday() */
#include <time.h>               /* time() */
#include <math.h>

#include "ringbuffer.h"
#include "ewma.h"
#include "monitor.h"
#include "schedule.h"
#include "rc.h"

/* window size */
#define WINSZ  10 

int main(int argc, char **argv)
{
    virConnectPtr conn = NULL;
    int *active_domain_ids = NULL;
    virNodeInfoPtr nodeinfo = NULL;
    long index = 0;
    time_t curtime = 0;
    int ret, i, j;
    uint32_t totalmem;
    struct mach_load *ac, *tc, cap; /* size is the number of VMs */

    struct ringbuffer *est, *obs;
    struct ringbuffer **vm_est, **vm_obs;

    conn = virConnectOpen(hypervisor);
    if (NULL == conn)
    {
        fprintf(stderr, "failed to open connection to %s\n", hypervisor);
        exit(VIR_ERR_NO_CONNECT);
    }

    nodeinfo = (virNodeInfoPtr)malloc(sizeof(virNodeInfo));
    if (0 == virNodeGetInfo(conn, nodeinfo))
        totalmem = nodeinfo->memory;
    unsigned int nrcpus = nodeinfo->cpus;
    printf("\ntotalmem = %"PRIu32"\nnr_cpus=%u\n", totalmem, nrcpus);


    struct ewma_coff  coff = {0.7, 0.8, 0.8, 0.8};

    active_domain_num = virConnectNumOfDomains(conn);

    active_domain_ids = (int *)malloc(sizeof(int) * active_domain_num);
    ret = virConnectListDomains(conn, active_domain_ids, active_domain_num);
    if (-1 == ret)
    {
        fprintf(stderr, "failed to get IDs of active VMs\n");
        exit(VIR_ERR_OPERATION_FAILED);
    }


    double *mean_error, *diff;
    mean_error = (double *)malloc(sizeof(double) * active_domain_num);
    diff = (double *)malloc(sizeof(double) * active_domain_num);

    struct vm_info *vminfo = 
        (struct vm_info *)malloc(sizeof(struct vm_info) * active_domain_num);
    struct phy_info *phyinfo = 
        (struct phy_info *)malloc(sizeof(struct phy_info) * 1);          

    init_phy_info(phyinfo);
    create_phy_rst_file(phyinfo);    


    est = (struct ringbuffer *)malloc(sizeof(struct ringbuffer));
    obs = (struct ringbuffer *)malloc(sizeof(struct ringbuffer));
    rb_init(est, WINSZ);
    rb_init(obs, WINSZ);    

    vm_est = (struct ringbuffer **)
        malloc(sizeof(struct ringbuffer *) * active_domain_num);
    vm_obs = (struct ringbuffer **)
        malloc(sizeof(struct ringbuffer *) * active_domain_num);

    for (i = 0; i < active_domain_num; i++) {
        vm_est[i] = (struct ringbuffer *)malloc(sizeof(struct ringbuffer));
        vm_obs[i] = (struct ringbuffer *)malloc(sizeof(struct ringbuffer));

        rb_init(vm_est[i], WINSZ);
        rb_init(vm_obs[i], WINSZ);

    }

    for (i = 0; i < active_domain_num; i++) {

        init_vm_info(&vminfo[i]);

        vminfo[i].id = active_domain_ids[i]; //

        vminfo[i].dp = virDomainLookupByID(conn, vminfo[i].id); //
        if (NULL == vminfo[i].dp) {
            fprintf(stderr, "failed to get the domain instance with ID: %d\n", 
                    vminfo[i].id);
            exit(VIR_ERR_INVALID_DOMAIN);
        }

        get_vm_static_info(&vminfo[i]);
        print_vm_info(&vminfo[i]);
    }                                  


    const char *a = "Time";
    const char *b = "Index";
    const char *c = "cpu_load";
    const char *d = "mem_load";
    const char *e = "rd_load";
    const char *f = "wr_load";
    const char *h = "MeanError";

    for (i = 0; i < active_domain_num; i++) {
        fprintf(vminfo[i].fp, "%s  %s  %s  %s  %s  %s %s\n", a,b,c,d,e,f,h);      

    }


    struct timeval *tv_before = 
        (struct timeval *)malloc(sizeof(struct timeval));
    struct timeval *tv_after = 
        (struct timeval *)malloc(sizeof(struct timeval));

    struct vm_statistics *vm_stat_before = (struct vm_statistics *)
        malloc(sizeof(struct vm_statistics) * active_domain_num);

    struct vm_statistics *vm_stat_after = (struct vm_statistics *)
        malloc(sizeof(struct vm_statistics) * active_domain_num);

    struct mach_load *vm_sysload = (struct mach_load *)
        malloc(sizeof(struct mach_load) * active_domain_num);

    struct phy_statistics *phy_stat_before = 
        (struct phy_statistics *)malloc(sizeof(struct phy_statistics));

    struct phy_statistics *phy_stat_after = 
        (struct phy_statistics *)malloc(sizeof(struct phy_statistics));

    struct mach_load *phy_sysload = 
        (struct mach_load *)malloc(sizeof(struct mach_load));

    ac = (struct mach_load *)malloc(sizeof(struct mach_load) * active_domain_num);
    memset(ac, 0, sizeof(struct mach_load)*active_domain_num);

    tc = (struct mach_load *)malloc(sizeof(struct mach_load) * active_domain_num);

    /* VM template resoruce allocation */
    for (i = 0; i < active_domain_num; i++) {
        tc[i].cpu_load = 60.0 / active_domain_num;
        tc[i].mem_load = 100.0 / active_domain_num;
        tc[i].rd_load = 1.0 / active_domain_num;
        tc[i].wr_load = 1.0 / active_domain_num;
        printf("tc[%d].cpu_load=%.2lf | tc[%d].mem_load=%.2lf\n", i, tc[i].cpu_load, i, tc[i].mem_load);
    }

    cap.cpu_load = 60.0;
    cap.mem_load = 100.0;
    cap.rd_load = 10.0;
    cap.wr_load = 10.0;

    const char *g = "---------------------------------------";

    while (1) { 
        index++;
        printf("\nindex = %ld\n\n", index);

        curtime = time((time_t *)NULL);

        memset(tv_before, 0, sizeof(struct timeval));
        memset(tv_after, 0, sizeof(struct timeval));
        memset(vm_stat_before, 0, sizeof(struct vm_statistics));
        memset(vm_stat_after, 0, sizeof(struct vm_statistics));

        gettimeofday(tv_before, NULL);
        for (i = 0; i < active_domain_num; i++) {
            get_vm_workload(&vm_stat_before[i], &vminfo[i]);
        }       
        get_phy_workload(phy_stat_before);

        sleep(TIME_INTERVAL);

        gettimeofday(tv_after, NULL);
        for (i = 0; i < active_domain_num; i++) {
            get_vm_workload(&vm_stat_after[i], &vminfo[i]);
        }        
        get_phy_workload(phy_stat_after);

        long elapsed_time = (tv_after->tv_sec - tv_before->tv_sec) * 1000000 + 
            (tv_after->tv_usec - tv_before->tv_usec);

        compute_phy_load(phy_sysload, phy_stat_before, 
                phy_stat_after, elapsed_time);

        rb_write(obs, phy_sysload);
        fprintf(phyinfo->fp, FORMATS, (long)curtime, index, phy_sysload->cpu_load, phy_sysload->mem_load, phy_sysload->rd_load, phy_sysload->wr_load);
        printf("the loads of physical host:\n"); 
        print_mach_load(phy_sysload);                        


        for (i = 0; i < active_domain_num; i++) 
        {
            compute_vm_load(&vm_sysload[i], &vm_stat_before[i], 
                    &vm_stat_after[i], elapsed_time, nodeinfo->memory, nrcpus);

            rb_write(vm_obs[i], &vm_sysload[i]);

            fprintf(vminfo[i].fp, FORMATS, (long)curtime, index, vm_sysload[i].cpu_load, vm_sysload[i].mem_load, vm_sysload[i].rd_load,
                    vm_sysload[i].wr_load);

        }         

/*    if (index >= 2) {
        for (i = 0; i < active_domain_num; i++) {

            printf("mean_error[%d]=%lf\n",i, mean_error[i]);
            struct mach_load obs_lload, est_lload;

            rb_read_last(vm_obs[i], &obs_lload);

            rb_read_last(vm_est[i], &est_lload);

            diff[i] = fabs(est_lload.cpu_load - obs_lload.cpu_load)/obs_lload.cpu_load;

            mean_error[i] += (diff[i] - mean_error[i])/(index - 1);

            printf("diff[%d]=%lf | mean_error[%d]=%lf\n",i, diff[i], i, mean_error[i]);
        }
    }            */

        for (i = 0; i < active_domain_num; i++) {    
            estimate(vm_obs[i], vm_est[i], coff, index);
            struct mach_load est_val;
            rb_read_last(vm_est[i], &est_val);
            /*           fprintf(vminfo[i].fp, "%-6ld %-6ld %-6.2lf %-6.2lf %-6.2lf %-6.2lf %-6.2lf\n", (long)curtime, index, est_val.cpu_load, est_val.mem_load, est_val.rd_load, est_val.wr_load, mean_error[i]);              */
        } 


        for (i = 0; i < active_domain_num; i++)
            fprintf(vminfo[i].fp, "%s\n", g);  

        for (i = 0; i < active_domain_num; i++) {
            struct mach_load lload, kload;

            rb_read_last(vm_obs[i], &lload);
            if (index <= WINSZ-1) {
                ac[i].cpu_load += tc[i].cpu_load-lload.cpu_load;
                ac[i].mem_load += tc[i].mem_load-lload.mem_load;
                ac[i].rd_load += tc[i].rd_load-lload.rd_load;
                ac[i].wr_load += tc[i].wr_load-lload.wr_load;
            } else {
                kload = vm_obs[i]->buff[vm_obs[i]->start];

                ac[i].cpu_load += kload.cpu_load - lload.cpu_load;
                ac[i].mem_load += kload.mem_load - lload.mem_load;
                ac[i].rd_load += kload.rd_load - lload.rd_load;
                ac[i].wr_load += kload.wr_load - lload.wr_load;
            }
        }

        for (i = 0; i < active_domain_num; i++)
        {
            printf("ac[%d].cpu_load: %.2lf | ac[%d].mem_load: %.2lf\n",i,ac[i].cpu_load, i, ac[i].mem_load);
        }


        struct mach_load sum_ac;
        sum_ac.cpu_load = 0.0;
        sum_ac.mem_load = 0.0;
        sum_ac.rd_load = 0.0;
        sum_ac.wr_load = 0.0;

        for (i = 0; i < active_domain_num; i++) {
            sum_ac.cpu_load += ac[i].cpu_load;
            sum_ac.mem_load += ac[i].mem_load;
            sum_ac.rd_load += ac[i].rd_load;
            sum_ac.wr_load += ac[i].wr_load;
        }
        printf("sum_ac.cpu_load: %.2lf | sum_ac.mem_load: %.2lf\n", sum_ac.cpu_load, sum_ac.mem_load);


        double *wr_cpu, *wr_mem, *wr_rd, *wr_wr;
        wr_cpu = (double *)malloc(sizeof(double) * active_domain_num); 
        wr_mem = (double *)malloc(sizeof(double) * active_domain_num);
        wr_rd = (double *)malloc(sizeof(double) * active_domain_num);
        wr_wr = (double *)malloc(sizeof(double) * active_domain_num);
        for (i = 0; i < active_domain_num; i++) {
            wr_cpu[i] = ac[i].cpu_load/sum_ac.cpu_load;
            wr_mem[i] = ac[i].mem_load/sum_ac.mem_load;
            wr_rd[i] = ac[i].rd_load/sum_ac.rd_load;
            wr_wr[i] = ac[i].wr_load/sum_ac.wr_load;

            printf("wr_cpu[%d]=%lf | wr_mem[%d]=%lf\n", i, wr_cpu[i], i, wr_mem[i]);
        }

        struct mach_load last_vm_est[active_domain_num];     

        double *tccpu, *tcmem, *tcrd, *tcwr;
        tccpu = (double *)malloc(sizeof(double) * active_domain_num);
        tcmem = (double *)malloc(sizeof(double) * active_domain_num);
        tcrd = (double *)malloc(sizeof(double) * active_domain_num);
        tcwr = (double *)malloc(sizeof(double) * active_domain_num);

        double *cpu_est, *mem_est, *dsk_rest, *dsk_west;
        cpu_est = (double *)malloc(sizeof(double) * active_domain_num);
        mem_est = (double *)malloc(sizeof(double) * active_domain_num);
        dsk_rest = (double *)malloc(sizeof(double) * active_domain_num);
        dsk_west = (double *)malloc(sizeof(double) * active_domain_num);

        for (i = 0; i < active_domain_num; i++) {
            tccpu[i] = tc[i].cpu_load;
            tcmem[i] = tc[i].mem_load;
            tcrd[i] = tc[i].rd_load;
            tcwr[i] = tc[i].wr_load;

            rb_read_last(vm_est[i], &last_vm_est[i]);
            cpu_est[i] = last_vm_est[i].cpu_load;
            mem_est[i] = last_vm_est[i].mem_load;
            dsk_rest[i] = last_vm_est[i].rd_load;
            dsk_west[i] = last_vm_est[i].wr_load;

            printf("cpu_est[%d]=%.2lf | mem_est[%d]=%.2lf\n",i, cpu_est[i], i, mem_est[i]);
        }      

        printf("\n\n");       

        schedcpu(conn, vminfo, cap.cpu_load, tccpu, wr_cpu, cpu_est, active_domain_num); 

        schedmem(vminfo, cap.mem_load, tcmem, wr_mem, mem_est, active_domain_num);

        schedrd(vminfo, cap.rd_load, tcrd, wr_rd, dsk_rest, active_domain_num);

        schedwr(vminfo, cap.wr_load, tcwr, wr_wr, dsk_west, active_domain_num); 

    }

    printf("ResCoin exiting, index=%ld\n", index);

    virConnectClose(conn);
    return 0;
}
     
