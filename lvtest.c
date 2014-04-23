#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <libvirt/libvirt-qemu.h>

#include "rc.h"

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: ./a.out <cpu_shares> <io_weight>\n");
        exit(1);
    }
    virConnectPtr conn = virConnectOpen(NULL);
    int actdom_nr = virConnectNumOfDomains(conn);
    int *actdomids = (int *)malloc(sizeof(int) * actdom_nr);
    virConnectListDomains(conn, actdomids, actdom_nr);
    virDomainPtr *dp = malloc(sizeof(virDomainPtr *)*actdom_nr);
    int i;
    virTypedParameterPtr params;
    int nparams;
    for (i = 0; i < actdom_nr; i++) { 
        dp[i] = virDomainLookupByID(conn, actdomids[i]);
        get_schedinfo(dp[i], &params, &nparams);
        print_typed_parameters(params, nparams);

        printf("********blkio************\n");
        nparams = 0;
        virDomainGetBlkioParameters(dp[0], params, &nparams, 0);
        printf("after: nparams = %d\n", nparams);
        virDomainGetBlkioParameters(dp[0], params, &nparams, 0);
        print_typed_parameters(params, nparams);
        printf("*********end*************\n");
        virDomainSetBlkioParameters(dp[0], params, nparams, 0);

    }
    printf("=======================================\n");
    params = NULL;
    nparams = 0;
    printf("argv[1]=%d\n", atoi(argv[1]));
    set_schedinfo(dp[0], params, nparams, atoi(argv[1]));
    set_blkio_weight(dp[0], params, &nparams, atoi(argv[2]));
    //print_typed_parameters(params, nparams);

    return 0;
}
