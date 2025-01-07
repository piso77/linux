/*
 * Device driver for the Amiga Gayle PCMCIA controller
 *
 * (C) Copyright 2000-2005 Kars de Jong <jongk@linux-m68k.org>
 * (C) Copyright 2025 Paolo Pisati <p.pisati@gmail.com>
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <pcmcia/ss.h>
#include <pcmcia/cistpl.h>
#include "cs_internal.h"

#include <asm/amigaints.h>
#include <asm/amipcmcia.h>

struct gayle_socket_info {
	struct pcmcia_socket	psocket;
	struct platform_device *pdev;

	phys_addr_t		attr;
	phys_addr_t		io;
	phys_addr_t		mem;

	u_int			csc_mask;
	u_char			reset_inten;
	u_char			intena;
	u_char			iocard:1;
	u_char			reset:1;
	u_char			Vcc:6;
	u_short			speed;
};
struct gayle_socket_info *socket;

#define to_gayle_socket(x) container_of(x, struct gayle_socket_info, psocket)

static int gayle_pcmcia_init(struct pcmcia_socket *s)
{
	return 0;
}

static int gayle_pcmcia_get_status(struct pcmcia_socket *s, u_int *value)
{
	struct gayle_socket_info *socket = to_gayle_socket(s);
	u_char status;
	u_int val = 0;

	status = gayle.cardstatus;
	if (socket->Vcc)
		val |= SS_POWERON;
	val |= (status & GAYLE_CS_CCDET) ? SS_DETECT : 0;
	if (socket->iocard) {
		val |= (status & GAYLE_CS_SC) ? SS_STSCHG : 0;
	} else {
		val |= (status & GAYLE_CS_WR) ? 0 : SS_WRPROT;
		val |= (status & GAYLE_CS_BSY) ? 0 : SS_READY;
		val |= (status & GAYLE_CS_BVD1) ? SS_BATDEAD : 0;
		val |= (status & GAYLE_CS_BVD2) ? SS_BATWARN : 0;
	}

	*value = val;
	return 0;
}

#if 0
static int gayle_pcmcia_get_socket(struct pcmcia_socket *s, socket_state_t *state)
{
	u_char reg, vpp;

	state->flags = SS_PWR_AUTO;	/* No power management features */
	state->Vcc = socket.Vcc;	/* Only 5V cards */
	state->Vpp = 0;
	reg = gayle.config;
	vpp = reg & GAYLE_VPP_MASK;
	if (vpp == GAYLE_CFG_5V)
		state->Vpp = 50;
	if (vpp == GAYLE_CFG_12V)
		state->Vpp = 120;

	/* IO card, IO interrupt */
	reg = gayle.cardstatus;
	if (socket.iocard) {
		state->flags |= SS_IOCARD;
		if (reg & GAYLE_CS_WR)   state->flags |= SS_OUTPUT_ENA;
		if (reg & GAYLE_CS_DA) state->flags |= SS_SPKR_ENA;
		state->io_irq = socket.psocket.pci_irq;
	}

	if (socket.reset) state->flags |= SS_RESET;

	/* Card status interrupt change mask */
	state->csc_mask = socket.csc_mask;

	return 0;
}
#endif

