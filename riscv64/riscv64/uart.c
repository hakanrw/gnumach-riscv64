/*
 * NS16550 UART driver for RISC-V 64-bit
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

#include <riscv64/uart.h>
#include <device/cons.h>
#include <kern/printf.h>

/*
 * QEMU virt machine: NS16550A UART at 0x10000000
 * Register stride is 1 byte (8-bit access) on QEMU virt.
 *
 * Allwinner D1 (C906): UART0 at 0x02500000
 * Same NS16550-compatible IP, also 8-bit stride.
 *
 * Select the base address at build time with UART_BASE.  It defaults
 * to the QEMU virt address.
 */

static volatile uint8_t *uart_base = (volatile uint8_t *)UART_BASE;

/* NS16550 register offsets (byte offsets for 8-bit access) */
#define UART_RBR	0	/* Receive Buffer Register (read) */
#define UART_THR	0	/* Transmit Holding Register (write) */
#define UART_IER	1	/* Interrupt Enable Register */
#define UART_FCR	2	/* FIFO Control Register (write) */
#define UART_IIR	2	/* Interrupt Identification Register (read) */
#define UART_LCR	3	/* Line Control Register */
#define UART_MCR	4	/* Modem Control Register */
#define UART_LSR	5	/* Line Status Register */
#define UART_MSR	6	/* Modem Status Register */
#define UART_SCR	7	/* Scratch Register */

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

static inline uint8_t
uart_read(int reg)
{
	return uart_base[reg];
}

static inline void
uart_write(int reg, uint8_t val)
{
	uart_base[reg] = val;
}

/*
 * Initialize the UART hardware.
 * Called early during boot, before the console subsystem is up.
 */
void
uart_init(void)
{
	/* Disable interrupts */
	uart_write(UART_IER, 0);

	/* Enable FIFO, clear TX and RX */
	uart_write(UART_FCR, FCR_FIFO_EN | FCR_FIFO_CLR);

	/* 8N1, no DLAB */
	uart_write(UART_LCR, LCR_8N1);

	/* No modem control */
	uart_write(UART_MCR, 0);
}

/*
 * Put a character (blocking until TX FIFO has space).
 */
void
uart_putc(int c)
{
	/* Wait for transmitter holding register to be empty */
	while (!(uart_read(UART_LSR) & LSR_THRE))
		;

	/* Send the character */
	uart_write(UART_THR, (uint8_t)c);
}

/*
 * Get a character (blocking).
 * Returns -1 if no data available and wait is false.
 */
int
uart_getc(int wait)
{
	for (;;) {
		if (uart_read(UART_LSR) & LSR_DR)
			return (int)(uart_read(UART_RBR) & 0xff);
		if (!wait)
			return -1;
		/* Spin - no interrupts early in boot */
	}
}

/*
 * Console interface: probe
 * Always report as available - we know the UART is there on QEMU virt.
 */
int
uart_cnprobe(struct consdev *cp)
{
	cp->cn_dev = 0;
	cp->cn_pri = CN_REMOTE;
	return 0;
}

/*
 * Console interface: init
 */
int
uart_cninit(struct consdev *cp)
{
	uart_init();
	return 0;
}

/*
 * Console interface: putc
 */
int
uart_cnputc(dev_t dev, int c)
{
	uart_putc(c);
	return 0;
}

/*
 * Console interface: getc
 */
int
uart_cngetc(dev_t dev, int wait)
{
	return uart_getc(wait);
}

/*
 * Console device table entry.
 * Referenced from cons_conf.c.
 */
struct consdev uart_consdev = {
	"uart",
	uart_cnprobe,
	uart_cninit,
	uart_cngetc,
	uart_cnputc,
	0,
	CN_REMOTE
};

/*
 * Early putchar for printf before console is initialized.
 * Used by kern/printf.c via romputc.
 */
void
uart_early_putc(char c)
{
	uart_putc(c);
}
