/*
 * NS16550 UART driver
 *
 * Copyright (C) 2025 Free Software Foundation, Inc.
 */

#ifndef _NS16550_H_
#define _NS16550_H_

#include <mach/machine/vm_types.h>
#include <chips/busses.h>
#include <device/conf.h>
#include <device/cons.h>

#define NS16550_CONF(conf) ((const struct ns16550_config *)conf)

struct ns16550_config {
	unsigned int	reg_shift;
	unsigned int	reg_io_width;
	unsigned long	clock_frequency;
	unsigned int	baud_rate;
	unsigned int	irq;
};

extern int  ns16550_probe(vm_offset_t port, struct bus_ctlr *dev);
extern void ns16550_attach(struct bus_device *dev);
extern void ns16550_intr(int unit);

extern io_return_t ns16550_open(dev_t dev, int flag, io_req_t ior);
extern void ns16550_close(dev_t dev, int flag);
extern io_return_t ns16550_read(dev_t dev, io_req_t ior);
extern io_return_t ns16550_write(dev_t dev, io_req_t ior);
extern io_return_t ns16550_getstat(dev_t dev, dev_flavor_t flavor,
				  dev_status_t data,
				  mach_msg_type_number_t *count);
extern io_return_t ns16550_setstat(dev_t dev, dev_flavor_t flavor,
				  dev_status_t data,
				  mach_msg_type_number_t count);
extern io_return_t ns16550_portdeath(dev_t dev, mach_port_t port);

extern int  ns16550_cnprobe(struct consdev *cp);
extern int  ns16550_cninit(struct consdev *cp);
extern int  ns16550_cnputc(dev_t dev, int c);
extern int  ns16550_cngetc(dev_t dev, int wait);

#endif /* _NS16550_H_ */
