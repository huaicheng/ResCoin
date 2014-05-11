#!/bin/awk -f
BEGIN {
    user = 0
    nice = 0
    sys = 0
    idle = 0
    iowait = 0
    irq = 0
    softirq = 0
    steal = 0
    guest = 0
    guest_nice = 0
    sum = 0
}

{
    user += $2
    nice += $3
    sys += $4
    idle += $5
    iowait += $6
    irq += $7
    softirq += $8
    steal += $9
    guest += $10
    guest_nice += $11

    printf "%8s%8s%8s%8s%8s%8s%8s%8s%8s%8s\n", $2, $3, $4, $5, $6, $7, $8, $9, $10, $11
}

END {
    printf "%8s%8s%8s%8s%8s%8s%8s%8s%8s%8s\n", user, nice, sys, idle, iowait, irq, softirq, steal, guest, guest_nice
}
