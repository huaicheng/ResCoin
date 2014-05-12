/*
 * working on CentOS6.5, kernel 2.6.32-431.1.2.0.1.el6.x86_64
 */

#include <libvirt/libvirt.h>    /* libvirt API */
#include <libvirt/virterror.h>  /* libvirt error handling */
#include <sys/time.h>           /* gettimeofday() */
#include <sys/types.h>          /* time_t etc. */
#include <stdbool.h>
#include <unistd.h>
#include <time.h>               /* time() */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>              /* system error number */

#define DOMAIN_XML_SIZE 4096    /* in bytes */

#define TIME_INTERVAL   10      /* in seconds */
#define ETHERNET        "br0"
#define DISK            "sda"

char *hypervisor = "qemu:///system";
int active_domain_num = 0;
int nr_cores = 1;    /* by default, we suppose there is only one core in the host */

typedef unsigned long long ull;
/*
 * Structure for CPU statistics.
 * In activity buffer: First structure is for global CPU utilisation ("all").
 * Following structures are for each individual CPU (0, 1, etc.)
 */
struct stats_cpu {
	ull cpu_user		__attribute__ ((aligned (16)));
	ull cpu_nice		__attribute__ ((aligned (16)));
	ull cpu_sys		__attribute__ ((aligned (16)));
	ull cpu_idle		__attribute__ ((aligned (16)));
	ull cpu_iowait		__attribute__ ((aligned (16)));
	ull cpu_steal		__attribute__ ((aligned (16)));
	ull cpu_hardirq		__attribute__ ((aligned (16)));
	ull cpu_softirq		__attribute__ ((aligned (16)));
	ull cpu_guest		__attribute__ ((aligned (16)));
	ull cpu_guest_nice	__attribute__ ((aligned (16)));
};

/* Structure for block devices statistics */
struct stats_disk {
	ull nr_ios	__attribute__ ((aligned (16)));
	unsigned long rd_sect		__attribute__ ((aligned (16)));
	unsigned long wr_sect		__attribute__ ((aligned (8)));
	unsigned int rd_ticks		__attribute__ ((aligned (8)));
	unsigned int wr_ticks		__attribute__ ((packed));
	unsigned int tot_ticks		__attribute__ ((packed));
	unsigned int rq_ticks		__attribute__ ((packed));
	unsigned int major		__attribute__ ((packed));
	unsigned int minor		__attribute__ ((packed));
};

/* Structure for network interfaces statistics */
struct stats_net_dev {
	ull rx_packets		__attribute__ ((aligned (16)));
	ull tx_packets		__attribute__ ((aligned (16)));
	ull rx_bytes		__attribute__ ((aligned (16)));
	ull tx_bytes		__attribute__ ((aligned (16)));
	ull rx_compressed	__attribute__ ((aligned (16)));
	ull tx_compressed	__attribute__ ((aligned (16)));
	ull multicast		__attribute__ ((aligned (16)));
	unsigned int       speed		__attribute__ ((aligned (16)));
	char 	 interface[16]	__attribute__ ((aligned (4)));
	char	 duplex;
};

typedef struct phy_statistics {
    /* cpu related stat */
    ull user; /* in ticks */
    ull nice;
    ull system;
    ull idle;
    ull iowait;
    ull irq;
    ull softirq;
    ull steal;
    ull guest;
    ull guest_nice; /* since Linux 2.6.33 */

    /* memory related stat */
    unsigned long memtotal;
    unsigned long memfree;
    unsigned long buffers;
    unsigned long cached;

    /* disk related stat */
    ull rd_sectors;
    ull  wr_sectors;

    /* net related stat */
    ull rx_bytes;
    ull tx_bytes;

}phy_statistics;

typedef struct phy_info {
    char *hostname;
    int hostname_length;
    FILE *fp;
}phy_info;

typedef struct vm_info {
    int id;
    virDomainPtr dp;
    const char *domname;
    char *blkname;
    char *ifname;
    FILE *fp;
}vm_info;

