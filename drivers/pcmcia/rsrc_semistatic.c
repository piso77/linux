/*
 * rsrc_semistatic.c -- Resource management routines for SS_CAP_STATIC_MAP
 *			sockets with io_offset == 0
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * The initial developer of the original code is David A. Hinds
 * <dahinds@users.sourceforge.net>.  Portions created by David A. Hinds
 * are Copyright (C) 1999 David A. Hinds.  All Rights Reserved.
 *
 * (C) 1999		David A. Hinds
 */

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <pcmcia/ss.h>
#include <pcmcia/cistpl.h>
#include "cs_internal.h"

MODULE_AUTHOR("David A. Hinds, Dominik Brodowski");
MODULE_LICENSE("GPL");

struct pcmcia_align_data {
	unsigned long	mask;
	unsigned long	offset;
};

static resource_size_t
pcmcia_align(void *align_data, const struct resource *res,
	     resource_size_t size, resource_size_t align)
{
	struct pcmcia_align_data *data = align_data;
	resource_size_t start;

	start = (res->start & ~data->mask) + data->offset;
	if (start < res->start)
		start += data->mask + 1;

#ifdef CONFIG_M68K
        if (res->flags & IORESOURCE_IO)
		if ((start + size - 1) >= 1024)
			start = res->end;
#endif
	return start;
}

/*======================================================================

    These find ranges of I/O ports or memory addresses that are not
    currently allocated by other devices.

    The 'align' field should reflect the number of bits of address
    that need to be preserved from the initial value of *base.  It
    should be a power of two, greater than or equal to 'num'.  A value
    of 0 means that all bits of *base are significant.  *base should
    also be strictly less than 'align'.

======================================================================*/

static int semistatic_find_io_region(struct pcmcia_socket *s, unsigned int attr,
						  unsigned int *base, unsigned int num,
						  unsigned int align, struct resource **parent)
{
	struct resource *res = pcmcia_make_resource(0, num, IORESOURCE_IO,
					     dev_name(&s->dev));
	struct pcmcia_align_data data;
	unsigned long min = *base;
	int ret;

	if (align == 0)
		align = 0x10000;

	data.mask = align - 1;
	data.offset = *base & data.mask;

#ifdef CONFIG_PCI
	if (s->cb_dev) {
		ret = pci_bus_alloc_resource(s->cb_dev->bus, res, num, 1,
					     min, 0, pcmcia_align, &data);
	} else
#endif
		ret = allocate_resource(&ioport_resource, res, num, min, ~0UL,
					1, pcmcia_align, &data);

	if (ret != 0) {
		kfree(res);
		return -EINVAL;
	}
	*parent = res;
	return 0;
}

static int semistatic_init(struct pcmcia_socket *s)
{
	/* the good thing about SS_CAP_STATIC_MAP sockets is
	 * that they don't need a resource database */

	s->resource_setup_done = 1;
	return 0;
}

struct pccard_resource_ops pccard_semistatic_ops = {
	.validate_mem = NULL,
	.find_io = semistatic_find_io_region,
	.find_mem = NULL,
	.init = semistatic_init,
	.exit = NULL,
};
EXPORT_SYMBOL(pccard_semistatic_ops);
