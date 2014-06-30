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

int set_params_value(virTypedParameterPtr params, int nparams, const char *field, int new_value);

int set_schedinfo(virDomainPtr domain, long long new_value);

int set_cpu_shares(virDomainPtr domain, long long new_cpu_share);

int set_vcpu_period(virDomainPtr domain, long long new_vcpu_period);

int set_vcpu_quota(virDomainPtr domain, long long new_vcpu_quota);

int set_vcpu_bw(virDomainPtr domain, long long, long long); 

int set_blkio(virDomainPtr domain, int new_weight);

int set_blkio_weight(virDomainPtr domain, int new_weight);


#endif
