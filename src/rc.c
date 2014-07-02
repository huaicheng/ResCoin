#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <libvirt/libvirt-qemu.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "rc.h"

/*
 * print the typed parameter value
 */
void print_typed_parameters(virTypedParameterPtr params, int nparams)
{
    int i;
    for (i = 0; i < nparams; i++) {
        switch (params[i].type) {
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
                printf("%-15s : %lf\n", domain[i].field, domain[i].value.d);
                break;

            case VIR_TYPED_PARAM_BOOLEAN:
                printf("%-15s : %d\n", domain[i].field, 
                        domain[i].value.b ? 1 : 0);
                break;

            case VIR_TYPED_PARAM_STRING:
                printf("%-15s : %s\n", domain[i].field, domain[i].value.s);
                break;

            default:
                printf("unimplemented parameter type [%d]", domain[i].type);
        }
    }
}


/*
 * Take the input parameter "value" as "char *" type, then translate to its 
 * corresponding data type, this is essential for those virTypedParamters 
 * parameters whose value data type is unknown
 * Return 0 in case of success and -1 in case of error
 */
int set_params_value(virTypedParameterPtr params, int nparams, 
        const char *field, const char *value)
{
    int ret = -1;
    int i;

    /*
     * Firstly, find out the element of field from the virTypedParameter 
     * struct and then do the change
     */ 
    virTypedParameterPtr param = virTypedParamsGet(params, nparams, field);
    if (!param)
        goto cleanup;

    switch (param->type) {
        case VIR_TYPED_PARAM_INT:
            if (strtol_i(value, NULL, 10, param->value.i) < 0) {
                fprintf(stderr, "Invalid value for field '%s':"
                        "expected int", name);
                goto cleanup;
            }
            break;

        case VIR_TYPED_PARAM_UINT:
            if (strtol_ui(value, NULL, 10, param->value.ui) < 0) {
                fprintf(stderr, "Invalid value for field '%s':"
                        "expected unsigned int", name);
                goto cleanup;
            }
            break;

        case VIR_TYPED_PARAM_LLONG:
            if (strtol_ll(value, NULL, 10, param->value.l) < 0) {
                fprintf(stderr, "Invalid value for field '%s':"
                        "expected long long int", name);
                goto cleanup;
            }
            break;

        case VIR_TYPED_PARAM_ULLONG:
            if (strtol_ull(value, NULL, 10, param->value.ul) < 0) {
                fprintf(stderr, "Invalid value for field '%s':"
                        "expected unsigned long long", name);
                goto cleanup;
            }
            break;

        case VIR_TYPED_PARAM_DOUBLE:
            if (strtod_d(value, NULL, 10, param->value.d) < 0) {
                fprintf(stderr, "Invalid value for field '%s':"
                        "expected double", name);
                goto cleanup;
            }
            break;

        case VIR_TYPED_PARAM_BOOLEAN:
            /* TODO: need modification here */
            params[i].value.b = (bool)new_value;
            break;

        default :
            printf("unexpected type %d for field %s\n", type, name);
            goto cleanup;
    }

    ret = 0;

cleanup:
    return ret;
}

/* 
 * Get the scheduling information of "domain" and store it to "params"
 * Attention: before calling this function, what you need to do is just
 * to define the "params" and "nparams" variable(better define params = NULL, 
 * nparams = 0), DO NOT try to allocate any memory for them as this function 
 * will do that for you.
 * Return 0 in case of success and -1 in case of error
 */
int get_schedinfo(virDomainPtr domain, virTypedParameterPtr *params, 
        int *nparams)
{
    int ret = -1;

    char *retc = virDomainGetSchedulerType(domain, nparams);
    if ((NULL == retc) || (0 == *nparams)) {
        fprintf(stderr, "get domain scheduler type failed...\n");
        goto cleanup
    }

    *params = (virTypedParameterPtr)malloc(sizeof(**params) * (*nparams));
    if (-1 == virDomainGetSchedulerParametersFlags(domain, *params, nparams, 
                0)) {
        fprintf(stderr, "get domain scheduler parameters failed...\n");
        goto cleanup;
    }
    
    ret = 0;

cleanup:
    return ret;
}

