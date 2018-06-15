/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) ARM Limited, 2014
 *
 * Authors:
 *  Jean-Philippe Brucker <jean-philippe.brucker@arm.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 * Alternatively, you can use or redistribute this file under the following
 * BSD license:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _JAILHOUSE_INMATE_H
#define _JAILHOUSE_INMATE_H

#define COMM_REGION_BASE	0x80000000
#define HEAP_BASE 0x0

#define PAGE_SIZE		(4 * 1024UL)
#define PAGE_MASK		(~(PAGE_SIZE - 1))

#define PCI_CFG_VENDOR_ID	0x000
#define PCI_CFG_DEVICE_ID	0x002
#define PCI_CFG_COMMAND		0x004
#define PCI_CMD_IO		(1 << 0)
#define PCI_CMD_MEM		(1 << 1)
#define PCI_CMD_MASTER		(1 << 2)
#define PCI_CMD_INTX_OFF	(1 << 10)
#define PCI_CFG_STATUS		0x006
#define PCI_STS_INT		(1 << 3)
#define PCI_STS_CAPS		(1 << 4)
#define PCI_CFG_BAR		0x010
#define PCI_BAR_64BIT		0x4
#define PCI_CFG_CAP_PTR		0x034

#define PCI_ID_ANY		0xffff

#define PCI_DEV_CLASS_OTHER	0xff

#define PCI_CAP_MSI		0x05
#define PCI_CAP_MSIX		0x11

#define MSIX_CTRL_ENABLE	0x8000
#define MSIX_CTRL_FMASK		0x4000

typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;

static inline u8 mmio_read8(void *address)
{
	return *(volatile u8 *)address;
}

static inline void mmio_write8(void *address, u8 value)
{
	*(volatile u8 *)address = value;
}

static inline u16 mmio_read16(void *address)
{
	return *(volatile u16 *)address;
}

static inline void mmio_write16(void *address, u16 value)
{
	*(volatile u16 *)address = value;
}

static inline u32 mmio_read32(void *address)
{
	return *(volatile u32 *)address;
}

static inline void mmio_write32(void *address, u32 value)
{
	*(volatile u32 *)address = value;
}

static inline void __attribute__((noreturn)) halt(void)
{
	while (1)
		asm volatile("wfi" : : : "memory");
}

static inline unsigned int cpu_id(void) {
       // FIXME: We should get the real CPI ID (from cp15?)!!!
       // x86 does this: return read_msr(X2APIC_ID);
       return 1;
}

typedef void (*irq_handler_t)(unsigned int);
void gic_setup(irq_handler_t handler);
void gic_enable_irq(unsigned int irq);

unsigned long timer_get_frequency(void);
u64 timer_get_ticks(void);
u64 timer_ticks_to_ns(u64 ticks);
void timer_start(u64 timeout);

u32 pci_read_config(u16 bdf, unsigned int addr, unsigned int size);
void pci_write_config(u16 bdf, unsigned int addr, u32 value,
                     unsigned int size);
int pci_find_device(u16 vendor, u16 device, u16 start_bdf);
int pci_find_cap(u16 bdf, u16 cap);
void pci_msi_set_vector(u16 bdf, unsigned int vector);
void pci_msix_set_vector(u16 bdf, unsigned int vector, u32 index);

void arch_mmu_enable(void);

#include <asm/processor.h>
#include <arch/inmate.h>

#include "../inmate_common.h"

/*
 * To ease the debugging, we can send a spurious hypercall, which should return
 * -ENOSYS, but appear in the hypervisor stats for this cell.
 */
static inline void heartbeat(void)
{
#ifndef CONFIG_BARE_METAL
	jailhouse_call(0xdeadbeef);
#endif
}

#endif /* !_JAILHOUSE_INMATE_H */
