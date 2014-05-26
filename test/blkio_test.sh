#!/bin/bash

source ./funclib.sh

CGDIR=/sys/fs/cgroup
BLKCGD=$CGDIR/blkio
T1D=$BLKCGD/test1
T2D=$BLKCGD/test2

fl_mkdir $T1D $T2D

echo 1000 > $T1D/blkio.weight
echo 500  > $T2D/blkio.weight

sync
echo 3 > /proc/sys/vm/drop_caches

t11_time=`get_blkio_time $T1D/blkio.time`
t21_time=`get_blkio_time $T2D/blkio.time`

dd if=/dev/sda of=/dev/null &
echo $! > $T1D/tasks
cat $T1D/tasks

dd if=/dev/sda of=/dev/null &
echo $! > $T2D/tasks
cat $T2D/tasks

sleep 100

# kill all the "dd" tasks
#for i in `ps -ef | grep -w dd | grep -v grep | awk '{print $2}'`
#do
    #kill -9 $i 2>/dev/null
#done


t12_time=`get_blkio_time $T1D/blkio.time`
t22_time=`get_blkio_time $T2D/blkio.time`

printf "test1: %s\n" "`expr $t12_time - $t11_time`"
printf "test2: %s\n" "`expr $t22_time - $t21_time`"

echo "removing result files..."
rm -rf test1_file.out test2_file.out
echo "Test Done!"