/* 
 * set the scheduler parameters of domain to "params", remember that params is 
 * the new scheduler parameters now, you should set it to the appropriate value 
 * before calling this function.
 * This is a general function to set sched related value for domain
 * return 0 in case of success, and -1 in case of error
 */
int set_schedinfo(virDomainPtr domain, const char *field, const char *value)
{
    int ret = -1;
    virTypedParameterPtr params = NULL;
    int nparams = 0;

    if (-1 == get_schedinfo(domain, &params, &nparams))
        goto cleanup;

    if (-1 == set_params_value(params, nparams, field, value)) 
        goto cleanup;
    /* 
     * flags:0 represents that the setting is only effective for the current 
     * state 
     */
    if (-1 == virDomainSetSchedulerParametersFlags(domain, params, nparams, 0))
        goto cleanup;
    /* 
     * you can print the schedinfo here, or you will have to recall 
     * the virDomainGetSchedulerParameter() function to get the params 
     * and then print its value. 
     */
    /* print_typed_parameters(params, nparams); */

    ret = 0;

cleanup:
    virTypedParamsFree(params, nparams);
    return ret;
}

/* 
 * set cpu_shares: unsigned long long
 */
int set_cpu_shares(virDomainPtr domain, const char *cpu_shares)
{
    return set_schedinfo(domain, VIR_DOMAIN_SCHEDULER_CPU_SHARES, cpu_shares);
}

int set_cpu_shares_ull(virDomainPtr domain, unsigned long long *cpu_shares)
{
    int i;
    int type;
    virTypedParamsPtr params = NULL;
    virTypedParamsPtr param = NULL;
    int nparams = 0;
    int ret = -1;

    if (-1 == get_schedinfo(domain, &params, &nparams)) 
        goto cleanup;
    if (param = virTypedParamsGet(params, nparams, 
                VIR_DOMAIN_SCHEDULER_CPU_SHARES)) {
        param->value.ul = *cpu_shares;

        if (-1 == virDomainSetSchedulerParametersFlags(domain, params, nparams, 0))
            goto cleanup;

        ret = 0;
    }

cleanup:
    virTypedParamsFree(params, nparams);
    return ret;
}

/* known cpu_shares: ull */
int get_cpu_shares(virDomainPtr domain, unsigned long long *cpu_shares)
{
    int i;
    int type;
    virTypedParamsPtr params = NULL;
    virTypedParamsPtr param = NULL;
    int nparams = 0;
    int ret = -1;

    if (-1 == get_schedinfo(domain, &params, &nparams)) 
        goto cleanup;
    if (param = virTypedParamsGet(params, nparams, 
                VIR_DOMAIN_SCHEDULER_CPU_SHARES)) {
        *cpu_shares = param->value.ul;
        ret = 0;
        return ret;
    }

cleanup:
    virTypedParamsFree(params, nparams);
    return ret;
}

/* set vcpu_period: unsigned long long */
int set_vcpu_period(virDomainPtr domain, const char *vcpu_period)
{
    return set_schedinfo(domain, VIR_DOMAIN_SCHEDULER_VCPU_PERIOD, vcpu_period);
}

int set_vcpu_period_ull(virDomainPtr domain, unsigned long long vcpu_period)
{
    int i;
    int type;
    virTypedParamsPtr params = NULL;
    virTypedParamsPtr param = NULL;
    int nparams = 0;
    int ret = -1;

    if (-1 == get_schedinfo(domain, &params, &nparams)) 
        goto cleanup;

    if (param = virTypedParamsGet(params, nparams, 
                VIR_DOMAIN_SCHEDULER_VCPU_PERIOD)) {
        param->value.ul = *vcpu_period;
        if (-1 == virDomainSetSchedulerParametersFlags(domain, params, nparams, 0))
            goto cleanup;
        ret = 0;
    }

cleanup:
    virTypedParamsFree(params, nparams);
    return ret;
}