typedef struct vm_statistics {
    ull cpu_time;
    unsigned long maxmem;
    unsigned long curmem;
    unsigned long rss; // resident set size
    long long rd_bytes;
    long long wr_bytes;
    long long rx_bytes;
    long long tx_bytes;
}vm_statistics;

/* used for both physical and VMs */
typedef struct mach_load {
    double cpu_load; /* 0~100 */
    double mem_load; /* 0~100 */
    double rd_load;  /* Kbps */
    double wr_load;  /* Kbps */
    double rx_load;  /* Kbps */
    double tx_load;  /* Kbps */
}mach_load;


/*
 * must be called before using struct vm_info
 */
void init_vm_info(struct vm_info *vminfo)
{
    vminfo->id = -1;
    vminfo->dp = NULL;
    /* get domname by virDomainGetName, need not allocate space for domname field */
    /* vminfo->domname = (char *)malloc(sizeof(char) * 100); */
    vminfo->blkname = (char *)malloc(sizeof(char) * 100);
    vminfo->ifname = (char *)malloc(sizeof(char) * 100);
    /* memset(vminfo->domname, '\0', sizeof(vminfo->domname)); */
    memset(vminfo->blkname, '\0', 100);
    memset(vminfo->ifname, '\0', 100);
    vminfo->fp = NULL;
}

void init_phy_info(struct phy_info *phyinfo)
{
    phyinfo->hostname_length = 20;
    phyinfo->hostname = (char *)malloc(sizeof(char) * phyinfo->hostname_length);
    memset(phyinfo->hostname, '\0', phyinfo->hostname_length);
    phyinfo->fp = NULL;
}

/* 
 * print vm info 
 */
void print_vm_info(struct vm_info *vminfo)
{
    printf("%-4s | %-10s | %-8s | %-50s | %-8s | %-10s\n", 
            "ID", "domain", "name", "blkname", "ifname", "FILE *fp");
    printf("%-4d | %-10p | %-8s | %-50s | %-8s | %-10p\n", vminfo->id, vminfo->dp, 
            vminfo->domname, vminfo->blkname, vminfo->ifname, vminfo->fp);
}

/*
 * print vm statistics
 */
void print_vm_statistics(struct vm_statistics *vm_stat, struct vm_info *vminfo)
{
    printf("%-8s: %-20s %-8s %-8s %-12s %-12s %-12s %-12s\n", "domname", "cpu_time", 
            "maxmem", "curmem", "rd_bytes", "wr_bytes", "rx_bytes", "tx_bytes");
    printf("%-8s: %-20lld %-8ld %-8ld %-12lld %-12lld %-12lld %-12lld\n", vminfo->domname,
            vm_stat->cpu_time, vm_stat->maxmem, vm_stat->curmem, 
            vm_stat->rd_bytes, vm_stat->wr_bytes, vm_stat->rx_bytes, vm_stat->tx_bytes);
}

/* 
 * print vm workload
 */
void print_mach_load(struct mach_load *vmload)
{
    printf("%-6.2lf %-6.2lf %-8.2lf %-8.2lf %-8.2lf %-8.2lf\n", vmload->cpu_load, vmload->mem_load,
            vmload->rd_load, vmload->wr_load, vmload->rx_load, vmload->tx_load);
}

/* 
 * match the "source file" field to get the PATH of the virtual disk 
 * WARNING: currently, only consider the ONLY ONE virtual disk situation here
 */
void get_vm_blkpath(char *xml, char *blkpath)
{
    int i;
    char *p = strstr(xml, "source file");
    if (NULL == p)
        return;
    char *q = NULL, *r = NULL;
    for (; p++; p) {
        if (*p == '\'') {
            q = p + 1;
            break;
        }
    }
    for (p = q; p++; p) {
        if (*p == '\'') {
            r = p - 1;
            break;
        }
    }
    for (i = 0, p = q; p <= r; p++, i++)
        blkpath[i] = *p;
}

/* 
 * match the "vnet" field to get the PATH of the virtual interface 
 * WARNING: currently, only consider the ONLY ONE virtual interface situation here
 */