static int gayle_pcmcia_set_socket(struct pcmcia_socket *s, socket_state_t *state)
{
	struct gayle_socket_info *socket = to_gayle_socket(s);
	u_char oldreg, reg;
	u_int changed;

	pr_err("%s::%d cardstatus: 0x%x\n", __func__, __LINE__, gayle.cardstatus);
	pr_err("%s::%d intreq: 0x%x\n", __func__, __LINE__,gayle.intreq);
	pr_err("%s::%d inten: 0x%x\n", __func__, __LINE__, gayle.inten);
	pr_err("%s::%d config: 0x%x\n", __func__, __LINE__, gayle.config);
	socket->iocard = (state->flags & SS_IOCARD) ? 1 : 0;
	oldreg = reg = gayle.config;
	reg &= ~GAYLE_VPP_MASK;
	switch (state->Vcc) {
	case 0:	break;
	case 50:	break;
	default:	return -EINVAL;
	}
	socket->Vcc = state->Vcc;
	switch (state->Vpp) {
	case 0:	break;
	case 50:	reg |= GAYLE_CFG_5V; break;
	case 120:	reg |= GAYLE_CFG_12V; break;
	default:	return -EINVAL;
	}

	if (reg != oldreg)
		gayle.config = reg;

	if (state->flags & SS_RESET) {
		socket->reset_inten = gayle.inten;
		gayle.inten = (socket->reset_inten & ~(GAYLE_IRQ_BVD1|GAYLE_IRQ_BVD2|GAYLE_IRQ_WR|GAYLE_IRQ_BSY));
		gayle.intreq = 0xff;
		socket->reset = 1;
	} else if (socket->reset) {
		gayle.intreq = 0xfc;
		udelay(10);
		gayle.intreq = (0xfc & ~(GAYLE_IRQ_BVD1|GAYLE_IRQ_BVD2|GAYLE_IRQ_WR|GAYLE_IRQ_BSY));
		gayle.inten = socket->reset_inten;
		socket->reset = 0;
	}

	oldreg = reg = (gayle.cardstatus & (GAYLE_CS_WR|GAYLE_CS_DA|GAYLE_CS_DAEN));

	if (socket->iocard) {
		if (state->flags & SS_OUTPUT_ENA) {
			reg |= GAYLE_CS_WR|GAYLE_CS_DAEN;
		}
		if (state->flags & SS_SPKR_ENA) {
			reg |= GAYLE_CS_DA|GAYLE_CS_DAEN;
		}
	} else {
	    reg &= ~(GAYLE_CS_WR|GAYLE_CS_DA|GAYLE_CS_DAEN);
	}

	if (reg != oldreg) {
		gayle.cardstatus = reg;
		gayle.intreq = (0xfc & ~(GAYLE_IRQ_WR|GAYLE_IRQ_DA));
	}

	/* Card status change interrupt mask */
	changed = socket->csc_mask ^ state->csc_mask;
	socket->csc_mask = state->csc_mask;
	oldreg = reg = gayle.inten;

	if (changed & SS_DETECT) {
		if (state->csc_mask & SS_DETECT)
			reg |= GAYLE_IRQ_CCDET;
		else
			reg &= ~GAYLE_IRQ_CCDET;
	}

	if (changed & SS_READY) {
		if (state->csc_mask & SS_READY)
			reg |= GAYLE_IRQ_BSY;
		else
			reg &= ~GAYLE_IRQ_BSY;
	}

	if (changed & SS_BATDEAD) {
		if (state->csc_mask & SS_BATDEAD)
			reg |= GAYLE_IRQ_BVD1;
		else
			reg &= ~GAYLE_IRQ_BVD1;
	}

	if (changed & SS_BATWARN) {
		if (state->csc_mask & SS_BATWARN)
			reg |= GAYLE_IRQ_BVD2;
		else
			reg &= ~GAYLE_IRQ_BVD2;
	}

	if (changed & SS_STSCHG) {
		if (state->csc_mask & SS_STSCHG)
			reg |= GAYLE_IRQ_SC;
		else
			reg &= ~GAYLE_IRQ_SC;
	}

	if (reg != oldreg) {
		gayle.inten = reg;
		socket->intena = (gayle.inten & (GAYLE_IRQ_CCDET|GAYLE_IRQ_BVD1|GAYLE_IRQ_BVD2|GAYLE_IRQ_WR|GAYLE_IRQ_BSY));
	}

	pr_err("%s::%d cardstatus: 0x%x\n", __func__, __LINE__, gayle.cardstatus);
	pr_err("%s::%d intreq: 0x%x\n", __func__, __LINE__,gayle.intreq);
	pr_err("%s::%d inten: 0x%x\n", __func__, __LINE__, gayle.inten);
	pr_err("%s::%d config: 0x%x\n", __func__, __LINE__, gayle.config);
	return 0;
}

