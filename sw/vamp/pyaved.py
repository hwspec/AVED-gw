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

    def __del__(self):
        try:
            self.close()
        except Exception:
            pass
