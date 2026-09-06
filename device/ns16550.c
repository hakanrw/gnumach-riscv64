/*
 * NS16550 UART driver
 *
 * Copyright (C) 2025 Free Software Foundation, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if NNS16550 > 0

#include <string.h>
#include <util/atoi.h>

#include <kern/debug.h>
#include <kern/printf.h>
#include <mach/std_types.h>

#include <chips/busses.h>
#include <device/conf.h>
#include <device/cons.h>

#include <device/ns16550.h>

extern char *kernel_cmdline;

static vm_offset_t ns16550_std[NNS16550] = { 0 };
struct bus_device *ns16550_info[NNS16550];
struct bus_driver ns16550_driver = {
	ns16550_probe, 0, ns16550_attach, 0, ns16550_std,
	"nsuart", ns16550_info, 0, 0, 0};

static int rcline = 0;
static struct bus_device *ns16550_cndev;

/* NS16550 register indices. */
#define NS16550_RBR	0	/* Receive Buffer Register (read) */
#define NS16550_THR	0	/* Transmit Holding Register (write) */
#define NS16550_DLL	0	/* Divisor Latch LSB (DLAB = 1) */
#define NS16550_IER	1	/* Interrupt Enable Register */
#define NS16550_DLM	1	/* Divisor Latch MSB (DLAB = 1) */
#define NS16550_FCR	2	/* FIFO Control Register (write) */
#define NS16550_IIR	2	/* Interrupt Identification Register (read) */
#define NS16550_LCR	3	/* Line Control Register */
#define NS16550_MCR	4	/* Modem Control Register */
#define NS16550_LSR	5	/* Line Status Register */
#define NS16550_MSR	6	/* Modem Status Register */
#define NS16550_SCR	7	/* Scratch Register */

/* LSR bits */
#define LSR_DR		(1 << 0)	/* Data Ready */
#define LSR_THRE	(1 << 5)	/* Transmitter Holding Register Empty */
#define LSR_TEMT	(1 << 6)	/* Transmitter Empty */

/* LCR bits */
#define LCR_8N1		0x03		/* 8 data, no parity, 1 stop */
#define LCR_DLAB	(1 << 7)	/* Divisor Latch Access Bit */

/* FCR bits */
#define FCR_FIFO_EN	0x01		/* Enable FIFO */
#define FCR_FIFO_CLR	0x06		/* Clear TX and RX FIFOs */

/* IIR bits & masks */
#define IIR_NO_INT	(1 << 0)	/* 1 = No interrupt pending, 0 = Pending */
#define IIR_ID_MASK	0x0E		/* Interrupt ID mask (bits 1-3) */
#define IIR_FIFO_MASK	0xC0		/* FIFO status mask (bits 6-7) */
#define IIR_FIFO_NONE	0x00		/* No FIFO enabled (8250 / 16450) */
#define IIR_FIFO_BROKEN	0x80		/* FIFO enabled but broken (unusable 16550) */
#define IIR_FIFO_16550A	0xC0		/* FIFOs fully enabled and working (16550A) */

static inline vm_offset_t
ns16550_reg_address(const struct bus_device *dev, unsigned int reg)
{
	const struct ns16550_config *config = NS16550_CONF(dev->sysdep);

	return dev->address + ((vm_offset_t)reg << config->reg_shift);
}

static uint8_t
ns16550_reg_read(const struct bus_device *dev, unsigned int reg)
{
	const struct ns16550_config *config = NS16550_CONF(dev->sysdep);
	vm_offset_t address = ns16550_reg_address(dev, reg);

	switch (config->reg_io_width) {
	case 1:
		return *(volatile uint8_t *)address;
	case 2:
		return *(volatile uint16_t *)address & 0xff;
	case 4:
		return *(volatile uint32_t *)address & 0xff;
	default:
		panic("nsuart%d: unsupported register width %u",
		      dev->unit, config->reg_io_width);
	}

	return 0;
}

static void
ns16550_reg_write(const struct bus_device *dev, unsigned int reg, uint8_t val)
{
	const struct ns16550_config *config = NS16550_CONF(dev->sysdep);
	vm_offset_t address = ns16550_reg_address(dev, reg);

	switch (config->reg_io_width) {
	case 1:
		*(volatile uint8_t *)address = val;
		break;
	case 2:
		*(volatile uint16_t *)address = val;
		break;
	case 4:
		*(volatile uint32_t *)address = val;
		break;
	default:
		panic("nsuart%d: unsupported register width %u",
		      dev->unit, config->reg_io_width);
	}
}

