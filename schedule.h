#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include "monitor.h"
#include "ringbuffer.h"
#include <stdio.h>
#include <stdlib.h>


#ifndef __SCHEDULE_H
#define __SCHEDULE_H

#define VIR_MEM_LOW_BOUND_IN_BYTES (256 * 1024)   /* in kilobytes */

void sum_load(struct mach_load dsum, struct mach_load *ds, int nrvm);

int alloccpu(virConnectPtr, virDomainPtr domain, double cpu_bw_perc);

int allocmem(virDomainPtr domain, double mem_perc);

int allocrd(virDomainPtr domain, const char *dsk, double disk_rbps);

int allocwr(virDomainPtr domain, const char *dsk, double disk_wbps);

void cal_wr(struct mach_load , struct mach_load, double , double , double , double);

double min(double, double);

void schedcpu(virConnectPtr, struct vm_info *, double, double *, double *, double *, int);

void schedmem(struct vm_info *, double, double *, double *, double *, int);

void schedrd(struct vm_info *, double, double *, double *, double *, int);

void schedwr(struct vm_info *, double, double *, double *, double *, int);

void update_ac(struct mach_load *, struct mach_load *, struct ringbuffer *, int , int);

void schedule(struct vm_info *, struct mach_load *, struct mach_load **, 
        struct mach_load **, struct ringbuffer **, struct ringbuffer **, int);

#endif
