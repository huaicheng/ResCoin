#ifndef __EWMA_H_
#define __EWMA_H_

#define alpha       0.8
#define nr_data     100


double genrand();

double ewma(double estv_prev, double val_prev);

double stdiff(double estv, double data);

double genrand()
{
    return (1.0 * rand()/RAND_MAX);
}

double ewma(double estv_prev, double val_prev)
{
    return (alpha * val_prev + (1 - alpha) * estv_prev);
}

double stdiff(double estv, double data)
{
    return (fabs(estv - data) / data);
}

#endif

