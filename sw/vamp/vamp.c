// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Kazutomo Yoshii
/*
 * vamp.c - VAMP: V80 AMI Minimal Primitives
 */
#include "vamp.h"

#include <stdlib.h>

#include "ami.h"
#include "ami_device.h"
#include "ami_mem_access.h"

struct vamp_dev {
	ami_device *dev;
	uint8_t     bar;       /* BAR used for register access */
	uint64_t    mem_base;  /* BAR offset of the DDR window */
	uint64_t    mem_size;
	/* future: DMA engine state, ... */
};

int vamp_open(const char *bdf, vamp_dev **h)
{
	vamp_dev *p;

	if (!bdf || !h)
		return VAMP_ERR;

	p = calloc(1, sizeof(*p));
	if (!p)
		return VAMP_ERR;

	/* ami_dev_find, not ami_dev_bringup: BAR access needs no sensor/AMC setup */
	if (ami_dev_find(bdf, &p->dev) != AMI_STATUS_OK)
		goto fail;
	if (ami_dev_request_access(p->dev) != AMI_STATUS_OK)
		goto fail;

	p->bar      = 0;
	p->mem_base = VAMP_MEM_BAR_OFFSET;
	p->mem_size = VAMP_MEM_SIZE;
	*h = p;
	return VAMP_OK;

fail:
	if (p->dev)
		ami_dev_delete(&p->dev);
	free(p);
	return VAMP_ERR;
}

void vamp_close(vamp_dev **h)
{
	if (!h || !*h)
		return;
	ami_dev_delete(&(*h)->dev);
	free(*h);
	*h = NULL;
}

int vamp_read32(vamp_dev *h, uint64_t offset, uint32_t *val)
{
	if (!h || !val)
		return VAMP_ERR;
	return ami_mem_bar_read(h->dev, h->bar, offset, val) == AMI_STATUS_OK
		? VAMP_OK : VAMP_ERR;
}

int vamp_write32(vamp_dev *h, uint64_t offset, uint32_t val)
{
	if (!h)
		return VAMP_ERR;
	return ami_mem_bar_write(h->dev, h->bar, offset, val) == AMI_STATUS_OK
		? VAMP_OK : VAMP_ERR;
}

/* AMI's driver vzalloc()s a kernel bounce buffer per call; keep calls bounded. */
#define VAMP_MEM_CHUNK  (1U << 20)

static int mem_check(const vamp_dev *h, uint64_t addr, const void *buf, uint64_t len)
{
	return h && buf && len && !(addr & 3) && !(len & 3) &&
	       addr <= h->mem_size && len <= h->mem_size - addr;
}

int vamp_mem_read(vamp_dev *h, uint64_t addr, void *buf, uint64_t len)
{
	uint8_t *p = buf;

	if (!mem_check(h, addr, buf, len))
		return VAMP_ERR;
	while (len) {
		uint32_t n = len > VAMP_MEM_CHUNK ? VAMP_MEM_CHUNK : (uint32_t)len;
		if (ami_mem_bar_read_range(h->dev, h->bar, h->mem_base + addr,
					   n / 4, (uint32_t *)p) != AMI_STATUS_OK)
			return VAMP_ERR;
		addr += n; p += n; len -= n;
	}
	return VAMP_OK;
}

int vamp_mem_write(vamp_dev *h, uint64_t addr, const void *buf, uint64_t len)
{
	const uint8_t *p = buf;

	if (!mem_check(h, addr, buf, len))
		return VAMP_ERR;
	while (len) {
		uint32_t n = len > VAMP_MEM_CHUNK ? VAMP_MEM_CHUNK : (uint32_t)len;
		/* AMI takes a non-const pointer but only reads from it */
		if (ami_mem_bar_write_range(h->dev, h->bar, h->mem_base + addr,
					    n / 4, (uint32_t *)(uintptr_t)p) != AMI_STATUS_OK)
			return VAMP_ERR;
		addr += n; p += n; len -= n;
	}
	return VAMP_OK;
}

const char *vamp_last_error(void)
{
	const char *e = ami_get_last_error();
	return e ? e : "";
}
