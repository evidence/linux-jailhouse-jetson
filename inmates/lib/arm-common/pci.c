/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2014
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <inmate.h>

// PCI configuration space base address taken from the
// root cell config file:config.header.platform_info.pci_mmconfig_base
//#define PCI_CFG_BASE	(0x02000000)
#define PCI_CFG_BASE	(0x48000000)

u32 pci_read_config(u16 bdf, unsigned int addr, unsigned int size)
{
	/*u32 reg_addr originally*/
	u64 reg_addr = PCI_CFG_BASE | ((u32)bdf << 8) | (addr & 0xfc);
	//printk("%s(bdf:0x%x, addr:%p, size:0x%x), reg_addr0x%x\n", __func__, bdf, addr, size, reg_addr);
	switch (size) {
	case 1:
		return mmio_read8((u8 *)(reg_addr + (addr & 0x3)));
	case 2:
		return mmio_read16((u16 *)(reg_addr + (addr & 0x3)));
	case 4:
		return mmio_read32((u32 *)(reg_addr));
	default:
		return -1;
	}
}

void pci_write_config(u16 bdf, unsigned int addr, u32 value, unsigned int size)
{
	/* u32 reg_addr  originally */
	u64 reg_addr = PCI_CFG_BASE | ((u32)bdf << 8) | (addr & 0xfc);
	//printk("%s(bdf:0x%x, addr:%p, value:0x%x, size:0x%x), reg_addr0x%x\n", __func__, bdf, addr, value, size, reg_addr);
	switch (size) {
	case 1:
		mmio_write8((u8 *)(reg_addr + (addr & 0x3)), value);
		break;
	case 2:
		mmio_write16((u16 *)(reg_addr + (addr & 0x3)), value);
		break;
	case 4:
		mmio_write32((u32 *)(reg_addr), value);
		break;
	}
}

void pci_msix_set_vector(u16 bdf, unsigned int vector, u32 index)
{
	int cap = pci_find_cap(bdf, PCI_CAP_MSIX);
	unsigned int bar;
	u64 msix_table = 0;
	u32 addr;
	u16 ctrl;
	u32 table;

	if (cap < 0)
		return;
	ctrl = pci_read_config(bdf, cap + 2, 2);
	/* bounds check */
	if (index > (ctrl & 0x3ff))
		return;
	table = pci_read_config(bdf, cap + 4, 4);
	bar = (table & 7) * 4 + PCI_CFG_BAR;
	addr = pci_read_config(bdf, bar, 4);

	if ((addr & 6) == PCI_BAR_64BIT) {
		msix_table = pci_read_config(bdf, bar + 4, 4);
		msix_table <<= 32;
	}
	msix_table |= addr & ~0xf;
	msix_table += table & ~7;

	/* enable and mask */
	ctrl |= (MSIX_CTRL_ENABLE | MSIX_CTRL_FMASK);
	pci_write_config(bdf, cap + 2, ctrl, 2);

	msix_table += 16 * index;
	/*
	mmio_write32((u32 *)((u32)msix_table), 0xfee00000 | cpu_id() << 12);
	mmio_write32((u32 *)((u32)(msix_table + 4)), 0);
	mmio_write32((u32 *)((u32)(msix_table + 8)), vector);
	mmio_write32((u32 *)((u32)(msix_table + 12)), 0);*/
	mmio_write32((u32 *)(msix_table), 0xfee00000 | cpu_id() << 12);
	mmio_write32((u32 *)((msix_table + 4)), 0);
	mmio_write32((u32 *)((msix_table + 8)), vector);
	mmio_write32((u32 *)((msix_table + 12)), 0);
	/* enable and unmask */
	ctrl &= ~MSIX_CTRL_FMASK;
	pci_write_config(bdf, cap + 2, ctrl, 2);
}

void pci_msi_set_vector(u16 bdf, unsigned int vector)
{
	int cap = pci_find_cap(bdf, PCI_CAP_MSI);
	u16 ctl, data;

	if (cap < 0)
		return;

	pci_write_config(bdf, cap + 0x04, 0xfee00000 | (cpu_id() << 12), 4);

	ctl = pci_read_config(bdf, cap + 0x02, 2);
	if (ctl & (1 << 7)) {
		pci_write_config(bdf, cap + 0x08, 0, 4);
		data = cap + 0x0c;
	} else
		data = cap + 0x08;
	pci_write_config(bdf, data, vector, 2);

	pci_write_config(bdf, cap + 0x02, 0x0001, 2);
}
