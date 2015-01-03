#ifndef _RC_H_
#define _RC_H_

#include <libvirt/libvirt.h>
#include <libvirt/libvirt-qemu.h>
#include <libvirt/virterror.h>
#include <errno.h>

/* VM scheduling infomation */
typedef struct vm_schedinfo {
    int cpu_shares;
    long long vcpu_period;
    long long vcpu_quota;
    long long emulator_period;
    long long emulator_quota;
}vm_schedinfo;

/* VM blkio information */
typedef struct vm_blkiotune {
    int weight;
    int device_weight;
}vm_blkiotune;

/* used for VM resource scheduling */
typedef struct vm_resource {
    vm_schedinfo cpu_sched;
    vm_blkiotune blkio_resource;
    /* many more resources can be added here */
}vm_resource;


int get_schedinfo(virDomainPtr domain, virTypedParameterPtr *params, int *nparams);

void print_typed_parameters(virTypedParameterPtr domain, int nparams);

int set_params_value(virTypedParameterPtr params, int nparams, const char *field, const char *value);

int set_schedinfo(virDomainPtr domain, const char *field, const char *value);

int set_cpu_shares(virDomainPtr domain, const char *cpu_shares);

int set_cpu_shares_ull(virDomainPtr domain, unsigned long long *cpu_shares);

int get_cpu_shares(virDomainPtr domain, unsigned long long *cpu_shares);

int set_vcpu_period(virDomainPtr domain, const char *vcpu_period);

int set_vcpu_period_ull(virDomainPtr domain, unsigned long long *vcpu_period);

int get_vcpu_period(virDomainPtr domain, unsigned long long *vcpu_period);

int set_vcpu_quota(virDomainPtr domain, const char *vcpu_quota);

int set_vcpu_quota_ll(virDomainPtr domain, long long *vcpu_quota);

int get_vcpu_quota(virDomainPtr domain, long long *vcpu_quota);

int set_vcpu_bw(virDomainPtr domain, unsigned long long *vcpu_period, long long *vcpu_quota); 

double get_vcpu_bw(virDomainPtr domain, unsigned long long *vcpu_period, long long *vcpu_quota);

int get_blkio_params(virDomainPtr domain, virTypedParameterPtr *params, int *nparams);

int set_blkio_params(virDomainPtr domain, const char *field, const char *value);

int get_blkio_read_iops(virDomainPtr domain,  char *str_riops);

int set_blkio_read_iops(virDomainPtr domain, const char *str_riops);

int get_blkio_write_iops(virDomainPtr domain, char *str_wiops);

int set_blkio_write_iops(virDomainPtr domain, const char *str_wiops);

int get_blkio_read_bytes(virDomainPtr domain, char *str_rbps);

int set_blkio_read_bytes(virDomainPtr domain, const char *str_rbps);

int get_blkio_write_bytes(virDomainPtr domain, char *str_wbps);

int set_blkio_write_bytes(virDomainPtr domain, const char *str_wbps);

int get_blkio_dev_weight(virDomainPtr domain,  char *str_dev_weight);

int set_blkio_dev_weight(virDomainPtr domain, const char *str_dev_weight);

int get_blkio_weight(virDomainPtr domain,  char *str_weight);

int set_blkio_weight(virDomainPtr domain, const char *str_weight);

int get_mem_params(virDomainPtr domain, virTypedParameterPtr *params, int *nparams);

int set_mem_params(virDomainPtr domain, const char *field, const char *value);

#endif
