# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Kazutomo Yoshii
# pyaved - ctypes binding for libvamp.so (API used by garageworks' AVED_Bridge)
import ctypes as C


class AVED:
    def __init__(self, libpath="libvamp.so"):
        L = self.lib = C.CDLL(libpath)
        L.vamp_open.argtypes = [C.c_char_p, C.POINTER(C.c_void_p)]
        L.vamp_close.argtypes = [C.POINTER(C.c_void_p)]
        L.vamp_close.restype = None
        L.vamp_read32.argtypes = [C.c_void_p, C.c_uint64, C.POINTER(C.c_uint32)]
        L.vamp_write32.argtypes = [C.c_void_p, C.c_uint64, C.c_uint32]
        L.vamp_mem_read.argtypes = [C.c_void_p, C.c_uint64, C.c_void_p, C.c_uint64]
        L.vamp_mem_write.argtypes = [C.c_void_p, C.c_uint64, C.c_void_p, C.c_uint64]
        L.vamp_dma_init.argtypes = [C.c_void_p]
        L.vamp_dma_start_mm2s.argtypes = [C.c_void_p, C.c_uint64, C.c_uint32]
        L.vamp_dma_start_s2mm.argtypes = [C.c_void_p, C.c_uint64, C.c_uint32]
        L.vamp_dma_wait_mm2s.argtypes = [C.c_void_p]
        L.vamp_dma_wait_s2mm.argtypes = [C.c_void_p]
        L.vamp_dma_copy.argtypes = [C.c_void_p, C.c_uint64, C.c_uint64, C.c_uint32]
        L.vamp_last_error.restype = C.c_char_p
        self.h = C.c_void_p()

    def _err(self, what):
        return RuntimeError(f"{what}: {self.lib.vamp_last_error().decode()}")

    def open(self, bdf: str):
        if self.lib.vamp_open(bdf.encode(), C.byref(self.h)) != 0:
            raise self._err(f"vamp_open({bdf})")

    def close(self):
        if self.h:
            self.lib.vamp_close(C.byref(self.h))

    def read32(self, offset: int) -> int:
        v = C.c_uint32()
        if self.lib.vamp_read32(self.h, offset, C.byref(v)) != 0:
            raise self._err(f"read32(0x{offset:x})")
        return v.value

    def write32(self, offset: int, value: int):
        if self.lib.vamp_write32(self.h, offset, value & 0xFFFFFFFF) != 0:
            raise self._err(f"write32(0x{offset:x})")

    # card DDR (addr relative to the DDR window, 4-byte aligned)
    def mem_read(self, addr: int, nbytes: int) -> bytes:
        buf = C.create_string_buffer(nbytes)
        if self.lib.vamp_mem_read(self.h, addr, buf, nbytes) != 0:
            raise self._err(f"mem_read(0x{addr:x}, {nbytes})")
        return buf.raw

    def mem_write(self, addr: int, data):
        data = bytes(data)
        if self.lib.vamp_mem_write(self.h, addr, data, len(data)) != 0:
            raise self._err(f"mem_write(0x{addr:x}, {len(data)})")

    # PL DMA engine. Call dma_init() once before the first transfer.
    def dma_init(self):
        if self.lib.vamp_dma_init(self.h) != 0:
            raise self._err("dma_init")

    def dma_start_mm2s(self, src: int, length: int):
        if self.lib.vamp_dma_start_mm2s(self.h, src, length) != 0:
            raise self._err(f"dma_start_mm2s(0x{src:x}, {length})")

    def dma_start_s2mm(self, dst: int, length: int):
        if self.lib.vamp_dma_start_s2mm(self.h, dst, length) != 0:
            raise self._err(f"dma_start_s2mm(0x{dst:x}, {length})")

    def dma_wait_mm2s(self):
        if self.lib.vamp_dma_wait_mm2s(self.h) != 0:
            raise self._err("dma_wait_mm2s")

    def dma_wait_s2mm(self):
        if self.lib.vamp_dma_wait_s2mm(self.h) != 0:
            raise self._err("dma_wait_s2mm")

    # Starts MM2S and S2MM independently (see vamp_dma_copy); does not wait.
    def dma_copy(self, src: int, dst: int, length: int):
        if self.lib.vamp_dma_copy(self.h, src, dst, length) != 0:
            raise self._err(f"dma_copy(0x{src:x}, 0x{dst:x}, {length})")

    def __del__(self):
        try:
            self.close()
        except Exception:
            pass
