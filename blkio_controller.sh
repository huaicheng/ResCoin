#!/bin/bash

CGDIR=/sys/fs/cgroup
BLKIO_CGDIR=$CGDIR/blkio
MACHDIR=$BLKIO_CGDIR/machine
VMASTER_DIR=$MACHDIR/vmaster.libvirt-qemu
VSLAVE01_DIR=$MACHDIR/vslave01.libvirt-qemu

function get_blkio_time()  #1: blkio.time
{ 
    cat $1 | awk '{print $2}'
}
echo "Welcome to the VM blkio weight changing test"
echo "====================old blkio weight================="
printf "%-10s\t%s\n" "VMASTER"  "`cat $VMASTER_DIR/blkio.weight`"
printf "%-10s\t%s\n" "VSLAVE01" "`cat $VSLAVE01_DIR/blkio.weight`"

echo "input new weight for vmaster:"
read vm_weight
echo "lenght(vm_weight)=${#vm_weight[@]}"
if [[ $vm_weight -gt 0 ]] && [[ $vm_weight -lt 1000 ]]; then
    echo "$vm_weight" > $VMASTER_DIR/blkio.weight
fi

echo "input new weight for vslave01:"
read vs_weight
if [[ $vs_weight -gt 0 ]] && [[ $vs_weight -lt 1000 ]]; then
    echo "$vs_weight" > $VSLAVE01_DIR/blkio.weight
fi

echo "====================current blkio weight================="
printf "%-10s\t%s\n" "VMASTER"  "`cat $VMASTER_DIR/blkio.weight`"
printf "%-10s\t%s\n" "VSLAVE01" "`cat $VSLAVE01_DIR/blkio.weight`"

vm1_time=`get_blkio_time $VMASTER_DIR/blkio.time`
vs1_time=`get_blkio_time $VSLAVE01_DIR/blkio.time`
echo "Now we will run dd tasks on each of the two VMs..."
ssh vmaster "time dd if=/dev/vda of=/dev/null bs=1M count=1000 &" &
echo "vmaster dd started..."
ssh vslave01 "time dd if=/dev/vda of=/dev/null bs=1M count=1000 &" &
echo "vslave01 dd started..."

sleep 10
vm2_time=`get_blkio_time $VMASTER_DIR/blkio.time`
vs2_time=`get_blkio_time $VSLAVE01_DIR/blkio.time`

echo "******IO time summary******"
printf "%-10s\t%s\n" "VMASTER"  "`expr $vm2_time - $vm1_time`"
printf "%-10s\t%s\n" "VSLAVE01" "`expr $vs2_time - $vs1_time`"
