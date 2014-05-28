#!/bin/bash

. ./funclib.sh

cat $CGROUP_ROOT/blkio/machine/blkio.throttle.read_bps_device
cat $CGROUP_ROOT/blkio/machine/blkio.throttle.write_bps_device
cat $CGROUP_ROOT/blkio/machine/*/blkio.throttle.read_bps_device
cat $CGROUP_ROOT/blkio/machine/*/blkio.throttle.write_bps_device

# 10MBps
#BPSLIMIT=10485760

# 200MBps
#BPSLIMIT=209715200

# 50MBps
#BPSLIMIT=52428800

# 30MBps
BPSLIMIT=31457280

# unlimited / default
#BPSLIMIT=0

DEV_MAJOR=253
DEV_MINOR_LIST="0 1 2"

if [ -n "$1" ]
then
	BPSLIMIT="$1"
fi

virsh list --name | while read n
do
	[ -n "$n" ] || continue
	echo $n
	
	for DEV_MINOR in $DEV_MINOR_LIST
	do
		echo "$DEV_MAJOR:$DEV_MINOR $BPSLIMIT" > $CGROUP_ROOT/blkio/machine/${n}.libvirt-qemu/blkio.throttle.read_bps_device
		echo "$DEV_MAJOR:$DEV_MINOR $BPSLIMIT" > $CGROUP_ROOT/blkio/machine/${n}.libvirt-qemu/blkio.throttle.write_bps_device
	done
	
	cat "$CGROUP_ROOT/blkio/machine/${n}.libvirt-qemu/blkio.throttle.read_bps_device"
	cat "$CGROUP_ROOT/blkio/machine/${n}.libvirt-qemu/blkio.throttle.write_bps_device"
done

cat $CGROUP_ROOT/blkio/machine/blkio.throttle.read_bps_device
cat $CGROUP_ROOT/blkio/machine/blkio.throttle.write_bps_device
cat $CGROUP_ROOT/blkio/machine/*/blkio.throttle.read_bps_device
cat $CGROUP_ROOT/blkio/machine/*/blkio.throttle.write_bps_device

