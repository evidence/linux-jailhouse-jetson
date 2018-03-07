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

// Each PCI device has 256B (1<<8) of configuration space.
// The PCI device configuration area provided by Jailhouse is 1MiB (1<<20).
// The maximum number of PCI device configurations is 1MiB/256B,
// i.e. ((1 << 20) / (1 << 8)), which is (1 << (20 - 8))
#define PCI_CFG_DEVS_MAX (1 << 12)

int pci_find_device(u16 vendor, u16 device, u16 start_bdf)
{
	unsigned int bdf;
	u16 id;

	for (bdf = start_bdf; bdf < PCI_CFG_DEVS_MAX; bdf++) {
		id = pci_read_config(bdf, PCI_CFG_VENDOR_ID, 2);
		if (id == PCI_ID_ANY || (vendor != PCI_ID_ANY && vendor != id))
			continue;
		if (device == PCI_ID_ANY ||
		    pci_read_config(bdf, PCI_CFG_DEVICE_ID, 2) == device)
			return bdf;
	}
	return -1;
}

int pci_find_cap(u16 bdf, u16 cap)
{
	u8 pos = PCI_CFG_CAP_PTR - 1;

	if (!(pci_read_config(bdf, PCI_CFG_STATUS, 2) & PCI_STS_CAPS))
		return -1;

	while (1) {
		pos = pci_read_config(bdf, pos + 1, 1);
		if (pos == 0)
			return -1;
		if (pci_read_config(bdf, pos, 1) == cap)
			return pos;
	}
}
