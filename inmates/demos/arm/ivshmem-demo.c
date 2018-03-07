/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2014-2016
 *
 * Authors:
 *  Henning Schild <henning.schild@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */
#include <inmate.h>

#define VENDORID	0x1af4
#define DEVICEID	0x1110

#define IVSHMEM_CFG_SHMEM_PTR	0x40
#define IVSHMEM_CFG_SHMEM_SZ	0x48

#define JAILHOUSE_SHMEM_PROTO_UNDEFINED	0x0000

#define IVSHMEM_IRQ (154+32) /*get this vaule fron cell configuration vpci_irq_base + 32*/

#define MAX_NDEV	4
#define UART_BASE	0x3F8

static char str[64] = "Hello from bare-metal ivshmem-demo inmate!!!  ";
static int irq_counter;

struct ivshmem_dev_data {
	u16 bdf;
	u32 *registers;
	void *shmem;
	//u32 *msix_table;
	u64 shmemsz;
	u64 bar2sz;
};

static struct ivshmem_dev_data devs[MAX_NDEV];

static u64 pci_cfg_read64(u16 bdf, unsigned int addr)
{
	u64 bar;

	bar = ((u64)pci_read_config(bdf, addr + 4, 4) << 32) |
	      pci_read_config(bdf, addr, 4);
	return bar;
}

static void pci_cfg_write64(u16 bdf, unsigned int addr, u64 val)
{
	pci_write_config(bdf, addr + 4, (u32)(val >> 32), 4);
	pci_write_config(bdf, addr, (u32)val, 4);
}

static u64 get_bar_sz(u16 bdf, u8 barn)
{
	u64 bar, tmp;
	u64 barsz;

	bar = pci_cfg_read64(bdf, PCI_CFG_BAR + (8 * barn));
	pci_cfg_write64(bdf, PCI_CFG_BAR + (8 * barn), 0xffffffffffffffffULL);
	tmp = pci_cfg_read64(bdf, PCI_CFG_BAR + (8 * barn));
	barsz = ~(tmp & ~(0xf)) + 1;
	pci_cfg_write64(bdf, PCI_CFG_BAR + (8 * barn), bar);

	return barsz;
}

static int map_shmem_and_bars(struct ivshmem_dev_data *d)
{
	if (0 > pci_find_cap(d->bdf, PCI_CAP_MSIX)) {
		printk("IVSHMEM ERROR: device is not MSI-X capable\n");
		//return 1;
	}

	d->shmemsz = pci_cfg_read64(d->bdf, IVSHMEM_CFG_SHMEM_SZ);
	/*d->shmem = (void *)((u32)(0xffffffff & pci_cfg_read64(d->bdf, IVSHMEM_CFG_SHMEM_PTR)));*/
	d->shmem = (void *)((u64)(0xffffffffffffffff & pci_cfg_read64(d->bdf, IVSHMEM_CFG_SHMEM_PTR)));

	printk("IVSHMEM: shmem is at %p\n", d->shmem);
	d->registers =
	 (u32 *)(((u64)(d->shmem + d->shmemsz + PAGE_SIZE - 1)) & PAGE_MASK);
 	/* (u32 *)(((u32)(d->shmem + d->shmemsz + PAGE_SIZE - 1)) & PAGE_MASK);*/
	/*pci_cfg_write64(d->bdf, PCI_CFG_BAR, (u32)d->registers);*/
	pci_cfg_write64(d->bdf, PCI_CFG_BAR, (u64)d->registers);
	printk("IVSHMEM: bar0 is at %p\n", d->registers);
	d->bar2sz = get_bar_sz(d->bdf, 2);

	/*
	d->msix_table =
	  (u32 *)(d->registers + PAGE_SIZE);
	pci_cfg_write64(d->bdf, PCI_CFG_BAR + 16, (u32)d->msix_table);
	printk("IVSHMEM: bar2 is at %p\n", d->msix_table);
	*/

	pci_write_config(d->bdf, PCI_CFG_COMMAND,
			 (PCI_CMD_MEM | PCI_CMD_MASTER), 2);
	return 0;
}

