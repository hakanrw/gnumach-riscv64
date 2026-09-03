/*
 * NS16550 UART driver for RISC-V 64-bit
 *
 * Copyright (C) 2025 Free Software Foundation, Inc.
 */

#ifndef _RISCV64_UART_H_
#define _RISCV64_UART_H_

#include <mach/machine/vm_types.h>
#include <device/cons.h>

/*
 * UART base addresses for known RISC-V platforms.
 * QEMU virt machine: 0x10000000
 * Allwinner D1 (C906): 0x02500000 (UART0)
 */
#define UART_BASE_QEMU_VIRT	0x10000000UL
#define UART_BASE_ALLWINNER_D1	0x02500000UL

/* Default to QEMU virt for development */
#ifndef UART_BASE
#define UART_BASE	UART_BASE_QEMU_VIRT
#endif

/* Initialize UART hardware */
extern void uart_init(void);

/* Blocking put character */
extern void uart_putc(int c);

/* Get character; returns -1 if no data and wait==0 */
extern int uart_getc(int wait);

/* Early putchar for printf before console init */
extern void uart_early_putc(char c);

/* Console interface functions (used by cons_conf.c) */
extern int  uart_cnprobe(struct consdev *cp);
extern int  uart_cninit(struct consdev *cp);
extern int  uart_cnputc(dev_t dev, int c);
extern int  uart_cngetc(dev_t dev, int wait);

#endif /* _RISCV64_UART_H_ */