static int
ns16550_config_valid(const struct bus_device *dev, int noisy)
{
	const struct ns16550_config *config = NS16550_CONF(dev->sysdep);
	uint64_t denominator, divisor;

	if (config == NULL) {
		if (noisy)
			printf("nsuart%d: no device configuration\n", dev->unit);
		return 0;
	}

	if (config->reg_shift > sizeof(vm_offset_t) * 8 - 3) {
		if (noisy)
			printf("nsuart%d: invalid register shift %u\n",
			       dev->unit, config->reg_shift);
		return 0;
	}

	if (config->reg_io_width != 1 && config->reg_io_width != 2
	    && config->reg_io_width != 4) {
		if (noisy)
			printf("nsuart%d: unsupported register width %u\n",
			       dev->unit, config->reg_io_width);
		return 0;
	}

	if (((vm_offset_t)1 << config->reg_shift)
	    < config->reg_io_width) {
		if (noisy)
			printf("nsuart%d: register width exceeds spacing\n",
			       dev->unit);
		return 0;
	}

	if (dev->address & (config->reg_io_width - 1)) {
		if (noisy)
			printf("nsuart%d: unaligned register address %zx\n",
			       dev->unit, dev->address);
		return 0;
	}

	if ((config->clock_frequency == 0) != (config->baud_rate == 0)) {
		if (noisy)
			printf("nsuart%d: incomplete baud rate configuration\n",
			       dev->unit);
		return 0;
	}

	if (config->baud_rate != 0) {
		denominator = 16ULL * config->baud_rate;
		divisor = (config->clock_frequency + denominator / 2)
			  / denominator;
		if (divisor == 0 || divisor > 0xffff) {
			if (noisy)
				printf("nsuart%d: invalid baud rate %u for clock %lu\n",
				       dev->unit, config->baud_rate,
				       config->clock_frequency);
			return 0;
		}
	}

	return 1;
}

static int
ns16550_probe_general(struct bus_device *dev, int noisy)
{
	uint8_t saved;
	int present = 0;
	int unit = dev->unit;

	if (unit < 0 || unit >= NNS16550) {
		if (noisy)
			printf("nsuart%d: unit out of range\n", unit);
		return 0;
	}

	if (dev->address == 0) {
		if (noisy)
			printf("nsuart%d: no register address\n", unit);
		return 0;
	}

	if (!ns16550_config_valid(dev, noisy))
		return 0;

	saved = ns16550_reg_read(dev, NS16550_SCR);

	ns16550_reg_write(dev, NS16550_SCR, 0x55);
	if (ns16550_reg_read(dev, NS16550_SCR) != 0x55)
		goto out;

	ns16550_reg_write(dev, NS16550_SCR, 0xaa);
	if (ns16550_reg_read(dev, NS16550_SCR) != 0xaa)
		goto out;

	present = 1;

out:
	ns16550_reg_write(dev, NS16550_SCR, saved);

	if (!present && noisy)
		printf("nsuart%d: probe failed\n", unit);

	return present;
}

int
ns16550_probe(vm_offset_t port, struct bus_ctlr *dev)
{
	(void) port;

	return ns16550_probe_general((struct bus_device *)dev, /*noisy*/ 0);
}

void
ns16550_attach(struct bus_device *dev)
{
	const struct ns16550_config *config = NS16550_CONF(dev->sysdep);
	uint64_t denominator, divisor;
	uint8_t lcr;
	u_char	unit = dev->unit;

	if (unit >= NNS16550) {
		printf(", disabled by NNS16550 configuration\n");
		return;
	}

	/* Select the ordinary register bank and disable interrupts. */
	lcr = ns16550_reg_read(dev, NS16550_LCR);
	ns16550_reg_write(dev, NS16550_LCR, lcr & ~LCR_DLAB);
	ns16550_reg_write(dev, NS16550_IER, 0);

	if (config->baud_rate != 0) {
		denominator = 16ULL * config->baud_rate;
		divisor = (config->clock_frequency + denominator / 2)
			  / denominator;

		ns16550_reg_write(dev, NS16550_LCR, LCR_8N1 | LCR_DLAB);
		ns16550_reg_write(dev, NS16550_DLL, divisor & 0xff);
		ns16550_reg_write(dev, NS16550_DLM, divisor >> 8);
	}

	/* Enable FIFO, clear TX and RX */
	ns16550_reg_write(dev, NS16550_FCR,
			  FCR_FIFO_EN | FCR_FIFO_CLR);

	/* 8N1, no DLAB */
	ns16550_reg_write(dev, NS16550_LCR, LCR_8N1);

	/* No modem control */
	ns16550_reg_write(dev, NS16550_MCR, 0);
}

void
ns16550_intr(int unit)
{
	(void) unit;

	/* TODO: implement */
}

