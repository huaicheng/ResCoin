#include <sys/time.h>           /* gettimeofday() */
#include <time.h>               /* time() */
#include "ring_buffer.h"
#include "ewma.h"
#include "monitor.h"

#define WINSZ  10

/*
 *  "delta <-- ml1 - ml2"
 */
void compute_load_delta(struct mach_load *delta, struct mach_load *ml1,
                        struct mach_load *ml2)
{
    delta->cpu_load = ml1->cpu_load - ml2->cpu_load;
    delta->mem_load = ml1->mem_load - ml2->mem_load;
    delta->disk_load = ml1->disk_load - ml2->disk_load;
    /* delta->rd_load = ml1->rd_load - ml2->rd_load; */
    /* delta->wr_load = ml1->wr_load - ml2->wr_load; */
    /* delta->rx_load = ml1->rx_load - ml2->rx_load; */
    /* delta->tx_load = ml1->tx_load - ml2->tx_load; */
}


int main(int argc, char **argv)
{
    virConnectPtr conn = NULL;
    int *active_domain_ids = NULL;
    virNodeInfoPtr nodeinfo = NULL;
    long index = 0;
    time_t curtime = 0;
    int ret, i;
    struct mach_load *ac; /* size is the number of VMs */


    struct ring_buffer *est, *obs, *delta;
    struct ring_buffer **vm_est, **vm_obs, **vm_delta;

    /* Suppose the initial coefficient "alpha" value is 0.7 */
    struct ewma_coff  coff = { 0.7, 0.7, 0.7 }, 
                     hcoff = { 0.7, 0.7, 0.7 };


    /* build connection with hypervisor */
    conn = virConnectOpen(hypervisor);
    if (NULL == conn) {
        fprintf(stderr, "failed to open connection to %s\n", hypervisor);
        exit(VIR_ERR_NO_CONNECT);
    }

    /* collect info about node */
    nodeinfo = (virNodeInfoPtr)malloc(sizeof(virNodeInfo));
    if (0 == virNodeGetInfo(conn, nodeinfo))
        nr_cores = nodeinfo->cores;

    /* get the active domain instances by ID */
    active_domain_num = virConnectNumOfDomains(conn);
    /* firstly, get domain IDs of active VMs, store it in an array */
    active_domain_ids = (int *)malloc(sizeof(int) * active_domain_num);
    ret = virConnectListDomains(conn, active_domain_ids, active_domain_num);
    if (-1 == ret) {
        fprintf(stderr, "failed to get IDs of active VMs\n");
        exit(VIR_ERR_OPERATION_FAILED);
    }

    /* store all VM information obtained to struct vm_info */
    struct vm_info *vminfo = 
        (struct vm_info *)malloc(sizeof(struct vm_info) * active_domain_num);
    struct phy_info *phyinfo = 
        (struct phy_info *)malloc(sizeof(struct phy_info) * 1);

    init_phy_info(phyinfo);
    create_phy_rst_file(phyinfo);

    /* for host workload related data storage */
    est = (struct ring_buffer *)malloc(sizeof(struct ring_buffer));
    obs = (struct ring_buffer *)malloc(sizeof(struct ring_buffer));
    delta = (struct ring_buffer *)malloc(sizeof(struct ring_buffer));
    rb_init(est, WINSZ);
    rb_init(obs, WINSZ);
    rb_init(delta, WINSZ);

    /* ring buffer for each VM */ 
    vm_est = (struct ring_buffer **)
        malloc(sizeof(struct ring_buffer *) * active_domain_num);
    vm_obs = (struct ring_buffer **)
        malloc(sizeof(struct ring_buffer *) * active_domain_num);
    vm_delta = (struct ring_buffer **)
        malloc(sizeof(struct ring_buffer *) * active_domain_num);
    for (i = 0; i < active_domain_num; i++) {
        vm_est[i] = (struct ring_buffer *)malloc(sizeof(struct ring_buffer));
        vm_obs[i] = (struct ring_buffer *)malloc(sizeof(struct ring_buffer));
        vm_delta[i] = (struct ring_buffer *)malloc(sizeof(struct ring_buffer));
        rb_init(vm_est[i], WINSZ);
        rb_init(vm_obs[i], WINSZ);
        rb_init(vm_delta[i], WINSZ);
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
        //print_vm_info(&vminfo[i]);
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

    /* time window history data storage */


    ac = (struct mach_load *)malloc(sizeof(struct mach_load) * active_domain_num);
    memset(ac, 0, sizeof(struct mach_load)*active_domain_num);


    while (1) {

        index++;
        curtime = time((time_t *)NULL);
        pno = index % WINSZ;

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

        /* elaspsed time in microsecond */
        long elapsed_time = (tv_after->tv_sec - tv_before->tv_sec) * 1000000 + 
            (tv_after->tv_usec - tv_before->tv_usec);
        
        compute_phy_load(phy_sysload, phy_stat_before, 
                phy_stat_after, elapsed_time);

        /* write the observed host data to ring buffer */
        rb_write(obs, phy_sysload);

        fprintf(phyinfo->fp, FORMATS, 
                (long)curtime, index, phy_sysload->cpu_load, 
                phy_sysload->mem_load, /*phy_sysload->rd_load, 
                phy_sysload->wr_load, */phy_sysload->disk_load/*,
                phy_sysload->rx_load, phy_sysload->tx_load*/);

        for (i = 0; i < active_domain_num; i++) {
            compute_vm_load(&vm_sysload[i], &vm_stat_before[i], 
                    &vm_stat_after[i], elapsed_time, nodeinfo->memory,
                    phy_sysload);

            /* write the observed data to ring buffer */
            rb_write(vm_obs[i], &vm_sysload[i]);

            /* 
             * TODO: flush the buffer when program exits abnormally 
             * using signal processing 
             */
            fprintf(vminfo[i].fp, FORMATS, 
                    (long)curtime, index, vm_sysload[i].cpu_load, 
                    vm_sysload[i].mem_load, /*vm_sysload[i].rd_load, 
                    vm_sysload[i].wr_load, */vm_sysload[i].disk_load/*, 
                    vm_sysload[i].rx_load, vm_sysload[i].tx_load*/);
        }
        /* 
         * actually, we have collected all information needed, 
         * do the prediction and schedule here 
         */

        /* init the first estimation value to the observed data */
        struct mach_load delta_tmp;
        if (index == 1) {
            /* VMs' first estimation value initialization */
            for (i = 0; i < active_domain_num; i++) {
                rb_write(vm_est[i], &vm_obs[i]->buff[0]);
                /* delta = vm_est - vm_obs */
                memset(&delta_tmp, 0, sizeof(delta_tmp));
                rb_write(vm_delta[i], &delta_tmp);
            }
        } else {
            /* estimate the next time period workload data using ewma() */
            struct mach_load est_curr_val;
            int prev_pos;
            
            /* VMs */
            for (i = 0; i < active_domain_num; i++) { 
                prev_pos = (vm_est[i]->end - 1 + vm_est[i]->size) % 
                    vm_est[i]->size;
                memset(&est_curr_val, 0, sizeof(est_curr_val));
                ewma_load(&est_curr_val, vm_est[i]->buff[prev_pos], 
                        vm_obs[i]->buff[prev_pos], coff);
                rb_write(vm_est[i], &est_curr_val);
                compute_load_delta(&delta_tmp, &vm_est[i]->buff[prev_pos], 
                        &vm_obs[i]->buff[prev_pos]);
                rb_write(vm_delta[i], &delta_tmp);

                /* for test */
                struct mach_load est_val, obs_val;
                rb_read_last(vm_est[i], &obs_val);
                rb_read_last(vm_est[i], &est_val);
                printf("%6.2lf%6.2lf%6.2lf%6.2lf%6.2lf%6.2lf\n", 
                        obs_val.cpu_load, est_val.cpu_load,
                        obs_val.mem_load, est_val.mem_load,
                        obs_val.disk_load, est_val.disk_load);
            }
        }

        /* do resource scheduling here */
        /* schedule(struct ring_buffer *obs, struct ring_buffer **vm_delta, 
                int active_domain_num); */
    }

    /* free all the VM instances */
    for (i = 0; i < active_domain_num; i++) {
        free(vminfo[i].dp);
        free(vminfo[i].blkname);
        free(vminfo[i].ifname);
        fclose(vminfo[i].fp);
        free(&vminfo[i]);
    }
    free(phyinfo);
    virConnectClose(conn);
    free(active_domain_ids);
    free(tv_before);
    free(tv_after);
    free(vm_stat_before);
    free(vm_stat_after);
    free(phy_stat_before);
    free(phy_stat_after);

    return 0;
}