/* known vcpu_period: ull */
int get_vcpu_period(virDomainPtr domain, unsigned long long *vcpu_period)
{
    int i;
    int type;
    virTypedParamsPtr params = NULL;
    virTypedParamsPtr param = NULL;
    int nparams = 0;
    int ret = -1;

    if (-1 == get_schedinfo(domain, &params, &nparams)) 
        goto cleanup;

    if (param = virTypedParamsGet(params, nparams, 
                VIR_DOMAIN_SCHEDULER_VCPU_PERIOD)) {
        *vcpu_period = param->value.ul;
        ret = 0;
        return ret;
    }

cleanup:
    virTypedParamsFree(params, nparams);
    return ret;
}

/* set vcpu_quota: long long */
int set_vcpu_quota(virDomainPtr domain, const char *vcpu_quota)
{
    return set_schedinfo(domain, VIR_DOMAIN_SCHEDULER_VCPU_QUOTA, vcpu_quota);
}

int set_vcpu_quota_ll(virDomainPtr domain, long long *vcpu_quota)
{
    int i;
    int type;
    virTypedParamsPtr params = NULL;
    virTypedParamsPtr param = NULL;
    int nparams = 0;
    int ret = -1;

    if (-1 == get_schedinfo(domain, &params, &nparams)) 
        goto cleanup;

    if (param = virTypedParamsGet(params, nparams, 
                VIR_DOMAIN_SCHEDULER_VCPU_QUOTA)) {
        param->value.ul = *vcpu_quota;
        if (-1 == virDomainSetSchedulerParametersFlags(domain, params, nparams, 0))
            goto cleanup;
        ret = 0;
    }

cleanup:
    virTypedParamsFree(params, nparams);
    return ret;
}

/* known vcpu_quota: ll */
int get_vcpu_quota(virDomainPtr domain, long long *vcpu_quota)
{
    int i;
    int type;
    virTypedParamsPtr params = NULL;
    virTypedParamsPtr param = NULL;
    int nparams = 0;
    int ret = -1;

    if (-1 == get_schedinfo(domain, &params, &nparams)) 
        goto cleanup;

    if (param = virTypedParamsGet(params, nparams, 
                VIR_DOMAIN_SCHEDULER_VCPU_QUOTA)) {
        *vcpu_quota = param->value.ul;
        ret = 0;
        return ret;
    }

cleanup:
    virTypedParamsFree(params, nparams);
    return ret;
}

int set_vcpu_bw(virDomainPtr domain, unsigned long long *vcpu_period, 
        long long *vcpu_quota) 
{
    int ret = -1;

    if (-1 == set_vcpu_period(domain, vcpu_period))
        goto cleanup;
    if (-1 == set_vcpu_quota(domain, vcpu_quota))
        goto cleanup;

    ret = 0;

cleanup:
    return ret;
}

/* 
 * get vcpu bandwidth in percentage [range: 0%~100%]
 */
double get_vcpu_bw(virDomainPtr domain, unsigned long long *vcpu_period,
        long long *vcpu_quota)
{
    int ret = 0;
    if (-1 == get_vcpu_period(domain, vcpu_period))
        goto cleanup;
    if (-1 == get_vcpu_quota(domain, vcpu_quota))
        goto cleanup;
    return (double)*vcpu_quota / *vcpu_period;

cleanup:
    return (double)ret;
}

/* 
 * Get blkio parameter of the given domain, it automatically allocate space
 * for params parameter so there is no need for you to initialize the variable
 * of "params" and "nparams". This function will decide the correct value for
 * them.
 * Return 0 in case of success and -1 in case of error
 */
