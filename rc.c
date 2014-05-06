#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <libvirt/libvirt-qemu.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "rc.h"

/*
 * print the typed parameter value
 */
void print_typed_parameters(virTypedParameterPtr domain, int nparams)
{
    int i;
    for (i = 0; i < nparams; i++) {
        switch (domain[i].type) {
            case VIR_TYPED_PARAM_INT:
                printf("%-15s : %d\n", domain[i].field, domain[i].value.i);
                break;

            case VIR_TYPED_PARAM_UINT:
                printf("%-15s : %u\n", domain[i].field, domain[i].value.ui);
                break;

            case VIR_TYPED_PARAM_LLONG:
                printf("%-15s : %lld\n", domain[i].field, domain[i].value.l);
                break;

            case VIR_TYPED_PARAM_ULLONG:
                printf("%-15s : %llu\n", domain[i].field, domain[i].value.ul);
                break;

            case VIR_TYPED_PARAM_DOUBLE:
                printf("%-15s : %f\n", domain[i].field, domain[i].value.d);
                break;

            case VIR_TYPED_PARAM_BOOLEAN:
                printf("%-15s : %d\n", domain[i].field, domain[i].value.b ? 1 : 0);
                break;

            case VIR_TYPED_PARAM_STRING:
                printf("%-15s : %s\n", domain[i].field, domain[i].value.s);
                break;

            default:
                printf("unimplemented parameter type %d", domain[i].type);
        }
    }
}

/* 
 * Get the scheduling information of "domain" and store it to "params"
 * Attention: before calling this function, what you need to do is just
 * to define the "params" and "nparams" variable(better define 
 * params = NULL, nparams = 0), DO NOT try to allocate any
 * memory for them as this function will do that for you.
 * @Return 0 in case of success and -1 in case of error
 */
int get_schedinfo(virDomainPtr domain, virTypedParameterPtr *params, int *nparams)
{
    char *retc = virDomainGetSchedulerType(domain, nparams);
    if ((NULL == retc) || (0 == *nparams)) {
        fprintf(stderr, "get domain scheduler type failed...\n");
        return -1;
    }
    *params = (virTypedParameterPtr)malloc(sizeof(**params) * (*nparams));
    if (-1 == virDomainGetSchedulerParametersFlags(domain, *params, nparams, 0)) {
        fprintf(stderr, "get domain scheduler parameters failed...\n");
        return -1;
    }

    return 0;
}

/* 
 * Get blkio parameter of the given domain, it automatically allocate space
 * for params parameter so there is no need for you to initialize the variable
 * of "params" and "nparams". This function will decide the correct value for
 * them.
 * @Return 0 in case of success and -1 in case of error
 */
int get_blkio(virDomainPtr domain, virTypedParameterPtr *params, int *nparams)
{
    /* first time to call virDomainGetBlkioParameters() to set the nparams variable */
    virDomainGetBlkioParameters(domain, *params, nparams, 0);
    *params = (virTypedParameterPtr)malloc(sizeof(**params) * (*nparams));
    /* call virDomainGetBlkioParameters() again to fill information to the params struct */
    if (-1 == virDomainGetBlkioParameters(domain, *params, nparams, 0)) {
        fprintf(stderr, "get domain blkio parameters failed...\n");
        return -1;
    }

    return 0;
}

/*
 * TODO: params.value is a union, how should the new_value be set to the approriate data type ?
 * change params[field].value to new_value, this function is currently used to modify cpu_shares
 * and blkio.weight, so "int" type is enough for new_value.
 * return 0 in case of success and -1 in case of error
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

/* 
 * set the scheduler parameters of domain to "params", remember that params is the new
 * scheduler parameters now, you should set it to the appropriate value before calling 
 * this function.
 * return 0 in case of success, and -1 in case of error
 */
int set_schedinfo(virDomainPtr domain, int new_cpu_shares)
{
    virTypedParameterPtr params = NULL;
    int nparams = 0;

    get_schedinfo(domain, &params, &nparams);
    if (0 == set_params_value(params, nparams, "cpu_shares", new_cpu_shares)) {
        /* 
         * flags:0 represents that the setting is only effective for the current state 
         */
        virDomainSetSchedulerParametersFlags(domain, params, nparams, 0);
        /* 
         * you can print the schedinfo here, or you will have to recall 
         * the virDomainGetSchedulerParameter() function to get the params 
         * and then print its value. 
         */
        print_typed_parameters(params, nparams);
        free(params);
        return 0;
    }
    return -1;
}

/* 
 * wrapper for set_schedinfo()
 */
int set_cpu_shares(virDomainPtr domain, int new_cpu_shares)
{
    return set_schedinfo(domain, new_cpu_shares);
}

/*
 * params and nparams don't need to be initialized here, get_blkio() will do that for you
 * set the blkio parameters of "weight" to the specific value
 * return 0 in case of success and -1 in case of error
 */
int set_blkio(virDomainPtr domain, int new_weight)
{
    virTypedParameterPtr params = NULL;
    int *nparams = 0;

    get_blkio(domain, &params, nparams);
    /* set weight to new_weight */
    set_params_value(params, *nparams, "weight", new_weight);
    /* make the change effective */
    virDomainSetBlkioParameters(domain, params, *nparams, 0);
    print_typed_parameters(params, *nparams);

}

/* 
 * wrapper for set_blkio() function
 */
int set_blkio_weight(virDomainPtr domain, int new_weight)
{
    return set_blkio(domain, new_weight);
}
