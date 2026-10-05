# AVED for GarageWorks (V80, no SMBus)

Fork of [Xilinx/AVED](https://github.com/Xilinx/AVED) used as the FPGA backend for
[GarageWorks](https://github.com/hwspec/garageworks). Only `amd_v80_gen5x8_25.1` is modified.

## Changes vs upstream
- **SMBus IP removed** (`axi_smbus_rpu`, its IIC port/pins, IRQ, LPD address). AMC builds without it
  (`HAL_SMBUS_FEATURE=0` when `XPAR_SMBUS_0_BASEADDR` is absent); SMBus/BMC are simply not started.
- **`user_accel`** — RTL module reference `wrapper` on `pcie_slr0_mgmt_sc/M04_AXI`,
  100 MHz `clk_pl`, **BAR0 + 0x1100000 (1 MB)** — the default `base` of GarageWorks' `AVED_Bridge`.
  Default content is GarageWorks' `Axi4Lite32Cmd` example.
- `sw/vamp`: VAMP (V80 AMI Minimal Primitives): `libvamp.so` C API, header-only C++ `vamp.hpp`, `pyaved.py`.
- `sw/AMI/api`: also builds `libami.so`.

## Flow
```bash
# 1. build (Vivado/Vitis 2025.1 + bootgen in PATH)
make hw
# 2. program + rescan PCIe (or flash via ami_tool), then
make sw
eval $(make -s env)           # LD_LIBRARY_PATH, PYTHONPATH, PARAMFN
# 3. run converted cocotb tests (conv_cocotb_to_fpga ... -> fpga_tb_*.py)
```

## garageworks top module
Top module `wrapper` (`wrapper.v`) with ports `s_axi_aclk`, `s_axi_aresetn`, `S_AXI_*`
(32-bit AXI4-Lite) — exactly what GarageWorks' `genAxiWrapper` emits. Only addr[19:0] is meaningful.
