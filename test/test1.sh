#!/bin/bash

sync; echo 3 > /proc/sys/vm/drop_caches

dd if=test1_file of=/dev/null &
echo $! > /sys/fs/cgroup/blkio/test1/tasks
echo 1000 > /sys/fs/cgroup/blkio/test1/blkio.weight
cat /sys/fs/cgroup/blkio/test1/tasks
