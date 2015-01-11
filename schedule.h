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
