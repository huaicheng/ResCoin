#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define alpha   0.8
#define nr_data 100

double genrand()
{
    return (1.0 * rand()/RAND_MAX);
}

double ewma(double estv_prev, double data_prev)
{
    return (alpha * data_prev + (1 - alpha) * estv_prev);
}

double stdiff(double estv, double data)
{
    return (fabs(estv - data) / data);
}

int main()
{
    double data[nr_data] = {0.0}; 
    double estv[nr_data] = {0.0};

    int i;
    int cnt = 0;

    /* for time t, the formula is like below */
    //double estv_curr = alpha * data[t-1] + (1 - alpha) * estv_prev; 

    srand(time((time_t *)NULL));
    data[0] = genrand();
    /* init estimation value est to the first data */
    estv[0] = data[0];
    printf("%4.2lf\t%4.2lf\n", data[0], estv[0]);

    for (i = 1; i < (nr_data-1); i++) {

        /* compute the eligible data */
        if (stdiff(estv[i-1], data[i-1]) < 0.50) cnt++;

        /* make the prediction */
        estv[i] = ewma(estv[i-1], data[i-1]);

        /* actually, we can get the real data in the next time period */
        data[i] = genrand();

        printf("%4.2lf\t%4.2lf\n", data[i], estv[i]);

    }

    printf("%d/%d\n", cnt, nr_data);

    return 0;
}