void get_vm_ifpath(char *xml, char *ifpath)
{
    int i = 0;
    char *q = NULL, *r = NULL;
    char *p = strstr(xml, "vnet");
    if (NULL == p)
        return;
    for (q = p; *q != '\''; q++) 
        i++;
    for (i = 0, r = p; r < q; r++, i++)
        ifpath[i] = *r;
}

void get_vm_dominfo(struct vm_statistics *vm_stat, struct vm_info *vminfo)
{
    int ret;
    int i;
    virDomainMemoryStatStruct stats[VIR_DOMAIN_MEMORY_STAT_NR];
    int nr_stats = virDomainMemoryStats(vminfo->dp, stats, VIR_DOMAIN_MEMORY_STAT_NR, 0);
    if (nr_stats == -1) {
        fprintf(stderr, "Failed to get VM memory statistics for");
        exit(errno);
    }
    for (i = 0; i < nr_stats; i++) 
        if (stats[i].tag == VIR_DOMAIN_MEMORY_STAT_RSS) {
            vm_stat->rss = stats[i].val;
            break;
        }

    virDomainInfoPtr dominfo = (virDomainInfoPtr)malloc(sizeof(virDomainInfo));
    ret = virDomainGetInfo(vminfo->dp, dominfo);
    if (-1 == ret) {
        fprintf(stderr, "failed to get domaininfo of VMs\n");
        exit(VIR_FROM_DOM);
    }
    vm_stat->cpu_time = dominfo->cpuTime; /* in nanoseconds */
    vm_stat->maxmem = dominfo->maxMem;    /* in KiloBytes */
    vm_stat->curmem = dominfo->memory;    /* in KiloBytes */

    free(dominfo);
}

void get_vm_blkstat(struct vm_statistics *vm_stat, struct vm_info *vminfo)
{
    int ret;
    virDomainBlockStatsPtr blkstat = (virDomainBlockStatsPtr)malloc(sizeof(virDomainBlockStatsStruct));
    ret = virDomainBlockStats(vminfo->dp, vminfo->blkname, 
            blkstat, sizeof(virDomainBlockStatsStruct));
    if (-1 == ret) {
        fprintf(stderr, "failed to get domblkstats of VMs\n");
        exit(VIR_FROM_DOM);
    }
    vm_stat->rd_bytes = blkstat->rd_bytes;
    vm_stat->wr_bytes = blkstat->wr_bytes;

    free(blkstat);
}

void get_vm_ifstat(struct vm_statistics *vm_stat, struct vm_info *vminfo)
{
    int ret;
    virDomainInterfaceStatsPtr ifstat = (virDomainInterfaceStatsPtr)malloc(sizeof(virDomainInterfaceStatsStruct));
    ret = virDomainInterfaceStats(vminfo->dp, vminfo->ifname, 
            ifstat, sizeof(virDomainInterfaceStatsStruct));
    if (-1 == ret) {
        fprintf(stderr, "failed to get domifstats of VMs\n");
        exit(VIR_FROM_DOM);
    }
    vm_stat->rx_bytes = ifstat->rx_bytes;
    vm_stat->tx_bytes = ifstat->tx_bytes;

    free(ifstat);
}

/* collect VM's workload statistics */
void get_vm_workload(struct vm_statistics *vm_stat, struct vm_info *vminfo)
{
    int ret;

    /* get domaininfo of each active VM, including CPU and MEM infomation */
    get_vm_dominfo(vm_stat, vminfo);

    /* WARNING: suppose we have only 1 vdisk here */
    get_vm_blkstat(vm_stat, vminfo);

    /* WARNINg: suppose we have only 1 virtual interface */
    get_vm_ifstat(vm_stat, vminfo);

    //print_vm_statistics(vm_stat, vminfo);
}

void calculate_vm_load(struct mach_load *vmload, struct vm_statistics *vm_stat_before, 
        struct vm_statistics *vm_stat_after, long long microsec, unsigned long total_mem)
{

