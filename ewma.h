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

#ifndef __EWMA_H_
#define __EWMA_H_

#include <stdlib.h>
#include <math.h>
#include "monitor.h"
#include "ringbuffer.h" 

#define alpha       0.8
#define nr_data     100

/* 
 * coefficient for ewma prediction of workload
 * All between 0~1
 */
struct ewma_coff 
{
    double cpu_alpha;
    double mem_alpha;
    double rd_alpha;
    double wr_alpha;
};

double genrand();

double ewma(double estv_prev, double val_prev, double calpha);

void ewma_load(struct mach_load *estv_curr, struct mach_load estv_prev, 
        struct mach_load val_prev, struct ewma_coff coff);

void estimate(struct ringbuffer *vm_obs, struct ringbuffer *vm_est, 
        struct ewma_coff coff, int index);

double genrand()
{
    return (1.0 * rand()/RAND_MAX);
}

double ewma(double estv_prev, double val_prev, double calpha)
{
    return (calpha * val_prev + (1 - calpha) * estv_prev);
}

void ewma_load(struct mach_load *estv_curr, struct mach_load estv_prev, 
        struct mach_load val_prev, struct ewma_coff coff)
{
    estv_curr->cpu_load = ewma(estv_prev.cpu_load, val_prev.cpu_load, coff.cpu_alpha);
    estv_curr->mem_load = ewma(estv_prev.mem_load, val_prev.mem_load, coff.mem_alpha);
    estv_curr->rd_load = ewma(estv_prev.rd_load, val_prev.rd_load, coff.rd_alpha);
    estv_curr->wr_load = ewma(estv_prev.wr_load, val_prev.wr_load, coff.wr_alpha);
}

void estimate(struct ringbuffer *vm_obs, struct ringbuffer *vm_est, 
        struct ewma_coff coff, int index)
{
    if (index == 1) {
        rb_write(vm_est, &vm_obs->buff[0]);
    } else {
        struct mach_load est_curr_val;
        int prev_pos_obs, prev_pos_est;

        prev_pos_obs = (vm_obs->end - 1 + vm_obs->size) % 
            vm_obs->size;
        prev_pos_est = (vm_est->end - 1 + vm_est->size) %
            vm_est->size;

        memset(&est_curr_val, 0, sizeof(est_curr_val));
        struct mach_load obs_val, est_val;

        ewma_load(&est_curr_val, vm_est->buff[prev_pos_est], 
                vm_obs->buff[prev_pos_obs], coff);   

        rb_write(vm_est, &est_curr_val);
    }
}

#endif
