#include <libvirt/libvirt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/rc.h"
#include <errno.h>

int main()
{
    int i;
    virConnectPtr conn = virConnectOpen("qemu:///system");

    char *cap = (char *)malloc(sizeof(1024)*sizeof(char));
    cap = virConnectGetCapabilities(conn);
    printf("%s\n", cap);

    char *hostname = (char *)malloc(sizeof(32)*sizeof(char));
    hostname = virConnectGetHostname(conn);
    printf("%s\n", hostname);

    unsigned long libver;
    virConnectGetLibVersion(conn, &libver);
    int major = libver / 1000000;
    int tmp = libver % 1000000;
    int minor = tmp / 1000;
    int release = tmp % 1000;
    printf("%d.%d.%d\n", major, minor, release);

    char *sysinfo = (char *)malloc(1024*sizeof(char));
    sysinfo = virConnectGetSysinfo(conn, 0);
    printf("%s\n", sysinfo);

    //virNode* API
    virNodeInfoPtr info = (virNodeInfoPtr)malloc(sizeof(*info));
    virNodeGetInfo(conn, info);
    printf("model=%s\nmemory=%ld KB\ncpus=%d\nmhz=%d HZ\nnodes=%d\nsockets=%d\ncores=%d\nthreads=%d\n", info->model, info->memory,
            info->cpus, info->mhz, info->nodes, info->sockets, info->cores, info->threads);

    int cpunr = VIR_NODE_CPU_STATS_ALL_CPUS;
    virNodeCPUStatsPtr nodecpuparams = NULL;
    int nodecpunparams = 0;
    if (virNodeGetCPUStats(conn, cpunr, NULL, &nodecpunparams, 0) == 0 && nodecpunparams != 0) {
        nodecpuparams = (virNodeCPUStatsPtr)malloc(sizeof(virNodeCPUStats) * nodecpunparams);
        virNodeGetCPUStats(conn, cpunr, nodecpuparams, &nodecpunparams, 0);
    }
    for (i = 0; i < nodecpunparams; i++) {
        printf("%6s : %lld\n", nodecpuparams[i].field, nodecpuparams[i].value);
    }

    virNodeDevicePtr nodedevptr = virNodeDeviceLookupByName(conn, "block_sda_ST3500418AS_5VMNK7AD");
    printf("device name: %s\n", virNodeDeviceGetName(nodedevptr));

    printf("Free Memory: %lld KB\n", virNodeGetFreeMemory(conn)/1024);

    int cellnr = VIR_NODE_MEMORY_STATS_ALL_CELLS;
    virNodeMemoryStatsPtr nodememparams = NULL;
    int nodememnparams = 0;
    if (virNodeGetMemoryStats(conn, cellnr, NULL, &nodememnparams, 0) == 0 && nodememnparams != 0) {
        nodememparams = malloc(sizeof(virNodeMemoryStats) * nodememnparams);
        memset(nodememparams, 0, sizeof(virNodeMemoryStats) * nodememnparams);
        virNodeGetMemoryStats(conn, cellnr, nodememparams, &nodememnparams, 0);
    }
    for (i = 0; i < nodememnparams; i++) {
        printf("%8s : %lld KB\n", nodememparams[i].field, nodememparams[i].value);
    }

    /* 这个函数威力太大，测试时玩玩~ */
    //virNodeSuspendForDuration(conn, VIR_NODE_SUSPEND_TARGET_MEM, 100, 0);

    int start_cpu = 1;
    int ncpus = 1;
    int domcpunparams = 0;
    virTypedParameterPtr domcpuparams = NULL;

    /* NEED to get domain pointer here first */
    int actdom_nr = virConnectNumOfDomains(conn);
    int *actdomids = (int *)malloc(sizeof(int) * actdom_nr);
    virConnectListDomains(conn, actdomids, actdom_nr);
    virDomainPtr *dp = malloc(sizeof(virDomainPtr *)*actdom_nr);
    virTypedParameterPtr params = NULL;
    int nparams = 0;
    for (i = 0; i < actdom_nr; i++) { 
        dp[i] = virDomainLookupByID(conn, actdomids[i]);
        get_schedinfo(dp[i], &params, &nparams);
        print_typed_parameters(params, nparams);
    }
    domcpunparams = virDomainGetCPUStats(dp[0], NULL, 0, start_cpu, 1, 0);
    domcpuparams = malloc(sizeof(virTypedParameter) * domcpunparams);
    memset(domcpuparams, 0, sizeof(virTypedParameter) * domcpunparams);
    virDomainGetCPUStats(dp[0], domcpuparams, domcpunparams, start_cpu, ncpus, 0);
    //virDomainGetCPUStats(dp[0], domcpuparams, domcpunparams, -1, 1, 0);
    print_typed_parameters(domcpuparams, domcpunparams);

    virDomainInfoPtr dominfo = (virDomainInfoPtr)malloc(sizeof(virDomainInfo));
    memset(dominfo, 0, sizeof(struct virDomainInfoPtr *));
    virDomainGetInfo(dp[0], dominfo);
    printf("cpuTime=%lld\n", dominfo->cpuTime);

    char *domname = (char *)malloc(sizeof(char)*20);
    domname = virDomainGetName(dp[0]);
    printf("operating domain [%s]\n", domname);

    virDomainMemoryStatStruct stats[VIR_DOMAIN_MEMORY_STAT_NR];
    int nr_stats = virDomainMemoryStats(dp[0], stats, VIR_DOMAIN_MEMORY_STAT_NR, 0);
    if (nr_stats == -1) {
        fprintf(stderr, "Failed to get memory statistics for domain");
        exit(errno);
    }
    for (i = 0; i < nr_stats; i++) {
        if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_SWAP_IN)
            printf("swap_in %llu\n", stats[i].val);
        if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_SWAP_OUT)
            printf("swap_out %llu\n", stats[i].val);
        if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_MAJOR_FAULT)
            printf("major_fault %llu\n", stats[i].val);
        if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_MINOR_FAULT)
            printf("minor_fault %llu\n", stats[i].val);
        if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_UNUSED)
            printf("unused %llu\n", stats[i].val);
        if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_AVAILABLE)
            printf("available %llu\n", stats[i].val);
        if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON)
            printf("actual %llu\n", stats[i].val);
        if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_RSS)
            printf("rss %llu\n", stats[i].val);
    }


    return 0;
}