static u32 get_ivpos(struct ivshmem_dev_data *d)
{
	return mmio_read32(d->registers + 2);
}

static void send_irq(struct ivshmem_dev_data *d)
{
#if 0
	printk("IVSHMEM: %02x:%02x.%x sending IRQ (by writing 1 to %p)\n",
	       d->bdf >> 8, (d->bdf >> 3) & 0x1f, d->bdf & 0x3,
	       (void*)(d->registers + 3));
#endif
	mmio_write32(d->registers + 3, 1);
}

static void enable_irq(struct ivshmem_dev_data *d)
{
	printk("IVSHMEM: Enabling IVSHMEM_IRQs\n");
	mmio_write32(d->registers, 0xffffffff);
}

static void handle_irq(unsigned int irqn)
{
	printk("IVSHMEM: handle_irq(irqn:%d) - interrupt #%d\n",
	       irqn, irq_counter++);
}

void inmate_main(void)
{
	unsigned int i = 0;
	/* FIXME: Get the PCI configuration space base address from
	// command line arguments
	// (originally, get it from the root cell config
	// file:config.header.platform_info.pci_mmconfig_base) */
	int bdf = 0;
	unsigned int class_rev;
	struct ivshmem_dev_data *d = NULL;
	volatile char *shmem;
	int ndevices = 0;

	gic_setup(handle_irq);

	while ((ndevices < MAX_NDEV) &&
	       (-1 != (bdf = pci_find_device(VENDORID, DEVICEID, bdf)))) {
		printk("IVSHMEM: Found %04x:%04x at %02x:%02x.%x\n",
		       pci_read_config(bdf, PCI_CFG_VENDOR_ID, 2),
		       pci_read_config(bdf, PCI_CFG_DEVICE_ID, 2),
		       bdf >> 8, (bdf >> 3) & 0x1f, bdf & 0x3);
		class_rev = pci_read_config(bdf, 0x8, 4);
		if (class_rev != (PCI_DEV_CLASS_OTHER << 24 |
				  JAILHOUSE_SHMEM_PROTO_UNDEFINED << 8)) {
			printk("IVSHMEM: class/revision %08x, not supported "
			       "skipping device\n", class_rev);
			bdf++;
			continue;
		}
		ndevices++;
		d = devs + ndevices - 1;
		d->bdf = bdf;
		if (map_shmem_and_bars(d)) {
			printk("IVSHMEM: Failure mapping shmem and bars.\n");
			return;
		}

		printk("IVSHMEM: mapped shmem and bars, got position %p\n",
		       get_ivpos(d));

		/* NULL terminating the string */
		str[63] = 0;
		printk("IVSHMEM: memcpy(%p, %s, %d)\n", d->shmem, str, 64);
		memcpy(d->shmem, str, 64);
		printk("IVSHMEM: %p:%s\n", d->shmem, d->shmem);

		gic_enable_irq(IVSHMEM_IRQ + ndevices - 1);
		printk("IVSHMEM: Enabled IRQ:0x%x\n", IVSHMEM_IRQ +  ndevices -1);

		enable_irq(d);

		//pci_msix_set_vector(bdf, IVSHMEM_IRQ + ndevices - 1, 0);
		//printk("IVSHMEM: Vector set for PCI MSI-X.\n");
		bdf++;
	}

	if (!ndevices) {
		printk("IVSHMEM: No PCI devices found .. nothing to do.\n");
		return;
	}

	printk("IVSHMEM: Done setting up...\n");

	{

		u8 buf[64];
		memcpy(buf, d->shmem, sizeof(buf)/sizeof(buf[0]));
		buf[63] = 0;
		printk("IVSHMEM: %s\n", buf);
		memcpy(d->shmem, str, sizeof(str)/sizeof(str[0]));
	}

	while (1) {
		for (i = 0; i < ndevices; i++) {
			d = devs + i;
			//delay_us(1000*1000);
			shmem = d->shmem;
			shmem[19]++;
			send_irq(d);
		}
		printk("IVSHMEM: waiting for interrupt.\n");
		asm volatile("wfi" : : : "memory");
	}
}
