#include <math.h>

#include "schedule.h"
#include "rc.h"

void sum_load(struct mach_load dsum, struct mach_load *ds, 
        int nrvm)
{
    int i;
    for (i = 0; i < nrvm; i++) {
        dsum.cpu_load  += ds[i].cpu_load;
        dsum.mem_load  += ds[i].mem_load;
        dsum.rd_load += ds[i].rd_load;
        dsum.wr_load += ds[i].wr_load;
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
int alloccpu(virConnectPtr conn, virDomainPtr domain, double cpu_bw_perc)
{
    int ret = -1;
    unsigned long long cpu_period;
    long long cpu_quota = -1;
    virNodeInfo nodeinfo;
    virDomainInfo dominfo;
    unsigned int nr_pcpu = 0, nr_vcpu = 0;

    printf("cpu_bw_perc = %.2lf | ", cpu_bw_perc);

    if (-1 == virNodeGetInfo(conn, &nodeinfo))
        goto cleanup;
    nr_pcpu = nodeinfo.cpus;
    printf("nr_pcpu = %u | ", nr_pcpu);

    if (-1 == virDomainGetInfo(domain, &dominfo))
        goto cleanup;
    nr_vcpu = dominfo.nrVirtCpu;
    printf("nr_vcpu = %u | ", nr_vcpu);

    if (-1 == get_vcpu_period(domain, &cpu_period))
       goto cleanup;
       printf("cpu_period = %llu | ", cpu_period);

    if (cpu_bw_perc <= (double)(nr_vcpu*100/nr_pcpu)) {
        /* 
         * Compute the new quota which should be allocated to the domain, the 
         * quota is applied to each vcpu, thus the cpu_bw_percentage should be
         * divided by nr_vcpu.
         */
        cpu_quota = 
            (long long)(cpu_bw_perc / nr_vcpu * nr_pcpu * cpu_period);
        printf("Choose 1:cpu_quota = %lld\n", cpu_quota);
    }
    else
    {
        /* 
         * allocate at most (nr_vcpu / nr_pcpu) bandwidth for the domain
         */
        cpu_quota = (long long)(cpu_period);
        printf("Choose 2:cpu_quota = %lld\n", cpu_quota);
    }

    if (-1 == set_vcpu_quota_ll(domain, &cpu_quota))
        goto cleanup;

    ret = 0;

cleanup:
    return ret;
}

/* 
 * runtime memory allocate for domain
 */
int allocmem(virDomainPtr domain, double mem_perc)
{
    int ret = -1;
    struct phy_statistics *pinfo = 
        (struct phy_statistics *)malloc(sizeof(struct phy_statistics) * 1);  

    unsigned long vmemsz;
    virDomainInfoPtr dominfo;
    dominfo = (virDomainInfo *)malloc(sizeof(virDomainInfo));
    get_phy_memstat(pinfo);
    vmemsz = (unsigned long)(mem_perc * pinfo->memtotal / 100);
    printf("mem_perc=%.2lf |memtotal=%lu |vmemsz=%lu     ", 
            mem_perc, pinfo->memtotal, vmemsz);

    if (-1 == virDomainGetInfo(domain, dominfo)) 
        goto cleanup;
    /* memory size can't be too small, or OOM would kill the VM process */
    if (vmemsz < VIR_MEM_LOW_BOUND_IN_BYTES) {
        vmemsz = VIR_MEM_LOW_BOUND_IN_BYTES;
    } else if (vmemsz > dominfo->maxMem) {
    /* memory size < maxMem, for qemu, maxMem can't be changed at runtime */ 
        fprintf(stderr, "allocate %lu memory to domain which is larger" 
                "than the maxMem [%lu]\n", vmemsz, dominfo->maxMem);
        goto cleanup;
    }

    printf("-----finally,we set vmemsz=%lu\n", vmemsz);

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
int allocrd(virDomainPtr domain, const char *dsk, double disk_rbps)
{
    int ret = -1;
    char str_rbps[64];
    
    snprintf(str_rbps, 64, "%s, %lf", dsk, disk_rbps);
    
    if (!set_blkio_read_bytes(domain, str_rbps))
        goto cleanup;
        
    ret = 0;
    
cleanup:
    return ret;
}

int allocwr(virDomainPtr domain, const char *dsk, double disk_wbps)
{
    int ret = -1;
    char str_wbps[64];
    
    snprintf(str_wbps, 64, "%s, %lf", dsk, disk_wbps);
    
    if (!set_blkio_write_bytes(domain, str_wbps))
        goto cleanup;
        
    ret = 0;
    
cleanup:
    return ret;
}


double min(double a, double b)
{
    return (a <= b) ? a : b;
}

/* the policy to schedule CPU resources */
void schedcpu(virConnectPtr conn, struct vm_info *vinfo, double capcpu, 
        double *tccpu, double *wr, double *cpu_est, int n)
{
    int i, ret=0;
    double availcpu = 0.0, ttaken = 0.0,sum_cpu_est = 0.0;

    for (i = 0; i < n; i++) {
        ttaken += min(cpu_est[i], tccpu[i]);
        sum_cpu_est += cpu_est[i];
    }
    
    for (i = 0; i < n; i++){
        if (cpu_est[i] > tccpu[i])
           ret = -1;
           break;
    }
   
    availcpu = capcpu - ttaken;

    if ((sum_cpu_est <= capcpu)&&(ret == 0)){
       for (i=0;i<n;i++)
           alloccpu(conn, vinfo[i].dp, cpu_est[i]);
    } else if (availcpu >= 0.0){
            for (i = 0;i < n;i++)
                alloccpu(conn, vinfo[i].dp, 
                        min(cpu_est[i],tccpu[i]) + wr[i]*availcpu);
    } else 
            availcpu = 0.0;
}
 /* attention: should have some upper/lower bound for memory allocation 
 * TODO: slice memory allocation, each slice takes up some amount of 
 * memory, which should be allocated accordingly.
 */
void schedmem(struct vm_info *vinfo, double capmem, double *tcmem, double *wr,
        double *mem_est, int n)
{
    int i, ret = 0;
    double availmem = 0.0, ttaken = 0.0,sum_mem_est = 0.0;
    
    for (i = 0; i < n; i++) {
        ttaken += min(mem_est[i], tcmem[i]);
        sum_mem_est += mem_est[i];
    }
    availmem = capmem - ttaken;

    for (i = 0; i < n; i++){
        if (mem_est[i] > tcmem[i])
        ret = -1;
        break;
    }

    if ((sum_mem_est <= capmem) && (ret == 0)) {
       for (i = 0; i < n; i++)
           allocmem(vinfo[i].dp, mem_est[i]);
    } else if (availmem >= 0.0) {
       for (i=0; i<n; i++)
           allocmem(vinfo[i].dp, min(mem_est[i], tcmem[i]) + wr[i] * availmem); 
    } else 
               availmem = 0.0;
} 

/*
 * different from cpu scheduling, have to schedule read/write respectively.
 */
void schedrd(struct vm_info *vinfo, double caprd, double *tcrd, double *wr,
        double *rd_est, int n)
{
#define DSKNAME "/dev/sda"
    int i, ret = 0;
    double availrd = 0.0, ttaken = 0.0,sum_rd_est = 0.0;

    for (i = 0; i < n; i++) {
        ttaken += min(rd_est[i], tcrd[i]);
        sum_rd_est += rd_est[i];
    }
    availrd = caprd - ttaken;
    
    for (i = 0; i < n; i++) {
        if (rd_est[i] > tcrd[i])
            ret = -1;
        break;
    }
    if ((sum_rd_est <= caprd)&&(ret == 0)) {
       for (i = 0; i<n; i++)
            allocrd(vinfo[i].dp,DSKNAME, rd_est[i]);
           } else if (availrd >= 0.0) {
            for (i=0; i<n; i++)
                allocrd(vinfo[i].dp,DSKNAME, min(rd_est[i], tcrd[i]) + wr[i] * availrd); 
           } else 
               availrd = 0.0;
} 

void schedwr(struct vm_info *vinfo, double capwr, double *tcwr, double *wr,
        double *wr_est, int n)
{
#define DSKNAME "/dev/sda"
    int i, ret = 0;
    double availwr = 0.0, ttaken = 0.0,sum_wr_est = 0.0;

    for (i = 0; i < n; i++) {
        ttaken += min(wr_est[i], tcwr[i]);
        sum_wr_est += wr_est[i];
    }
    availwr = capwr - ttaken;
    
    for (i = 0; i < n; i++){
        if (wr_est[i] > tcwr[i])
        ret = -1;
        break;
    }
    if ((sum_wr_est <= capwr)&&(ret == 0)){
       for (i = 0; i<n; i++)
            allocwr(vinfo[i].dp,DSKNAME, wr_est[i]);
           } else if (availwr >= 0.0) {
            for (i=0; i<n; i++)
                allocrd(vinfo[i].dp,DSKNAME, min(wr_est[i], tcwr[i]) + wr[i] * availwr); 
           } else 
               availwr = 0.0;
} 
    

void update_ac(struct mach_load *ac, struct mach_load *tc, 
        struct ringbuffer *vmobs, int index, int WINSZ)
{
    struct mach_load lload, kload;
    
    int pos = (vmobs->end - 1 + WINSZ) % WINSZ;
    lload = vmobs->buff[pos];
/*    printf("lload.cpu_load = %6.2lf\n", lload.cpu_load);   */

    if (index <= WINSZ) {
       printf("tc.cpu_load = %6.2lf\n", tc->cpu_load);
       printf("lload.cpu_load = %6.2lf\n", lload.cpu_load);
       ac->cpu_load += tc->cpu_load-lload.cpu_load;
       ac->mem_load += tc->mem_load-lload.mem_load;
       ac->rd_load += tc->rd_load-lload.rd_load;   
       ac->wr_load += tc->wr_load-lload.wr_load;
       printf("ac.cpu_load = %6.2lf\n", ac->cpu_load);
    }
    /* else */

}


void cal_wr(struct mach_load ac, struct mach_load sum_ac, double wr_cpu, 
        double wr_mem, double wr_rd, double wr_wr)
{
    wr_cpu = ac.cpu_load/sum_ac.cpu_load;
    wr_mem = ac.mem_load/sum_ac.mem_load;
    wr_rd = ac.rd_load/sum_ac.rd_load;
    wr_wr = ac.wr_load/sum_ac.wr_load;
}    

