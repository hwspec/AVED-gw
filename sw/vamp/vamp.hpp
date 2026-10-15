// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Kazutomo Yoshii
/*
 * vamp.hpp - C++ wrapper for VAMP (header-only, RAII)
 *
 *   vamp::Device d("b1:00.0");
 *   d.write32(0x1100020, 42);
 *   uint32_t v = d.read32(0x1100020);
 *   std::vector<uint32_t> buf(1024);
 *   d.mem_write(0, buf); d.mem_read(0, buf);   // card DDR
 */
#ifndef VAMP_HPP
#define VAMP_HPP

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "vamp.h"

namespace vamp {

class Error : public std::runtime_error {
public:
	explicit Error(const std::string &what)
		: std::runtime_error(what + ": " + vamp_last_error()) {}
};

class Device {
public:
	explicit Device(const std::string &bdf)
	{
		if (vamp_open(bdf.c_str(), &h_) != VAMP_OK)
			throw Error("vamp_open(" + bdf + ")");
	}
	~Device() { vamp_close(&h_); }

	Device(const Device &) = delete;
	Device &operator=(const Device &) = delete;
	Device(Device &&o) noexcept : h_(std::exchange(o.h_, nullptr)) {}
	Device &operator=(Device &&o) noexcept
	{
		if (this != &o) {
			vamp_close(&h_);
			h_ = std::exchange(o.h_, nullptr);
		}
		return *this;
	}

	uint32_t read32(uint64_t offset) const
	{
		uint32_t v = 0;
		if (vamp_read32(h_, offset, &v) != VAMP_OK)
			throw Error("read32");
		return v;
	}

	void write32(uint64_t offset, uint32_t val)
	{
		if (vamp_write32(h_, offset, val) != VAMP_OK)
			throw Error("write32");
	}

	/* card DDR (addr relative to the DDR window, 4-byte aligned) */
	void mem_read(uint64_t addr, void *buf, uint64_t len) const
	{
		if (vamp_mem_read(h_, addr, buf, len) != VAMP_OK)
			throw Error("mem_read");
	}

	void mem_write(uint64_t addr, const void *buf, uint64_t len)
	{
		if (vamp_mem_write(h_, addr, buf, len) != VAMP_OK)
			throw Error("mem_write");
	}

	template <typename T>
	void mem_read(uint64_t addr, std::vector<T> &v) const { mem_read(addr, v.data(), vec_bytes(v)); }

	template <typename T>
	void mem_write(uint64_t addr, const std::vector<T> &v) { mem_write(addr, v.data(), vec_bytes(v)); }

	/* PL DMA engine. Call dma_init() once before the first transfer. */
	void dma_init()
	{
		if (vamp_dma_init(h_) != VAMP_OK)
			throw Error("dma_init");
	}

	void dma_start_mm2s(uint64_t src, uint32_t len)
	{
		if (vamp_dma_start_mm2s(h_, src, len) != VAMP_OK)
			throw Error("dma_start_mm2s");
	}

	void dma_start_s2mm(uint64_t dst, uint32_t len)
	{
		if (vamp_dma_start_s2mm(h_, dst, len) != VAMP_OK)
			throw Error("dma_start_s2mm");
	}

	void dma_wait_mm2s()
	{
		if (vamp_dma_wait_mm2s(h_) != VAMP_OK)
			throw Error("dma_wait_mm2s");
	}

	void dma_wait_s2mm()
	{
		if (vamp_dma_wait_s2mm(h_) != VAMP_OK)
			throw Error("dma_wait_s2mm");
	}

	/* Starts MM2S and S2MM independently (see vamp_dma_copy); does not wait. */
	void dma_copy(uint64_t src, uint64_t dst, uint32_t len)
	{
		if (vamp_dma_copy(h_, src, dst, len) != VAMP_OK)
			throw Error("dma_copy");
	}

	vamp_dev *handle() const { return h_; }   /* escape hatch for future C APIs */

private:
	template <typename T>
	static uint64_t vec_bytes(const std::vector<T> &v)
	{
		uint64_t n = v.size() * sizeof(T);
		if (n & 3)
			throw std::invalid_argument("vamp: vector size must be a multiple of 4 bytes");
		return n;
	}

	vamp_dev *h_ = nullptr;
};

} // namespace vamp
#endif /* VAMP_HPP */
