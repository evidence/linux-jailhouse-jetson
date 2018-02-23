JAILHOUSE for NVIDIA Jetson TX1
===============================

This is the Jailhouse hypervisor [1] for the default **4.4** Linux kernel [2]
provided by NVIDIA for the Jetson TX1 platform [3]. The porting activity has
been done in the context of the HERCULES European project [4].

Please, refer to the original website [1] for further information about Jailhouse
or other platforms/kernel versions.


Linux kernel build & Installation
---------------------------------

For installing Jailhouse, a copy of the compiled Linux kernel with all object
files is needed, to be able of building the kernel module.
Additionally, you need to configure the kernel using the configuration
available in the ```kernel/config``` file.

Note that NVIDIA provides its own Linux kernel (not Vanilla).
The kernel sources are available at [6].

Some scripts available in [5] allow to automatically download and build such
kernel for the TX1 platform, also fixing a few issues that prevent a successful
build.


Jailhouse build & Installation
------------------------------

To build and install jailhouse just type:

    sudo make KDIR=/path/to/compiled/kernel/ install

The hypervisor requires a contiguous piece of RAM for itself and each
additional cell. This currently has to be pre-allocated during boot-up.
On ARM platforms this is usually achieved by reducing the amount of memory seen
by the Linux kernel. You therefore need to modify the kernel boot arguments
adding ```mem=3968M vmalloc=512M``` (on TX1 this can be written inside the
```/boot/extlinux/extlinux.conf``` file).


Serial port assignment
----------------------

Usually, the serial port is assigned exclusively to the inmate, especially if
it does not run a full-fledged operating system capable of using more complex
hardware (e.g., a display). This is the case, for example, of the gic-demo
illustrated below, which prints its output directly on the serial console.

A FTDI USB cable can be used to physically connect the platform's serial
console to a host machine. The following picture shows how pins must be
connected on the platform side. More information about this connection is
available at [7]. You can then install a serial terminal program on the host
machine (e.g., Putty or minicom), set a 115200 baudrate and connect to the
board.

<p align="center">
<img src="images/TX1_serial_cable.jpg" width="400">
</p>

Then, Linux must be prevented from starting a console on the serial
port. This can be done by removing the ```console=ttyS0,115200n8``` parameter
from the boot arguments (keep ```earlyprintk=uart8250-32bit,0x70006000
console=tty0``` as it is useful for interface initialization).
Since the ```cbootargs``` environment variable gets automatically overwritten
at each boot, the best way to remove such option is to change the ```bootcmd``
variable by typing

    setenv bootcmd setenv cbootargs root=/dev/mmcblk0p1 rw rootwait OS=l4t fbcon=map:0 net.ifnames=0 tegraid=21.1.2.0.0 ddr_die=2048M@2048M ddr_die=2048M@4096M section=256M memtype=0 vpr_resize usb_port_owner_info=0 lane_owner_info=0 emc_max_dvfs=0 touch_id=0@63 video=tegrafb no_console_suspend=1 debug_uartport=lsport,0 maxcpus=4 usbcore.old_scheme_first=1 lp0_vec=0x1000@   0xff2bf000 nvdumper_reserved=0xff23f000 core_edp_mv=1075 core_edp_ma=4000 gpt earlyprintk=uart8250-32bit,0x70006000 console=tty0 \; run distro_bootcmd

    saveenv

    reset

After this change, Linux will not start the console on the serial port anymore.
The console will still be reachable through HDMI. Alternatively, before
disabling the serial port, you can assign a static IP to the platform by
appending to ```/etc/network/interfaces``` the needed information:

	auto eth0
	iface eth0 inet static
	address ...
	netmask ...
	gateway ...


Jailhouse usage
---------------

Once the boot arguments has been modified and the machine rebooted, to run the
hypervisor type:

	sudo sh -c 'echo 1 > /sys/kernel/debug/cpuidle_t210/fast_cluster_states_enable'
	sudo modprobe jailhouse
	sudo jailhouse enable jailhouse/configs/jetson-tx1.cell

Performance can be improved by setting the performance CPU frequency governor:

	sudo sh -c 'echo performance > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor'


Jailhouse demonstration
-----------------------

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
* [6] NVIDIA Linux kernel sources: http://developer.download.nvidia.com/embedded/L4T/r28_Release_v1.0/BSP/source_release.tbz2
* [7] Serial console http://www.jetsonhacks.com/2015/12/01/serial-console-nvidia-jetson-tx1/


