set terminal post color enh 
set output "wl.ps"
set title "kernbench test workload"
set xlabel "time"
set ylabel "%CPU"
set xrange [0:210]
set arrow from 0.0,25.0 to 210,25.0 nohead lt 16
#set autoscale
plot "localhost.localdomai.rst" u 2:3 w l lt 1 t "PM", "VM" u 2:3 w l lt 3 lw 2 t "VM" 
#plot "PM" u 2:3 w lp pt 2 title "PM"
