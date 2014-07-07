#ifndef __SCHEDULE_H
#define __SCHEDULE_H

#define VIR_MEM_LOW_BOUND_IN_BYTES 256*1024   /* in kilobytes */

void sum_load(struct mach_load *, struct mach_load **, int);

int alloccpu(virDomainPtr, double);

int allocmem(virDomainPtr, double);

int allocdsk(virDomainPtr, const char *, double, double);

double cal_wr(double, double);

double min(double, double);

void schedcpu(struct vm_info *, double, double, double, double, int);

void schedmem(struct vm_info *, double, double *, double *, double *, int);

void scheddsk(struct vm_info *, double, double *, double *, double *, double *, int);

void update_ac(struct mach_load *, struct mach_load *, struct ring_buffer *);

void schedule(string ring_buffer *, struct ring_buffer *, struct ring_buffer **, struct ring_buffer **, struct mach_laod *, int);

#endif