static boolean_t
ns16550_device_is_valid(dev_t dev)
{
	unsigned int unit = minor(dev);

	return unit < NNS16550 && ns16550_info[unit] != NULL
	       && ns16550_info[unit]->alive;
}

io_return_t
ns16550_open(dev_t dev, int flag, io_req_t ior)
{
	(void) flag;
	(void) ior;

	if (!ns16550_device_is_valid(dev))
		return D_NO_SUCH_DEVICE;

	return D_SUCCESS;
}

void
ns16550_close(dev_t dev, int flag)
{
	(void) dev;
	(void) flag;

	/* TODO: implement */
}

io_return_t
ns16550_read(dev_t dev, io_req_t ior)
{
	(void) ior;

	if (!ns16550_device_is_valid(dev))
		return D_NO_SUCH_DEVICE;

	/* TODO: implement */
	return D_INVALID_OPERATION;
}

io_return_t
ns16550_write(dev_t dev, io_req_t ior)
{
	(void) ior;

	if (!ns16550_device_is_valid(dev))
		return D_NO_SUCH_DEVICE;

	/* TODO: implement */
	return D_INVALID_OPERATION;
}

io_return_t
ns16550_getstat(dev_t dev, dev_flavor_t flavor, dev_status_t data,
		mach_msg_type_number_t *count)
{
	(void) flavor;
	(void) data;
	(void) count;

	if (!ns16550_device_is_valid(dev))
		return D_NO_SUCH_DEVICE;

	/* TODO: implement */
	return D_INVALID_OPERATION;
}

io_return_t
ns16550_setstat(dev_t dev, dev_flavor_t flavor, dev_status_t data,
		mach_msg_type_number_t count)
{
	(void) flavor;
	(void) data;
	(void) count;

	if (!ns16550_device_is_valid(dev))
		return D_NO_SUCH_DEVICE;

	/* TODO: implement */
	return D_INVALID_OPERATION;
}

io_return_t
ns16550_portdeath(dev_t dev, mach_port_t port)
{
	(void) port;

	if (!ns16550_device_is_valid(dev))
		return D_NO_SUCH_DEVICE;

	return D_SUCCESS;
}

int
ns16550_cnprobe(struct consdev *cp)
{
	struct	bus_device *b;
	int	maj, unit, pri;

#define CONSOLE_PARAMETER " console=nsuart"
	u_char *console = (u_char *) strstr(kernel_cmdline, CONSOLE_PARAMETER);

	if (console)
		mach_atoi(console + strlen(CONSOLE_PARAMETER), &rcline);

	if (strncmp(kernel_cmdline, CONSOLE_PARAMETER + 1,
		    strlen(CONSOLE_PARAMETER) - 1) == 0)
            mach_atoi((u_char*)kernel_cmdline + strlen(CONSOLE_PARAMETER) - 1,
			  &rcline);

	maj = 0;
	unit = -1;
	pri = CN_DEAD;

	for (b = bus_device_init; b->driver; b++)
		if (strcmp(b->name, "nsuart") == 0
		    && b->unit == rcline
		    && ns16550_probe_general(b, /*quiet*/ 0))
		{
			/* Found one */
			ns16550_cndev = b;
			unit = b->unit;
			pri = CN_REMOTE;
			break;
		}

	cp->cn_dev = makedev(maj, unit);
	cp->cn_pri = pri;

	return 0;
}

/*
 * Attach/init routine for console.  This isn't called by
 * configure_bus_device which sets the alive, adaptor, and minfo
 * fields of the bus_device struct (comattach is), therefore we do
 * that by hand.
 */
int
ns16550_cninit(struct consdev *cp)
{
	ns16550_cndev->alive = 1;
	ns16550_cndev->adaptor = 0;
	ns16550_info[minor(cp->cn_dev)] = ns16550_cndev;

	ns16550_attach(ns16550_cndev);

	return 0;
}

int
ns16550_cnputc(dev_t dev, int c)
{
	struct bus_device *device = ns16550_info[minor(dev)];

	/* Wait for transmitter holding register to be empty */
	while (!(ns16550_reg_read(device, NS16550_LSR) & LSR_THRE))
		;

	/* Send the character */
	ns16550_reg_write(device, NS16550_THR, (uint8_t)c);
	return 0;
}

int
ns16550_cngetc(dev_t dev, int wait)
{
	struct bus_device *device = ns16550_info[minor(dev)];

	for (;;) {
		if (ns16550_reg_read(device, NS16550_LSR) & LSR_DR)
			return (int)(ns16550_reg_read(device, NS16550_RBR)
				     & 0xff);
		if (!wait)
			return -1;
		/* Spin - no interrupts early in boot */
	}
}

#endif /* NNS16550 */
