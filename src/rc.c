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
 * to define the "params" and "nparams" variable(better define params = NULL, 
 * nparams = 0), DO NOT try to allocate any memory for them as this function 
 * will do that for you.
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
 * Get memory parameters of the given domain, it automatically allocate space
 * for params parametes so there is no need for you to initialize the variable
 * of "params" and "nparams". This function will decide the correct value for
 * them.
 * @Return 0 in case of success and -1 in case of error
 */
int get_mem(virDomainPtr domain, virTypedParameterPtr *params, int *nparams)
{
    if (virDomainGetMemoryParameters(domain, NULL, nparams, 0) == 0 && (*nparams != 0)) {
        *params = (virTypedParameterPtr)malloc(sizeof(**params) * (*nparams));
        memset(params, 0, sizeof(**params) * (*nparams));
        if (0 != virDomainGetMemoryParameters(domain, *params, nparams, 0))
            return -1;
        return 0;
    }
    return -1;
}

int get_params_type(virTypedParameterPtr params, int nparams,
        const char *field, int *params_type, int *pos)
{
    int i;
    for (i = 0; i < nparams; i++) 
        if (0 == strcmp(field, params[i].field)) {
            *params_type = params[i].type;
            *pos = i;
            return 0;
        }
    return -1;
}

/*
 * TODO: params.value is a union, how should the new_value be set to the 
 * approriate data type ? [ take "const char *" as input ] !!
 * change params[field].value to new_value
 * return 0 in case of success and -1 in case of error
 */
int set_params_value(virTypedParameterPtr params, int nparams, 
        const char *field, const char *value)
{
    int ret = -1;
    /*
     * Firstly, find out the element of field from the virTypedParameter 
     * struct and then do the change
     */ 
    int i;
    for (i = 0; i < nparams; i++) {
        if (0 == strcmp(field, params[i].field)) {
            switch (params[i].type) {
                case VIR_TYPED_PARAM_INT:
                    if (strtol_i(value, NULL, 10, &params->value.i) < 0)
                        goto cleanup;
                    break;

                case VIR_TYPED_PARAM_UINT:
                    if (strtol_ui(value, NULL, 10, &params->value.ui) < 0)
                        goto cleanup;
                    break;

                case VIR_TYPED_PARAM_LLONG:
                    if (strtol_ll(value, NULL, 10, &params->value.l) < 0)
                        goto cleanup;
                    break;

                case VIR_TYPED_PARAM_ULLONG:
                    if (strtol_ull(value, NULL, 10, &params->value.ul) < 0)
                        goto cleanup;
                    break;

                case VIR_TYPED_PARAM_DOUBLE:
                    if (strtod_d(value, NULL, 10, &params->value.d) < 0)
                        goto cleanup;
                    break;

                case VIR_TYPED_PARAM_BOOLEAN:
                    params[i].value.b = (bool)new_value;
                    if (strtod_d(value, NULL, 10, &params->value.d) < 0)
                        goto cleanup;
                    break;

                default :
                    printf("unexpected type %d for field %s\n", type, name);
                    return -1;
            }
            ret = 0;
        }
    }
    return ret;
}

/* 
 * set the scheduler parameters of domain to "params", remember that params is 
 * the new scheduler parameters now, you should set it to the appropriate value 
 * before calling this function.
 * return 0 in case of success, and -1 in case of error
 */
int set_schedinfo(virDomainPtr domain, const char *field, long long new_value)
{
    virTypedParameterPtr params = NULL;
    int nparams = 0;

    get_schedinfo(domain, &params, &nparams);
    if (0 == set_params_value(params, nparams, field, new_value)) {
        /* 
         * flags:0 represents that the setting is only effective for the current 
         * state 
         */
        virDomainSetSchedulerParametersFlags(domain, params, nparams, 0);
        /* 
         * you can print the schedinfo here, or you will have to recall 
         * the virDomainGetSchedulerParameter() function to get the params 
         * and then print its value. 
         */
        /* print_typed_parameters(params, nparams); */
        free(params);
        return 0;
    }
    return -1;
}

/* 
 * wrapper for set_schedinfo()
 */
int set_cpu_shares(virDomainPtr domain, long long new_cpu_shares)
{
    return set_schedinfo(domain, "cpu_shares", new_cpu_shares);
}

int get_cpu_shares(virDomainPtr domain, long long *cpu_shares)
{
    int i;
    int type;
    virTypedParamsPtr params = NULL;
    int nparams = 0;

    if (-1 == get_schedinfo(domain, params, &nparams)) 
        return -1;

    if (-1 == get_params_type(params, nparams, "cpu_shares", &type, &i)) 
        return -1;

    switch (type) {
        case VIR_TYPED_PARAM_INT:
            cpu_shares = (long long)params[i].value.i;
            break;

        case VIR_TYPED_PARAM_UINT:
            cpu_shares = (long long)params[i].value.ui; 
            break;

        case VIR_TYPED_PARAM_LLONG:
            cpu_shares = (long long)params[i].value.l;
            break;

        case VIR_TYPED_PARAM_ULLONG:
            cpu_shares = (long long)params[i].value.ul;
            break;

        case VIR_TYPED_PARAM_DOUBLE:
            cpu_shares = (long long)params[i].value.d;
            break;

        case VIR_TYPED_PARAM_BOOLEAN:
            cpu_shares = (long long)params[i].value.b;
            break;

        default :
            printf("Type %d not supported now\n", params[i].type);
            return -1;
    }
    if (1 == virTypedParamsGetLLong(params, nparams, "cpu_shares", cpu_shares))
            return 0;
        return -1;
    }
    return -1;
}

int set_vcpu_period(virDomainPtr domain, long long new_vcpu_period)
{
    return set_schedinfo(domain, "vcpu_period", new_vcpu_period);
}

int set_vcpu_quota(virDomainPtr domain, long long new_vcpu_quota)
{
    return set_schedinfo(domain, "vcpu_quota", new_vcpu_quota);
}

int set_vcpu_bw(virDomainPtr domain, long long new_vcpu_period, 
        long long new_vcpu_quota) 
{
    return (set_vcpu_period(domain, new_vcpu_period) && 
            set_vcpu_quota(domain, new_vcpu_quota));
}

/*
 * params and nparams don't need to be initialized here, get_blkio() will do 
 * that for you set the blkio parameters of "weight" to the specific value
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

int set_mem(virDomainPtr domain, unsigned long new_memsize)
{
    if (virDomainSetMemory(domain, new_memsize) != 0) {
        fprintf(stderr, "Can't set VM memory to %ld\n", new_memsize);
        exit(errno);
    }
}