    long long delta_cpu_time = vm_stat_after->cpu_time - vm_stat_before->cpu_time;
    long long delta_rd_bytes = vm_stat_after->rd_bytes - vm_stat_before->rd_bytes;
    long long delta_wr_bytes = vm_stat_after->wr_bytes - vm_stat_before->wr_bytes;
    long long delta_rx_bytes = vm_stat_after->rx_bytes - vm_stat_before->rx_bytes;
    long long delta_tx_bytes = vm_stat_after->tx_bytes - vm_stat_before->tx_bytes;

    /* calculate the corresponding workload of each VM using the monitored statistics data */

    /* (1). %CPU = 100 × delta_cpu_time / (time × nr_cores × 10^9) 
     * delta_cpu_time represent time differences domain get to run during time "time"
     */
    vmload->cpu_load = delta_cpu_time * 1.0 / 1000 / microsec * 100 / nr_cores;  

    /* (2). %MEM: use the rss size as the total used memory of VM */
    vmload->mem_load = (vm_stat_after->rss) * 1.0 / total_mem * 100; 
    //vmload->mem_load = (vm_stat_after->curmem)  * 1.0 / total_mem * 100;

    /* (3). vm disk read rate */
    vmload->rd_load = delta_rd_bytes * 1.0 / 1024 / microsec * 1000000;

    /* (4). vm disk write rate */
    vmload->wr_load = delta_wr_bytes * 1.0 / 1024 / microsec * 1000000;

    /* (5). vm net rx rate */
    vmload->rx_load = delta_rx_bytes * 1.0 / 1024 / microsec * 1000000;

    /* (6). vm net tx rate */
    vmload->tx_load = delta_tx_bytes * 1.0 / 1024 / microsec * 1000000;
}

void create_vm_rst_file(struct vm_info *vminfo)
{
    const char *suffix = ".rst";
    int domname_length = strlen(vminfo->domname);
    int fname_length = domname_length + strlen(suffix) + 1;
    char *fname = (char *)malloc(sizeof(char) * fname_length);
    memset(fname, '\0', fname_length);
    strncpy(fname, vminfo->domname, strlen(vminfo->domname));
    strncat(fname, suffix, strlen(suffix));
    vminfo->fp = fopen(fname, "w");
    setlinebuf(vminfo->fp);
    if (NULL == vminfo->fp) {
        fprintf(stderr, "failed to create result file %s [%s]\n", fname, strerror(errno));
        exit(errno);
    }

    free(fname);
}

void create_phy_rst_file(struct phy_info *phyinfo)
{
    const char *suffix = ".rst";
    gethostname(phyinfo->hostname, phyinfo->hostname_length);
    int hname_len = strlen(phyinfo->hostname);
    int fname_length = hname_len + strlen(suffix) + 1;
    char *fname = (char *)malloc(sizeof(char) * fname_length);
    memset(fname, '\0', fname_length);
    strncpy(fname, phyinfo->hostname, hname_len);
    strncat(fname, suffix, strlen(suffix));
    phyinfo->fp = fopen(fname, "w");
    setlinebuf(phyinfo->fp);
    if (NULL == phyinfo->fp) {
        fprintf(stderr, "failed to create result file %s [%s]\n", fname, strerror(errno));
        exit(errno);
    }
}

int match_nic(char *buffer, char *nic)
{
    int nic_length = strlen(nic);
    int buf_size = strlen(buffer);
    int i = 0, j = 0;
    
    for (i = 0; (i < buf_size) && ((i-j) < nic_length); i++) {
        if (buffer[i] == ' ') {
            j++;
            continue;
        }
        if (buffer[i] == nic[i-j])
            continue;
        else
            break;
    }
    if (buffer[i] == ':')
        return 1;
    else
        return 0;
}

