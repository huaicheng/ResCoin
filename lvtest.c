#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <libvirt/libvirt-qemu.h>

void print_typed_parameters(virTypedParameterPtr params, int nparams)
{
    int i;
    for (i = 0; i < nparams; i++)
        printf("%-15s: %lld\n", params[i].field, params[i].value.l);
}

/* 
 * change params[cpu_shares].value.l to new_cpu_shares, 0 is OK and -1 fails
 */
int set_params_value(virTypedParameterPtr params, int nparams, const char *field, int new_value)
{
    /*
     * Firstly, find out the element of cpu_shares from the virTypedParameter struct
     * and then do the change
     */ 
    int i;
    for (i = 0; i < nparams; i++) {
        if (0 == strcmp(field, params[i].field)) {
            switch (params[i].type) {
                case VIR_TYPED_PARAM_INT:
                    params[i].value.i = (int)new_value;
                    break;

                case VIR_TYPED_PARAM_UINT:
                    params[i].value.ui = (unsigned int)new_value;
                    break;

                case VIR_TYPED_PARAM_LLONG:
                    params[i].value.l = (long long)new_value;
                    break;

                case VIR_TYPED_PARAM_ULLONG:
                    params[i].value.ul = (unsigned long long)new_value;
                    break;

                case VIR_TYPED_PARAM_DOUBLE:
                    params[i].value.d = (double)new_value;
                    break;

                case VIR_TYPED_PARAM_BOOLEAN:
                    params[i].value.b = (bool)new_value;
                    break;

                default :
                    printf("Type %d not supported now\n", params[i].type);
                    return -1;
            }
            return 0;
        }
    }
    return -1;
}

void get_schedinfo(virDomainPtr domain, virTypedParameterPtr *params, int *nparams)
{
    char *retc = virDomainGetSchedulerType(domain, nparams);
    if ((NULL == retc) || (0 == *nparams)) {
        fprintf(stderr, "get domain scheduler type failed...");
        exit(VIR_ERR_RESOURCE_BUSY);
    }
    *params = (virTypedParameterPtr)malloc(sizeof(**params) * (*nparams));
    memset(*params, 0, sizeof(**params) * (*nparams));
    if (virDomainGetSchedulerParametersFlags(domain, *params, nparams, 0)) {
        fprintf(stderr, "get domain scheduler parameters failed...");
        exit(VIR_ERR_RESOURCE_BUSY);
    }
}

/* 
 * set the scheduler parameters of domain to "params", remember that params is the new
 * scheduler parameters now, you should set it to the appropriate value before calling 
 * this function.
 */

void set_schedinfo(virDomainPtr domain, virTypedParameterPtr params, int nparams, int new_cpu_shares)
{
    get_schedinfo(domain, &params, &nparams);
    set_cpu_shares(params, nparams, new_cpu_shares);
    virDomainSetSchedulerParametersFlags(domain, params, nparams, 0);
    print_typed_parameters(params, nparams);
    free(params);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: ./a.out <new_cpu_shares>\n");
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
        virDomainSetBlkioParameters(dp[0], params, &nparams, 0);

    }
    printf("=======================================\n");
    params = NULL;
    nparams = 0;
    set_schedinfo(dp[0], params, nparams, atoi(argv[1]));
    //print_typed_parameters(params, nparams);

    return 0;
}
