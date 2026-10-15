// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Kazutomo Yoshii
/*
 * vamp.c - VAMP: V80 AMI Minimal Primitives
 */
#include "vamp.h"

#include <stdlib.h>
#include <unistd.h>

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
	return h && (buf || !len) && !(addr & 3) && !(len & 3) &&
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

/* AXI DMA (PG021) register offsets, relative to VAMP_DMA_BAR_OFFSET. */
#define DMA_MM2S_DMACR   0x00   /* MM2S control: bit0=RS (run/stop) */
#define DMA_MM2S_DMASR   0x04   /* MM2S status: bit1=idle when done */
#define DMA_MM2S_SA      0x18   /* MM2S source addr (64-bit: low @ 0x18, high @ 0x1c) */
#define DMA_MM2S_LENGTH  0x28   /* MM2S length (bytes); write starts transfer */
#define DMA_S2MM_DMACR   0x30   /* S2MM control */
#define DMA_S2MM_DMASR   0x34   /* S2MM status: bit1=idle when done */
#define DMA_S2MM_DA      0x48   /* S2MM dest addr (64-bit: low @ 0x48, high @ 0x4c) */
#define DMA_S2MM_LENGTH  0x58   /* S2MM length (bytes); write starts transfer */
#define DMA_IDLE_MASK    0x02   /* status bit: channel idle when set */

static int dma_write(vamp_dev *h, uint32_t reg, uint32_t val)
{
	return vamp_write32(h, VAMP_DMA_BAR_OFFSET + reg, val);
}

static int dma_read(vamp_dev *h, uint32_t reg, uint32_t *val)
{
	return vamp_read32(h, VAMP_DMA_BAR_OFFSET + reg, val);
}

static int dma_write_addr64(vamp_dev *h, uint32_t reg, uint64_t addr)
{
	if (dma_write(h, reg, (uint32_t)(addr & 0xFFFFFFFFU)) != VAMP_OK)
		return VAMP_ERR;
	return dma_write(h, reg + 4, (uint32_t)(addr >> 32));
}

static int dma_wait(vamp_dev *h, uint32_t dmasr_reg)
{
	uint32_t sr;

	if (!h)
		return VAMP_ERR;

	do {
		if (dma_read(h, dmasr_reg, &sr) != VAMP_OK)
			return VAMP_ERR;
		if (sr & DMA_IDLE_MASK)
			return VAMP_OK;
		usleep(10);
	} while (1);
}

int vamp_dma_init(vamp_dev *h)
{
	uint32_t sr;

	if (!h)
		return VAMP_ERR;

	/* Clear any sticky status bits (including errors) by write-back. */
	if (dma_read(h, DMA_MM2S_DMASR, &sr) != VAMP_OK)
		return VAMP_ERR;
	if (dma_write(h, DMA_MM2S_DMASR, sr) != VAMP_OK)
		return VAMP_ERR;

	if (dma_read(h, DMA_S2MM_DMASR, &sr) != VAMP_OK)
		return VAMP_ERR;
	if (dma_write(h, DMA_S2MM_DMASR, sr) != VAMP_OK)
		return VAMP_ERR;

	/* Enable both channels in simple mode: set RS=1. */
	if (dma_write(h, DMA_MM2S_DMACR, 0x00000001U) != VAMP_OK)
		return VAMP_ERR;
	return dma_write(h, DMA_S2MM_DMACR, 0x00000001U);
}

int vamp_dma_start_mm2s(vamp_dev *h, uint64_t src, uint32_t len)
{
	if (!h || len == 0)
		return VAMP_ERR;

	if (dma_write_addr64(h, DMA_MM2S_SA, src) != VAMP_OK)
		return VAMP_ERR;
	/* Writing LENGTH starts the transfer in simple DMA mode. */
	return dma_write(h, DMA_MM2S_LENGTH, len);
}

int vamp_dma_start_s2mm(vamp_dev *h, uint64_t dst, uint32_t len)
{
	if (!h || len == 0)
		return VAMP_ERR;

	if (dma_write_addr64(h, DMA_S2MM_DA, dst) != VAMP_OK)
		return VAMP_ERR;
	return dma_write(h, DMA_S2MM_LENGTH, len);
}

int vamp_dma_wait_mm2s(vamp_dev *h)
{
	return dma_wait(h, DMA_MM2S_DMASR);
}

int vamp_dma_wait_s2mm(vamp_dev *h)
{
	return dma_wait(h, DMA_S2MM_DMASR);
}

int vamp_dma_copy(vamp_dev *h, uint64_t src, uint64_t dst, uint32_t len)
{
	if (!h || len == 0)
		return VAMP_ERR;

	if (vamp_dma_start_mm2s(h, src, len) != VAMP_OK)
		return VAMP_ERR;
	return vamp_dma_start_s2mm(h, dst, len);
}

const char *vamp_last_error(void)
{
	const char *e = ami_get_last_error();
	return e ? e : "";
}
