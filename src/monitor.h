#ifndef __MONITOR_H
#define __MONITOR_H

#include <libvirt/libvirt.h>    /* libvirt API */
#include <libvirt/virterror.h>  /* libvirt error handling */
#include <sys/socket.h>         /* socket */
#include <netinet/in.h>         /* struct sockaddr */
#include <sys/types.h>          /* time_t etc. */
#include <inttypes.h>
#include <stdbool.h>            /* bool type */
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>              /* system error number */

#define L_PORT          4588    /* server listen port */
#define DOMAIN_XML_SIZE 8192    /* in bytes */
#define MAXLINE         8192

#define TIME_INTERVAL   10      /* in seconds */
#define ETHERNET        "br0"
#define DISK            "sda"
#define BLKNAMEMAX      64
#define IFNAMEMAX       64
#define DOMNAMEMAX      64
#define IPSZ            16
#define ETC_HOSTS       "/etc/hosts"
#define FORMATS         "%-6ld %-6ld %-6.2lf %-6.2lf %-6.2lf\n"

extern char *hypervisor;
extern int active_domain_num;
extern int nr_cores;

struct vm_ipaddr_name_list 
{
    char domname[DOMNAMEMAX];   
    char ipaddr[IPSZ]; /* xxx.xxx.xxx.xxx */
};

extern struct vm_ipaddr_name_list vminl[];


#define vminlsz sizeof(vminl)/sizeof(struct vm_ipaddr_name_list)

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

enum {
    MEMTOTAL,
    MEMFREE,
    BUFFERS,
    CACHED,
    NR_MEMINFO // all mem info we need
};

typedef struct phy_statistics {
    /* cpu related stat, "/proc/stat" */
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

    /* memory related stat, "/proc/meminfo" */
    unsigned long memtotal;
    unsigned long memfree;
    unsigned long buffers;
    unsigned long cached;

    /* disk related stat, "/proc/diskstats" */
    ull rd_sectors;  /* Field 3 */
    ull wr_sectors;  /* Field 6 */
    ull io_time_ms;  /* Field 10 */ 

    /* net related stat, "/proc/dev/net" */
    ull rx_bytes;
    ull tx_bytes;

}phy_statistics;

typedef struct phy_info {
    char *hostname;
    int hostname_length;
    FILE *fp;
}phy_info;

typedef struct vm_info {
    /* we depend on "dp" to acquire other field */
    virDomainPtr dp;
    int id;
    /* virDomainGetName, no need to allocate for domname */
    const char *domname; 
    char blkname[BLKNAMEMAX];
    char ifname[IFNAMEMAX];
    FILE *fp;
    char ipaddr[IPSZ];
}vm_info;

typedef struct vm_statistics {
    ull cpu_time;

    /* the following three are gotten from libvirt API */
    unsigned long maxmem;
    unsigned long curmem;
    unsigned long rss; /* VM resident set size */

    /* 
     * the following are received from the agent in guest 
     * mem_percentage = (memfree+buffers+cached)/memtotal
     */
    uint32_t memtotal;
    uint32_t memfree;
    uint32_t buffers;
    uint32_t cached;

    ull rd_bytes;
    ull wr_bytes;

    ull rx_bytes;
    ull tx_bytes;
}vm_statistics;

/* used for both physical and VMs */
typedef struct mach_load {
    double cpu_load; /* 0~100 */
    double mem_load; /* 0~100 */

    double disk_load; /* 0~100 */
    double rd_load;  /* Kbps */
    double wr_load;  /* Kbps */

    double net_load; /* 0~100 */
    double rx_load;  /* Kbps */
    double tx_load;  /* Kbps */
}mach_load;

void init_vm_info(struct vm_info *vminfo);

void init_phy_info(struct phy_info *phyinfo);

void print_vm_info(struct vm_info *vminfo);

void print_vm_statistics(struct vm_statistics *vm_stat, struct vm_info *vminfo);

void print_mach_load(struct mach_load *vmload);

int get_vm_blkname(const char *xml, char *blkname);

int get_vm_ifname(const char *xml, char *ifname);

int get_vm_ipaddr(char *ipaddr, const char *domname);

void get_vm_rss(virDomainPtr dp, struct vm_statistics *vm_stat);

void get_vm_static_info(struct vm_info *vminfo);

void get_vm_cpustat(struct vm_statistics *vm_stat, struct vm_info *vminfo);

void get_vm_memstat(struct vm_statistics *vm_stat, struct vm_info *vminfo);

void get_vm_blkstat(struct vm_statistics *vm_stat, struct vm_info *vminfo);

void get_vm_ifstat(struct vm_statistics *vm_stat, struct vm_info *vminfo);

void get_vm_workload(struct vm_statistics *vm_stat, struct vm_info *vminfo);

void compute_vm_load(struct mach_load *vmload, 
        struct vm_statistics *vm_stat_before, 
        struct vm_statistics *vm_stat_after,
        ull microsec, unsigned long total_mem, 
        struct mach_load *phy_sysload);

void create_phy_rst_file(struct phy_info *phyinfo);

int create_vm_rst_file(struct vm_info *vminfo);

int match_nic(char *buffer, char *nic);

void extract_nic_bytes(char *buffer, ull *rx_bytes, ull *tx_bytes);

void get_phy_cpustat(struct phy_statistics *phy_stat);

void get_phy_memstat(struct phy_statistics *phy_stat);

void get_phy_blkstat(struct phy_statistics *phy_stat, const char *disk);

void get_phy_ifstat(struct phy_statistics *phy_stat, const char *nic);

void get_phy_workload(struct phy_statistics *phy_stat);

ull sum_cpu_stats(struct phy_statistics *phy_stats);

void compute_phy_load(struct mach_load *phyload, 
        struct phy_statistics *phy_stat_before,
        struct phy_statistics *phy_stat_after,
        long microsec);

#endif