static int gayle_pcmcia_set_io_map(struct pcmcia_socket *sock, struct pccard_io_map *map)
{
	if (map->map >= MAX_IO_WIN) {
		printk(KERN_ERR "%s(): map (%d) out of range\n", __FUNCTION__,
		       map->map);
		return -1;
	}

	if (map->stop == 1)
		map->stop = PAGE_SIZE-1;

	gayle_set_io_win(map->map, map->flags, map->start, map->stop);

	return 0;
}

static void gayle_pcmcia_set_speed(u_short speed) {
	u_char s;

	if (speed <= 100)
		s = GAYLE_CFG_100NS;
	else if (speed <= 150)
		s = GAYLE_CFG_150NS;
	else if (speed <= 250)
		s = GAYLE_CFG_250NS;
	else
		s = GAYLE_CFG_720NS;

	gayle.config = (gayle.config & ~GAYLE_SPEED_MASK) | s;
}

static int gayle_pcmcia_set_mem_map(struct pcmcia_socket *sock, struct pccard_mem_map *map)
{
	struct gayle_socket_info *socket = to_gayle_socket(sock);
	u_long start;

	if (map->map >= MAX_WIN)
		return -EINVAL;

	gayle_pcmcia_set_speed(map->speed);

	if (map->flags & MAP_ATTRIB) {
		start = socket->attr;
		if (map->flags & MAP_ACTIVE)
			gayle_pcmcia_set_speed(720);
	} else {
		start = socket->mem;
	}

	map->static_start = start + map->card_start;

	return 0;
}

static irqreturn_t gayle_pcmcia_interrupt(int irq, void *dev)
{
	struct gayle_socket_info *socket = dev;
	u_char sstat, ints, latch, ack = 0xfc;
	u_int events = 0;

	ints = gayle.intreq;

	sstat = gayle.cardstatus;
	latch = ints & socket->intena;

	if (latch & GAYLE_IRQ_CCDET) {
		/* Check for card removal */
		if (!(gayle.cardstatus & GAYLE_CS_CCDET)) {
			/* Better clear all ints */
			ack &= ~(GAYLE_IRQ_BVD1|GAYLE_IRQ_BVD2|GAYLE_IRQ_WR|GAYLE_IRQ_BSY);
			/* Turn off all IO interrupts */
			if (socket->iocard) {
				gayle.inten &= ~GAYLE_IRQ_IRQ;
				gayle.cardstatus = 0;
			}
			/* Don't do the rest unless a card is present */
			latch &= ~(GAYLE_IRQ_BVD1|GAYLE_IRQ_BVD2|GAYLE_IRQ_WR|GAYLE_IRQ_BSY);
		}
		ack &= ~GAYLE_IRQ_CCDET;
		events |= SS_DETECT;
	}

	if (latch & GAYLE_IRQ_BSY) {
		ack &= ~GAYLE_IRQ_BSY;
		events |= SS_READY;
	}

	if (latch & GAYLE_IRQ_BVD2) {
		ack &= ~GAYLE_IRQ_BVD2;
		events |= SS_BATWARN;
	}

	if (latch & GAYLE_IRQ_BVD1) {
		ack &= ~GAYLE_IRQ_BVD1;
		events |= SS_BATDEAD;
	}

	if (latch & GAYLE_IRQ_SC) {
		ack &= ~GAYLE_IRQ_SC;
		events |= SS_STSCHG;
	}

	gayle.intreq = ack;

	if (events)
		pcmcia_parse_events(&socket->psocket, events);

	return IRQ_HANDLED;
}

static irqreturn_t gayle_stschg_irq(int irq, void *data)
{
	unsigned char pcmcia_intreq;
	struct gayle_socket_info *socket = data;

	pcmcia_intreq = pcmcia_get_intreq();
	if (!(pcmcia_intreq & GAYLE_IRQ_IRQ))
		return IRQ_NONE;

	pr_info("%s::%d intreq: 0x%x\n", __func__, __LINE__, pcmcia_intreq);
	pcmcia_ack_int(pcmcia_get_intreq()); // ack int at gayle level to avoid an interrupt storm
	pcmcia_parse_events(&socket->psocket, SS_STSCHG);

	return IRQ_NONE;
}