int get_blkio_params(virDomainPtr domain, virTypedParameterPtr *params, 
        int *nparams)
{
    int ret = -1;
    /* 
     * First time to call virDomainGetBlkioParameters() to set the nparams 
     * variable 
     */
    if (-1 == virDomainGetBlkioParameters(domain, NULL, nparams, 0)) {
        fprintf(stderr, "Unable to get number of blkio parameters\n");
        goto cleanup;
    }
    *params = (virTypedParameterPtr)malloc(sizeof(**params) * (*nparams));

    /* 
     * Call virDomainGetBlkioParameters() again to fill information to the 
     * params struct 
     */
    if (-1 == virDomainGetBlkioParameters(domain, *params, nparams, 0)) {
        goto cleanup;
    }

    ret = 0;

cleanup:
    return ret;
}

int set_blkio_params(virDomainPtr domain, const char *field, const char *value)
{
    int ret = -1;
    virTypedParametersPtr params;
    int nparams = 0;

    if (-1 == get_blkio_params(domain, &params, &nparams)) 
        goto cleanup;
    if (-1 == set_params_value(params, nparams, field, value))
        goto cleanup;
    if (-1 == virDomainSetBlkioParameters(domain, params, nparams, 0))
        goto cleanup;

    ret = 0;

cleanup:
    virTypedParamsFree(params, nparams);
    return ret;
}

int get_blkio_read_iops(virDomainPtr domain, const char *str_riops)
{
    int i;
    int type;
    virTypedParamsPtr params = NULL;
    virTypedParamsPtr param = NULL;
    int nparams = 0;
    int ret = -1;

    if (-1 == get_blkio_params(domain, &params, &nparams)) 
        goto cleanup;
    if (param = virTypedParamsGet(params, nparams, 
                VIR_DOMAIN_BLKIO_DEVICE_READ_IOPS)) {
        *str_riops = param->value.s;
        ret = 0;
    }

cleanup:
    virTypedParamsFree(params, nparams);
    return ret;
}

int set_blkio_read_iops(virDomainPtr domain, const char *str_riops)
{
    return set_blkio_params(domain, VIR_DOMAIN_BLKIO_DEVICE_READ_IOPS, str_riops);
}

int get_blkio_write_iops(virDomainPtr domain, const char *str_wiops)
{
    int i;
    int type;
    virTypedParamsPtr params = NULL;
    virTypedParamsPtr param = NULL;
    int nparams = 0;
    int ret = -1;

    if (-1 == get_blkio_params(domain, &params, &nparams)) 
        goto cleanup;
    if (param = virTypedParamsGet(params, nparams, 
                VIR_DOMAIN_BLKIO_DEVICE_WRITE_IOPS)) {
        *str_wiops = param->value.s;
        ret = 0;
    }

cleanup:
    virTypedParamsFree(params, nparams);
    return ret;
}

int set_blkio_write_iops(virDomainPtr domain, const char *str_wiops)
{
    return set_blkio_params(domain, VIR_DOMAIN_BLKIO_DEVICE_WRITE_IOPS, *str_wiops);
}

int get_blkio_read_bytes(virDomainPtr domain, const char *str_rbps)
{
    int i;
    int type;
    virTypedParamsPtr params = NULL;
    virTypedParamsPtr param = NULL;
    int nparams = 0;
    int ret = -1;

    if (-1 == get_blkio_params(domain, &params, &nparams)) 
        goto cleanup;
    if (param = virTypedParamsGet(params, nparams, 
                VIR_DOMAIN_BLKIO_DEVICE_READ_BPS)) {
        *str_rbps = param->value.s;
        ret = 0;
    }

cleanup:
    virTypedParamsFree(params, nparams);
    return ret;
}

int set_blkio_read_bytes(virDomainPtr domain, const char *str_rbps)
{
    return set_blkio_params(domain, VIR_DOMAIN_BLKIO_DEVICE_READ_BPS, str_rbps);
}

int get_blkio_write_bytes(virDomainPtr domain, const char *str_wbps)
{
    int i;
    int type;
    virTypedParamsPtr params = NULL;
    virTypedParamsPtr param = NULL;
    int nparams = 0;
    int ret = -1;

    if (-1 == get_blkio_params(domain, &params, &nparams)) 
        goto cleanup;
    if (param = virTypedParamsGet(params, nparams, 
                VIR_DOMAIN_BLKIO_DEVICE_WRITE_BPS)) {
        *str_wbps = param->value.s;
        ret = 0;
    }

cleanup:
    virTypedParamsFree(params, nparams);
    return ret;
}

