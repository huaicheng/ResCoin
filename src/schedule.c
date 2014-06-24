#include "schedule.h"
#include "ring_buffer.h"
#include "monitor.h"

/* here defines the threshold value of each resource to be tractable */
#define THRES_CPU  0.8
#define THRES_MEM  0.8
#define THRES_RD   
#define THRES_WR
#define THRES_RX
#define THRES_TX

enum
{
    CPU_RES,
    MEM_RES,
    RD_RES,
    WR_RES,
    RX_RES,
    TX_RES,
    NR_RES
};

struct load_in_array
{
    double load;
    int pos;
};

void thres_store(struct ring_buffer *dest, const string ring_buffer *src, 
        struct ring_buffer *curr)
{
    struct mach_load *val, *val2;
    rb_read_last(src, &val);
    rb_read_last(curr, &val2);
}

/* 
 * positive number -> A, negative number -> B 
 * TODO: add some threshold checking here
 */
void struct2array(struct load_in_array *A, struct load_in_array *B, 
        int *ap, int *bp,  struct mach_load val)
{
    int app, bpp;

    if (val.cpu_load > 0) {
        app++;
        A[app].load = val.cpu_load;
        A[app].id = RES_CPU;
    }
    else if (val.cpu_load < 0) {
        bpp++;
        B[bpp].load = -1 * val.cpu_load;
        B[app].id = RES_CPU;
    }
    
    if (val.mem_load > 0) {
        app++;
        A[app].load = val.mem_load;
        A[app].id = RES_MEM;
    }
    else if (val.mem_load < 0) {
        bpp++;
        B[bpp].load = -1 * val.mem_load;
        B[bpp].id = RES_MEM;
    }

    if (val.rd_load > 0) {
        app++;
        A[app].load = val.rd_load;
        A[app].id = RES_RD;
    }
    else if (val.rd_load < 0) {
        bpp++;
        B[bpp].load = -1 * val.rd_load;
        B[bpp].id = RES_RD;
    }

    if (val.wr_load > 0) {
        app++;
        A[app].load = val.wr_load;
        A[app].id = RES_WR;
    }
    else if (val.wr_load < 0) {
        bpp++;
        B[bpp].load = -1 * val.wr_load;
        B[bpp].id = RES_WR;
    }

    /* return the array size of A/B */
    *ap = app;
    *bp = bpp;
}

void schedule(struct ring_buffer *obs, struct ring_buffer *est, struct ring_buffer **vm_obs, struct ring_buffer **vm_est,
        struct mach_load *ac, int nr_vm);
{
    int i;
    struct mach_load val;
    int ap, bp;

    struct ring_buffer **delta_thres;
    delta_thres = 
        (struct ring_buffer **)malloc(sizeof(struct ring_buffer *) * n);
    for (i = 0; i < n; i++) {
        delta_thres[i] = 
            (struct ring_buffer *)malloc(sizeof(struct ring_buffer));
        //thres_store(delta_thres[i], vm_delta[i], TODO);
    }

    /* search through all the VMs */
    for (i = 0; i < n; i++) {
        ap = bp = 0;
        /* exchange resource internally within each VM */ 
        rb_read_last(vm_delta[i], &val);
        struct2array(A, B, &ap, &bp, val);
        /* try your best to inter-exchange resources */
        int j, k;
        for (j = 0; j < ap; j++)
            for (k = 0; k < bp; k++) {
                if (B[i].load )
            }
    }
}