static struct pccard_operations gayle_pcmcia_operations = {
	.init		= gayle_pcmcia_init,
	.get_status	= gayle_pcmcia_get_status,
	.set_socket	= gayle_pcmcia_set_socket,
	.set_io_map	= gayle_pcmcia_set_io_map,
	.set_mem_map	= gayle_pcmcia_set_mem_map,
};

static int init_gayle_pcmcia(struct platform_device *pdev)
{
	struct resource *r;
	int err;

	socket = kzalloc(sizeof(struct gayle_socket_info), GFP_KERNEL);
	if (!socket)
		return -ENOMEM;

	printk(KERN_INFO "Amiga Gayle PCMCIA found, 1 socket\n");

	r = platform_get_resource_byname(pdev, IORESOURCE_MEM, "Gayle attribute");
	socket->attr = r->start;

	r = platform_get_resource_byname(pdev, IORESOURCE_MEM, "Gayle memory");
	socket->mem = r->start;

	r = platform_get_resource_byname(pdev, IORESOURCE_MEM, "Gayle I/O");
	socket->io = r->start;


	err = request_irq(IRQ_AMIGA_EXTER, gayle_pcmcia_interrupt, IRQF_SHARED,
			       "Gayle PCMCIA status", socket);
	if (err)
		goto out2;

	err = request_irq(IRQ_AMIGA_PORTS, gayle_stschg_irq, IRQF_SHARED,
			  "pcmcia_stschg", socket);
	if (err)
		goto out1;


	printk(KERN_INFO "  status change on irq %d\n", IRQ_AMIGA_EXTER);
	socket->psocket.owner = THIS_MODULE;
	socket->psocket.dev.parent = &pdev->dev;
	socket->psocket.ops = &gayle_pcmcia_operations;
	socket->psocket.resource_ops = &pccard_iodyn_ops;
	socket->psocket.features = SS_CAP_STATIC_MAP|SS_CAP_PCCARD;
	socket->psocket.irq_mask = 0;
	socket->psocket.map_size = PAGE_SIZE;
	socket->psocket.pci_irq = IRQ_AMIGA_PORTS;
	socket->psocket.io_offset = 0;

	platform_set_drvdata(pdev, socket);

	/* put gayle in a sane state */
	gayle.config = 0;
	pcmcia_reset();
	pcmcia_program_voltage(PCMCIA_0V);
	pcmcia_access_speed(PCMCIA_SPEED_250NS);
	pcmcia_write_enable();

	socket->intena = (gayle.inten & ~GAYLE_IRQ_IDE);
	gayle.cardstatus = 0;
	gayle.intreq = (0xfc & ~(GAYLE_IRQ_BVD1|GAYLE_IRQ_BVD2|GAYLE_IRQ_WR|GAYLE_IRQ_BSY));
	socket->speed = 250;

	err = pcmcia_register_socket(&socket->psocket);
	if (err)
		goto out;

	pcmcia_enable_irq();

	return 0;
out:
	free_irq(IRQ_AMIGA_PORTS, socket);
out1:
	free_irq(IRQ_AMIGA_EXTER, socket);
out2:
	kfree(socket);
	return err;
}

static int exit_gayle_pcmcia(struct platform_device *pdev)
{
	gayle.inten &= ~(GAYLE_IRQ_BVD1|GAYLE_IRQ_BVD2|GAYLE_IRQ_WR|GAYLE_IRQ_BSY);
	free_irq(IRQ_AMIGA_EXTER, socket);
	free_irq(IRQ_AMIGA_PORTS, socket);
	pcmcia_unregister_socket(&socket->psocket);
	kfree(socket);

	return 0;
}

static struct platform_driver gayle_pcmcia_driver = {
	.driver = {
		.name	= "amiga-gayle-pcmcia",
	},
	.probe		= init_gayle_pcmcia,
	.remove		= exit_gayle_pcmcia,
};
module_platform_driver(gayle_pcmcia_driver);

MODULE_AUTHOR("Kars de Jong <jongk@linux-m68k.org>");
MODULE_AUTHOR("Paolo Pisati <p.pisati@gmail.com>");
MODULE_DESCRIPTION("Amiga Gayle PCMCIA controller");
MODULE_LICENSE("GPL v2");
