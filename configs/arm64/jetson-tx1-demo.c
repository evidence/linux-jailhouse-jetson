/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration for uart+ivshmem demo inmate on Nvidia Jetson TX1:
 * 1 CPU, 64K RAM, serial port 0
 *
 * Copyright (c)  2018 Evidence Srl
 *
 * Authors:
 *  Luca Cuomo <l.cuomo@evidence.eu.com>
 *
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>

#define ARRAY_SIZE(a) sizeof(a) / sizeof(a[0])

struct {
	struct jailhouse_cell_desc cell;
	__u64 cpus[1];
	struct jailhouse_memory mem_regions[5];
	struct jailhouse_irqchip irqchips[2];
	struct jailhouse_pci_device pci_devices[2];
} __attribute__((packed)) config = {
	.cell = {
		.signature = JAILHOUSE_CELL_DESC_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.name = "jetson-tx1-demo",
		.flags = JAILHOUSE_CELL_PASSIVE_COMMREG,

		.cpu_set_size = sizeof(config.cpus),
		.num_memory_regions = ARRAY_SIZE(config.mem_regions),
		.num_irqchips = ARRAY_SIZE(config.irqchips),
		.num_pci_devices = ARRAY_SIZE(config.pci_devices),
		/*On Jetson TX1 the IRQs from 212 to 223 are not assigned.
		The bare metal cell will use IRQs from 218 to 223.
		Note: Jailhouse adds 32 (GIC's SPI) to the .vpci_irq_base,
		so 186 is the base value*/
		.vpci_irq_base = 186,
	},

	.cpus = {
		0x8,
	},

	.mem_regions = {
		/* UART */ {
			.phys_start = 0x70006000,
			.virt_start = 0x70006000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},
		/* RAM */ {
			.phys_start = 0x17bfe0000,
			.virt_start = 0,
			.size = 0x00010000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_LOADABLE,
		},
		/* communication region */ {
			.virt_start = 0x80000000,
			.size = 0x00001000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_COMM_REGION,
		},
		/* IVHSMEM  1*/ {
                        .phys_start = 0x17ba00000,
                        .virt_start = 0x17ba00000,
                        .size = 0x100000,
                        .flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE  |
                                JAILHOUSE_MEM_ROOTSHARED,

                },

                /* IVHSMEM  2*/ {
                        .phys_start = 0x17bd00000,
                        .virt_start = 0x17bd00000,
                        .size = 0x100000,
                        .flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
                                JAILHOUSE_MEM_ROOTSHARED ,

                },
	},

	.irqchips = {
		/* GIC */ {
			.address = 0x50041000,
                        .pin_base = 32,
                        /* Interrupts:
                           46 for UART C  */
                        .pin_bitmap = {
				0,
				1<<(46-32)
                        },
                },

		/* GIC */ {
			.address = 0x50041000,
                        .pin_base = 160,
                        /* Interrupts:
                           186 for IVSHMEM,
                           belongs to the bare metal cell  */
                        .pin_bitmap = {
				0,
				3<<(186-160)
                        },
                },
        },

	.pci_devices = {
                {
                        .type = JAILHOUSE_PCI_TYPE_IVSHMEM,
                        .bdf = 0x0 << 3,
                        .bar_mask = {
                                0xffffff00, 0xffffffff, 0x00000000,
                                0x00000000, 0x00000000, 0x00000000,
                        },
			/* num_msix_vectors needs to be 0 for INTx operation*/
			.num_msix_vectors = 0, 
                        .shmem_region = 3, /* must be no of IVSHMEM region above */
			.shmem_protocol = JAILHOUSE_SHMEM_PROTO_UNDEFINED,
                },
		 {
                        .type = JAILHOUSE_PCI_TYPE_IVSHMEM,
                        .bdf = 0xf << 3,
                        .bar_mask = {
                                0xffffff00, 0xffffffff, 0x00000000,
                                0x00000000, 0x00000000, 0x00000000,
                        },
                        /* num_msix_vectors needs to be 0 for INTx operation*/
                        .num_msix_vectors = 0,
                        .shmem_region = 4, /* must be no of IVSHMEM region above */
                        .shmem_protocol = JAILHOUSE_SHMEM_PROTO_UNDEFINED,
                },
        },
};