void extract_nic_bytes(char *buffer, ull *rx_bytes, ull *tx_bytes)
{
    int i, j, k;
    int has_space = 1;
    char rx[100] = {'\0'};
    int buf_size = strlen(buffer);
    
    for (i = 0; i < buf_size; i++)
        if (buffer[i] == ':')
            break;
    /* do no more checking here for the noexistence situation of wrong ethernet line */
    i++;
    if (buffer[i] != ' ')
        has_space = 0;
    if (has_space) {
        sscanf(buffer, "%*s %lld %*d %*d %*d %*d %*d %*d %*d %lld", rx_bytes, tx_bytes);
    }
    else {
        k = 0;
        for (j = i; buffer[j] != ' '; j++)
            rx[k++] = buffer[j];
        *rx_bytes = atol(rx);
        sscanf(buffer, "%*s %*d %*d %*d %*d %*d %*d %*d %llu", tx_bytes);
        //printf("in extract_nic_bytes, rx=%ld, tx=%ld\n", *rx_bytes, *tx_bytes);
    }
    
    //printf("rx=%ld, tx=%ld\n", *rx_bytes, *tx_bytes); 
}


/* 
 * get cpu info from /proc/stat
 */
void get_phy_cpu_stat(struct phy_statistics *phy_stat)
{
    char line[8192];
    FILE *fp = fopen("/proc/stat", "r");
    if (NULL == fp) {
        fprintf(stderr, "failed to open /proc/stat [%s]\n", strerror(errno));
        exit(errno);
    }

    memset(line, 0, 8192);
    if (NULL == fgets(line, sizeof(line), fp)) {
        fprintf(stderr, "failed to read info from /proc/stat [%s]\n", strerror(errno));
        exit(errno);
    }

    sscanf(line, "%*s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n", 
            &phy_stat->user, 
            &phy_stat->nice, 
            &phy_stat->system,
            &phy_stat->idle, 
            &phy_stat->iowait, 
            &phy_stat->irq, 
            &phy_stat->softirq, 
            &phy_stat->steal, 
            &phy_stat->guest,
            &phy_stat->guest_nice);

    fclose(fp);
}

void get_phy_mem_stat(struct phy_statistics *phy_stat)
{
    int i;
    FILE *fp = NULL;
    for (i = 0; i < 100; i++) {
        fp = fopen("/proc/meminfo", "r");
        if (fp)
            break;
    }
    if (NULL == fp) { 
        fprintf(stderr, "failed to open /proc/meminfo [%s]\n", strerror(errno));
        exit(errno);
    }
    fscanf(fp, "%*s %ld %*s\n%*s %ld %*s\n%*s %ld %*s\n%*s %ld", &phy_stat->memtotal, 
            &phy_stat->memfree, &phy_stat->buffers, &phy_stat->cached);
    fclose(fp);
}

/*
 * user must handle errors by themselves
 */
void get_phy_disk_stat(struct phy_statistics *phy_stat, const char *disk)
{
    char buf[100] = {'\0'};
    char device[20] = {'\0'};
    FILE *fp = fopen("/proc/diskstats", "r");
    if (NULL == fp) {
        fprintf(stderr, "faild to open /proc/diskstats [%s]\n", strerror(errno));
        exit(errno);
    }
    while (fgets(buf, sizeof(buf), fp)) {
        sscanf(buf, "%*d %*d %s", device);
        if (0 == strcmp(disk, device)) {
            sscanf(buf, "%*u %*u %*s %*u %*u %Lu %*u %*d %Lu", &phy_stat->rd_sectors, &phy_stat->wr_sectors);
            break;
        }
        memset(device, '\0', sizeof(device));
        memset(buf, '\0', sizeof(buf));
    }

    fclose(fp);
}

void get_phy_net_stat(struct phy_statistics *phy_stat, const char *nic)
{
    char buf[1024] = {'\0'};
    char device[20] = {'\0'}; 
    int match = 0;

    FILE *fp = fopen("/proc/net/dev", "r");
    if (NULL == fp) {
        fprintf(stderr, "faild to open /proc/net/dev [%s]\n", strerror(errno));
        exit(errno);
    }
    while (fgets(buf, sizeof(buf), fp)) {
        if (match_nic(buf, ETHERNET)) {
            match = 1;
            break;
        }
        memset(buf, '\0', sizeof(buf));
    }
    if (match == 0) {
        fprintf(stderr, "couldn't resolve [%s] in /proc/net/dev, make sure you have the NIC\n", ETHERNET);
        exit(errno);
    }
    //printf("%s\n", buf);
    extract_nic_bytes(buf, &phy_stat->rx_bytes, &phy_stat->tx_bytes);

    fclose(fp);
}

