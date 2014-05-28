#!/bin/bash

. ./funclib.sh

virsh list --name | while read n
do
	[ -n "$n" ] || continue
	echo $n
	echo "$CGROUP_ROOT/cpu/machine/${n}.libvirt-qemu/cpu.shares"
	cat "$CGROUP_ROOT/cpu/machine/${n}.libvirt-qemu/cpu.shares"
	vcpu=$(virsh vcpucount --current --maximum  $n | head -n 1)
    shares=$((1024*$vcpu))
	echo "$shares" > "$CGROUP_ROOT/cpu/machine/${n}.libvirt-qemu/cpu.shares"
	cat "$CGROUP_ROOT/cpu/machine/${n}.libvirt-qemu/cpu.shares"
done
