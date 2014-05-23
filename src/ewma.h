#ifndef __EWMA_H_
#define __EWMA_H_

#include <stdlib.h>
#include <math.h>
#include "monitor.h"

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
    double rx_alpha;
    double tx_alpha;
};

 
double genrand();

double stdev(double estv, double data);

double ewma(double estv_prev, double val_prev, double calpha);

void ewma_load(struct mach_load *estv_curr, struct mach_load estv_prev, 
        struct mach_load val_prev, struct ewma_coff coff);


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
    /* rx/tx workload prediction currently not implemented */
    estv_curr->rx_load = ewma(estv_prev.rx_load, val_prev.rx_load, coff.rx_alpha);
    estv_curr->tx_load = ewma(estv_prev.tx_load, val_prev.tx_load, coff.tx_alpha);
}

double stdev(double estv, double data)
{
    return (fabs(estv - data) / data);
}

#endif