void get_phy_workload(struct phy_statistics *phy_stat)
{
    char *disk = DISK;
    char *nic = ETHERNET;

    /* get physical cpu statistics */
    get_phy_cpu_stat(phy_stat);

    /* get physical memory statistics */
    get_phy_mem_stat(phy_stat);

    /* get physical disk statistics */
    get_phy_disk_stat(phy_stat, disk);

    /* get physical network statistics */
    get_phy_net_stat(phy_stat, nic);
}

/* sum up user+nice+sys+iowait+idle+irq+softirq+steal, precluding guest and guest_nice */
ull sum_cpu_stats(struct phy_statistics *phy_stats)
{
    ull cpu_ticks = 
        phy_stats->user + 
        phy_stats->nice + 
        phy_stats->system + 
        phy_stats->idle + 
        phy_stats->iowait + 
        phy_stats->irq + 
        phy_stats->softirq +
        phy_stats->steal /*+ 
        phy_stats->guest + 
        phy_stats->guest_nice*/;
    return cpu_ticks;
}

/*
 * caculate physical machine's workload
 */
void calculate_phy_load(struct mach_load *phyload, struct phy_statistics *phy_stat_before, 
        struct phy_statistics *phy_stat_after, long microsec)
{
    ull cpu_ticks_before = sum_cpu_stats(phy_stat_before);
    ull cpu_ticks_after = sum_cpu_stats(phy_stat_after);

    /* total elapsed cpu time in ticks, equavelent to the difference of gettimeofday */
    ull delta_cpu_time = cpu_ticks_after - cpu_ticks_before; 
    ull delta_idle_time = phy_stat_after->idle - phy_stat_before->idle;

    /* get memory unused(including the "free+buffers+cached" */
    unsigned long mem_unused = phy_stat_after->memfree + phy_stat_after->buffers + phy_stat_after->cached;

    ull delta_rd_bytes = (phy_stat_after->rd_sectors - phy_stat_before->rd_sectors) * 512;
    ull delta_wr_bytes = (phy_stat_after->wr_sectors - phy_stat_before->wr_sectors) * 512;

    ull delta_rx_bytes = phy_stat_after->rx_bytes - phy_stat_before->rx_bytes;
    ull delta_tx_bytes = phy_stat_after->tx_bytes - phy_stat_before->tx_bytes;

    /* calculate the corresponding workload of each VM using the monitored statistics data */

    /* (1) CPU%: we take non-idle cpu time as the time cpu busying running tasks */
    phyload->cpu_load = 100 - delta_idle_time * 1.0 / delta_cpu_time * 100; /* 1~100(%) */

    /* (2) MEM%: get the memory usage at the time poniter */
    phyload->mem_load = 100 - mem_unused * 1.0 / phy_stat_after->memtotal * 100; /* 1~100(%) */

    /* (3) DISK read rate */
    phyload->rd_load = delta_rd_bytes * 1.0 / 1024 / microsec * 1000000; /* in Kbps */

    /* (4) DISK write rate */
    phyload->wr_load = delta_wr_bytes * 1.0 / 1024 / microsec * 1000000; /* in Kbps */

    /* (5) NET rx rate */
    phyload->rx_load = delta_rx_bytes * 1.0 / 1024 / microsec * 1000000; /* in Kbps */

    /* (6) NET tx rate */
    phyload->tx_load = delta_tx_bytes * 1.0 / 1024 / microsec * 1000000; /* in Kbps */
}



