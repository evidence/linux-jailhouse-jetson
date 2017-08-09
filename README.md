JAILHOUSE for NVIDIA Jetson TX1
===============================

This is the Jailhouse hypervisor [1] for the default **4.4** Linux kernel [2]
running on NVIDIA Jetson TX1 platform [3]. The porting activity has been done
in the context of the HERCULES European project [4]. 

Plase, refer to the original website [1] for further information about Jailhouse
or other platforms/kernel versions.


Build & Installation
--------------------

To build Jailhouse, a copy of the compiled Linux kernel with all object files
is needed, to be able of building a kernel module.
You can refer to the guide in [5] for compiling a kernel for the TX1 platform.

To build and install jailhouse just type:

    sudo make KDIR=/path/to/compiled/kernel/ install

The hypervisor requires a contiguous piece of RAM for itself and each
additional cell. This currently has to be pre-allocated during boot-up.
On ARM platforms this is usually achieved by reducing the amount of memory seen
by the Linux kernel. You therefore need to modify the kernel boot arguments
adding ```mem=3968M vmalloc=512M``` (on TX1 this can be written inside the
```/boot/extlinux/extlinux.conf``` file).

Since the demo writes its output directly to the serial port, you also need to
make sure that the kernel command line does *not* have its console on the
serial port. In particular, you have to remove the ```console=ttyS0,115200n8```
parameter from the boot arguments (you can keep
```earlyprintk=uart8250-32bit,0x70006000 console=tty0``` which indeed are useful
for interface initialization).
Since the ```cbootargs``` environment variable gets automatically overwritten
at each boot, the best way to remove such option is to change the ```bootcmd``
variable by typing

    setenv bootcmd setenv cbootargs root=/dev/mmcblk0p1 rw rootwait OS=l4t fbcon=map:0 net.ifnames=0 tegraid=21.1.2.0.0 ddr_die=2048M@2048M ddr_die=2048M@4096M section=256M memtype=0 vpr_resize usb_port_owner_info=0 lane_owner_info=0 emc_max_dvfs=0 touch_id=0@63 video=tegrafb no_console_suspend=1 debug_uartport=lsport,0 maxcpus=4 usbcore.old_scheme_first=1 lp0_vec=0x1000@   0xff2bf000 nvdumper_reserved=0xff23f000 core_edp_mv=1075 core_edp_ma=4000 gpt earlyprintk=uart8250-32bit,0x70006000 console=tty0 \; run distro_bootcmd

    saveenv

    reset


Usage
-----

Once the boot arguments has been modified and the machine rebooted, to run the
hypervisor type:

	sudo sh -c 'echo 1 > /sys/kernel/debug/cpuidle_t210/fast_cluster_states_enable'
	sudo modprobe jailhouse
	sudo jailhouse enable jailhouse/configs/jetson-tx1.cell

Performance can be improved by setting the performance CPU frequency governor:

	sudo sh -c 'echo performance > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor'


Demonstration
-------------

Next, you can create a cell with a demonstration application as follows:

	sudo jailhouse cell create jailhouse/configs/jetson-tx1-demo.cell
	sudo jailhouse cell load jetson-tx1-demo jailhouse/inmates/demos/arm64/gic-demo.bin
	sudo jailhouse cell start jetson-tx1-demo

This application will program a periodic timer interrupt, measuring the jitter
and displaying the result on the console.

After creation, cells are addressed via the command line tool by providing
their names or their runtime-assigned IDs. You can obtain information about
active cells this way:

	jailhouse cell list

You can also obtain statistical information about the number of VM exists:

	jailhouse cell stats jetson-tx1-demo

Cell destruction is performed through the following command:

	sudo jailhouse cell destroy jetson-tx1-demo

Finally, the jailhouse hypervisor can be disabled by typing:

	sudo jailhouse disable

References
----------

* [1] Jailhouse hypervisor: https://github.com/siemens/jailhouse
* [2] Linux for Tegra: https://developer.nvidia.com/embedded/linux-tegra
* [3] NVIDIA Jetson TX1 platform: http://www.nvidia.com/object/jetson-tx1-dev-kit.html
* [4] HERCULES EU project: http://hercules2020.eu
* [5] Build TX1 Kernel and Modules: https://github.com/jetsonhacks/buildJetsonTX1Kernel


