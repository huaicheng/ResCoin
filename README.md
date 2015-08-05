# ResCoin #
"Resource Coin" Prototype System in pursuit of fairness and performance of virtualizd resource scheduling

Author: [Coperd](http://people.cs.uchicago.edu/~huaicheng)  
Email: <<lhcwhu@gmail.com>>   
Date: 2014-04-01
* * *

- ###### Requirement: ##

    - KVM 
        - CPU: Take Intel VT (AMD CPU has similar SVM instruction set) as an example 
          		
        		$ grep "vmx" /proc/cpuinfo # check whether Intel VT is supported
      
        - Linux: kvm.ko, kvm_intel.ko / kvm_amd.ko loaded
         
       
        		$ modprobe kvm.ko
        		$ modprobe kvm_intel.ko 
        
    - QEMU - used to emualte hardware device for VMs (using KVM to accelerate default since v1.6) 
    - Libvirt
        - enable cgroup support (/etc/libvirt/qemu.conf)
        - network: bridged (enable each VM to have network connection) 
        - better pin vcpu to physical cpu
    - Cgroup subsystems needed
        - cpu system: CFS propotional share
        - blkio system: CFQ throttling 
        - memory system
        - network system (currently not supported in ResCoin)

- ###### Compilation && Running:

    - make 
        - ResCoin: the main program
        - gmmagent: memory agent running in each VM 
        - clean: remove all the object and executable files
    - setting up
        - make sure each VM has network connection with the host
        - copy gmmagent to each VM and run it first  
        
        		# change user and guest to your real USER and HOSTNAME accordingly
        		$ scp gmmagent user@guest:~/ 
        		$ ssh user@guest "./gmmagent"
        		
        - start ResCoin in the host to take effect
        	
        		$ nohup ./ResCoin &

- ###### Source Tree:
    - common.h: some string handling wrappers
    - ringbuffer.c: simple ring buffer implementation used to store historic 
                    workload data
    - monitor.c: resource utilization monitoring using libvirt and /proc
    - ewma.c: the workload prediction module using EWMA 
    - rc.c: the resource control wrappers using libvirt
    - schedule.c: the implementation of our scheduling algorithm
    - gmmagent.c: the memory agent running in VMs to collect their memory usage
    - main.c: main entry of the whole system

- ###### System Architecture:


	                                  Architecture of ResCoin                            
    	                                                
     	                 	      ╔══════════════════════════════╗
  	                              ║   VM Resource Utilization    ║
 	                              ║                              ║
   	                       		  ║            +------------+    ║
	   	┌───────────────┐         ║ <--libvirt-+    CPU     |<───╫─────┐
	   	│               │         ║ |          +------------+    ║     │
	   	s       +-------v-------+ ║ |            +------------+  ║     │
	   	c       |    monitor    <-╬-<--gmmagent--+   Memory   |<─╫─────┤ (other VMs)
	   	h       +-------+-------+ ║ |            +------------+  ║     │      ^
	   	e               |         ║ |          +------------+    ║     │      │
	   	d               v         ║ <--libvirt-+    Disk    |<───╫─────┤      │
	   	u       +---------------+ ║            +------------+    ║     │      │
	   	l       |   predictor   | ╚══════════════════════════════╝     │      │
	   	i       +-------+-------+                                      │      │
	   	n               |                                              │      │
	   	g               v                                              │      │
	   	|       +---------------+   new shares of multiple resources   |      │
	   	p       |   scheduler   ┼─────────────────────────────────────-+──────┘
	   	e       +-------+-------+   resource control using cgroup
	   	r    <our fairness algorithm>
	   	i               │        
	   	o               │
	   	d               v
	   	│               │
	   	│               │
	   	└───────────────┘



- Scheduling Algorithm:  
  - Input: au(actual utilization), est(workload estimation), tc(template capacity)
	 
		  initialize 〖ac〗_i←0,(i=1..n)
		  for each period t; do
				〖ac〗_i (t) +=〖tc〗_i (t-1) - 〖au〗_i (t-1) - [〖tc〗_i (t-k-1)
 	    	                       -〖au〗_i (t-k-1)]; 
				for each VMi in vmlist; do // do the same thing for each resource
					if (∑_(i=0)^n 〖est〗_i   ≤ C_h  && 〖est〗_i≤ 〖tc〗_i); then 
  	             	    // tc satisfied
						allocate(〖est〗_i);
					else
						C_e= C_h-∑_(i=0)^(n-1)〖min(〖est〗_i (t-1),〖tc〗_i (t))〗
						〖wr〗_i=〖∥ac〗_i∥∕∑_(i=0)^n ∥〖ac〗_i 〗∥; 
	       	            // resource weight calibration
 						allocate(〖〖tc〗_i+wr〗_i∙C_e^T);  // resource allocation
 					end if
	 			end for
		 	end for
		
  - Output: resource matrix