int main(int argc, char **argv)
{
    virConnectPtr conn = NULL;
    int *active_domain_ids = NULL;
    virNodeInfoPtr nodeinfo = NULL;
    long index = 0;
    time_t curtime = 0;
    int ret, i;

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
    struct vm_info *vminfo = (struct vm_info *)malloc(sizeof(struct vm_info) * active_domain_num);
    struct phy_info *phyinfo = (struct phy_info *)malloc(sizeof(struct phy_info) * 1);

    init_phy_info(phyinfo);
    create_phy_rst_file(phyinfo);

    for (i = 0; i < active_domain_num; i++) {

        init_vm_info(&vminfo[i]);

        vminfo[i].id = active_domain_ids[i]; //

        vminfo[i].dp = virDomainLookupByID(conn, vminfo[i].id); //
        if (NULL == vminfo[i].dp) {
            fprintf(stderr, "failed to get the domain instance with ID: %d\n", vminfo[i].id);
            exit(VIR_ERR_INVALID_DOMAIN);
        }

        vminfo[i].domname = virDomainGetName(vminfo[i].dp); //
        if (NULL == vminfo[i].domname) {
            fprintf(stderr, "failed to get hostname of domain with ID: %d\n", vminfo[i].id);
            exit(VIR_ERR_UNKNOWN_HOST);
        }

        char *cap = (char *)malloc(sizeof(char) * DOMAIN_XML_SIZE);
        /* get XML configuration of each VM */
        cap = virDomainGetXMLDesc(vminfo[i].dp, VIR_DOMAIN_XML_UPDATE_CPU); 
        if (NULL == cap) {
            fprintf(stderr, "failed to get XMLDesc of domain: %s", vminfo[i].domname);
            exit(VIR_ERR_XML_ERROR);
        }
        /* get block statistics information of each VM */
        get_vm_blkpath(cap, vminfo[i].blkname); //
        get_vm_ifpath(cap, vminfo[i].ifname) ;  //
        free(cap);
        create_vm_rst_file(&vminfo[i]);
        //print_vm_info(&vminfo[i]);
    }

    struct timeval *tv_before = (struct timeval *)malloc(sizeof(struct timeval));
    struct timeval *tv_after = (struct timeval *)malloc(sizeof(struct timeval));
    struct vm_statistics *vm_stat_before = 
        (struct vm_statistics *)malloc(sizeof(struct vm_statistics) * active_domain_num);
    struct vm_statistics *vm_stat_after = 
        (struct vm_statistics *)malloc(sizeof(struct vm_statistics) * active_domain_num);
    struct mach_load *vm_sysload = (struct mach_load *)malloc(sizeof(struct mach_load) * active_domain_num);

    struct phy_statistics *phy_stat_before = 
        (struct phy_statistics *)malloc(sizeof(struct phy_statistics));
    struct phy_statistics *phy_stat_after = 
        (struct phy_statistics *)malloc(sizeof(struct phy_statistics));
    struct mach_load *phy_sysload = (struct mach_load *)malloc(sizeof(struct mach_load));

    while (1) {
        index++;
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
        /* elaspsed time in microsecond */
        long elapsed_time = (tv_after->tv_sec - tv_before->tv_sec) * 1000000 + (tv_after->tv_usec - tv_before->tv_usec);
        for (i = 0; i < active_domain_num; i++) {
            calculate_vm_load(&vm_sysload[i], &vm_stat_before[i], &vm_stat_after[i], elapsed_time, nodeinfo->memory);
            /* TODO: flush the buffer when program exits abnormally using signal processing */
            fprintf(vminfo[i].fp, "%-6ld %-6ld %-6.2lf %-6.2lf %-10.2lf %-10.2lf %-10.2lf %-10.2lf\n", 
                    (long)curtime, index, vm_sysload[i].cpu_load, vm_sysload[i].mem_load, 
                    vm_sysload[i].rd_load, vm_sysload[i].wr_load, vm_sysload[i].rx_load, vm_sysload[i].tx_load);
        }
        calculate_phy_load(phy_sysload, phy_stat_before, phy_stat_after, elapsed_time);
        fprintf(phyinfo->fp, "%-6ld %-6ld %-6.2lf %-6.2lf %-10.2lf %-10.2lf %-10.2lf %-10.2lf\n",
                    (long)curtime, index, phy_sysload->cpu_load, phy_sysload->mem_load, 
                    phy_sysload->rd_load, phy_sysload->wr_load, phy_sysload->rx_load, phy_sysload->tx_load);
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