int set_blkio_write_bytes(virDomainPtr domain, const char *str_wbps)
{
    return set_blkio_params(domain, VIR_DOMAIN_BLKIO_DEVICE_WRITE_BPS, str_wbps);
}

int get_blkio_dev_weight(virDomainPtr domain, const char *str_dev_weight)
{
    int i;
    int type;
    virTypedParamsPtr params = NULL;
    virTypedParamsPtr param = NULL;
    int nparams = 0;
    int ret = -1;

    if (-1 == get_blkio_params(domain, &params, &nparams)) 
        goto cleanup;
    if (param = virTypedParamsGet(params, nparams, 
                VIR_DOMAIN_BLKIO_DEVICE_WEIGHT)) {
        *str_dev_weight = param->value.s;
        ret = 0;
    }

cleanup:
    virTypedParamsFree(params, nparams);
    return ret;
}

/*
 * params and nparams don't need to be initialized here, get_blkio() will do 
 * that for you set the blkio parameters of "weight" to the specific value
 * e.g. device_weight[] = "/dev/sda2,500"
 * Return 0 in case of success and -1 in case of error
 */
int set_blkio_dev_weight(virDomainPtr domain, const char *str_dev_weight)
{
    return set_blkio_params(domain, VIR_DOMAIN_BLKIO_DEVICE_WEIGHT, str_dev_weight);
}

/*
 * TODO: do not known weight data type, int or unsigned int
 */
int get_blkio_weight(virDomainPtr domain, const char *str_weight)
{
    int i;
    int type;
    virTypedParamsPtr params = NULL;
    virTypedParamsPtr param = NULL;
    int nparams = 0;
    int ret = -1;

    if (-1 == get_blkio_params(domain, &params, &nparams)) 
        goto cleanup;
    if (param = virTypedParamsGet(params, nparams, VIR_DOMAIN_BLKIO_WEIGHT)) {
        *str_weight = param->value.ui;
        ret = 0;
    }

cleanup:
    virTypedParamsFree(params, nparams);
    return ret;
}

/* 
 * wrapper for set_blkio() function
 */
int set_blkio_weight(virDomainPtr domain, const char *str_weight)
{
    return set_blkio_params(domain, VIR_DOMAIN_BLKIO_WEIGHT, str_weight);
}

/* 
 * Get memory parameters of the given domain, it automatically allocate space
 * for params parametes so there is no need for you to initialize the variable
 * of "params" and "nparams". This function will decide the correct value for
 * them.
 * Return 0 in case of success and -1 in case of error
 */
int get_mem_params(virDomainPtr domain, virTypedParameterPtr *params, 
        int *nparams)
{
    int ret = -1;

    if ((virDomainGetMemoryParameters(domain, NULL, nparams, 0) == 0)
            && (*nparams != 0)) {
        *params = (virTypedParameterPtr)malloc(sizeof(**params) * (*nparams));
        memset(params, 0, sizeof(**params) * (*nparams));
        if (0 != virDomainGetMemoryParameters(domain, *params, nparams, 0))
            goto cleanup;
        ret = 0;
    }

cleanup:
    return ret;
}

int set_mem_params(virDomainPtr domain, const char *field, const char *value)
{
    int ret = -1;
    virTypedParametersPtr params;
    int nparams = 0;

    if (-1 == get_mem_params(domain, &params, &nparams)) 
        goto cleanup;
    if (-1 == set_params_value(params, nparams, field, value))
        goto cleanup;
    if (-1 == virDomainSetMemoryParameters(domain, params, nparams, 0))
        goto cleanup;

    ret = 0;

cleanup:
    virTypedParamsFree(params, nparams);
    return ret;
}
