// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Kazutomo Yoshii
/*
 * vamp.h - VAMP: V80 AMI Minimal Primitives (C API)
 *
 * Scope: open/close a device, 32-bit BAR register access, and bulk copies to
 * card DDR through a BAR window (host-CPU MMIO copy, not real DMA).
 * The handle is opaque so new primitives (e.g. vamp_dma_*) can be added
 * later without changing this ABI.
 */
#ifndef VAMP_H
#define VAMP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vamp_dev vamp_dev;

/* Return codes: 0 on success, negative on error. */
#define VAMP_OK    0
#define VAMP_ERR  -1

/* Open the device at PCI BDF (e.g. "b1:00.0"). Register access uses BAR0. */
int  vamp_open(const char *bdf, vamp_dev **h);
void vamp_close(vamp_dev **h);

int  vamp_read32(vamp_dev *h, uint64_t offset, uint32_t *val);
int  vamp_write32(vamp_dev *h, uint64_t offset, uint32_t val);

/* Card DDR through BAR0 (DDR LOW0, see create_bd_design.tcl).
 * addr is relative to the window; addr and len must be 4-byte aligned.
 * len == 0 is a no-op. */
#define VAMP_MEM_BAR_OFFSET  0x8000000ULL
#define VAMP_MEM_SIZE        (128ULL << 20)

int  vamp_mem_read (vamp_dev *h, uint64_t addr, void *buf, uint64_t len);
int  vamp_mem_write(vamp_dev *h, uint64_t addr, const void *buf, uint64_t len);

/* PL DMA engine (Xilinx AXI DMA, simple/direct-register mode) through BAR0.
 * VAMP_DMA_BAR_OFFSET is pl_dma's S_AXI_LITE base (64 KB window, see
 * create_bd_design.tcl / BLOCK_DIAGRAM). Not touched by vamp_open(); call
 * vamp_dma_init() explicitly before starting transfers. */
#define VAMP_DMA_BAR_OFFSET  0x1200000ULL

int  vamp_dma_init(vamp_dev *h);

int  vamp_dma_start_mm2s(vamp_dev *h, uint64_t src, uint32_t len);
int  vamp_dma_start_s2mm(vamp_dev *h, uint64_t dst, uint32_t len);
int  vamp_dma_wait_mm2s(vamp_dev *h);
int  vamp_dma_wait_s2mm(vamp_dev *h);

/* Programs and starts both channels independently (pl_dma's MM2S/S2MM are
 * not looped back in this design, so this is two one-sided transfers, not
 * a true memory-to-memory copy, unless/until a loopback path exists). */
int  vamp_dma_copy(vamp_dev *h, uint64_t src, uint64_t dst, uint32_t len);

/* Last AMI error string (may be empty). */
const char *vamp_last_error(void);

#ifdef __cplusplus
}
#endif
#endif /* VAMP_H */
