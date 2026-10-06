# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Kazutomo Yoshii
# GarageWorks-compatible AVED (V80, Vivado 2025.1, no SMBus IP)
HW_DIR  := hw/amd_v80_gen5x8_25.1

help:
	@echo "make hw      - Vivado + AMC FW + PDI (hw/.../build_all.sh)"
	@echo "make prog    - JTAG-program the V80 (no-FPT PDI)"
	@echo "make sw      - build libami + libvamp/pyaved"
	@echo "make timing  - timing report from the routed checkpoint"
	@echo "make uuid    - show logic UUID from the last hw build"
	@echo "make env     - print env exports for garageworks FPGA runs"

hw:
	cd $(HW_DIR) && ./build_all.sh

prog:
	vivado -mode batch -nojournal -nolog -source program_v80.tcl

sw:
	$(MAKE) -C sw/AMI/api
	$(MAKE) -C sw/vamp

timing:
	vivado -mode batch -nojournal -nolog -source timing_v80.tcl

uuid:
	@grep -i logic-uuid $(HW_DIR)/build/vivado.log

env:
	@echo "export LD_LIBRARY_PATH=$(CURDIR)/sw/vamp/build:$(CURDIR)/sw/AMI/api/build:\$$LD_LIBRARY_PATH"
	@echo "export PYTHONPATH=$(CURDIR)/sw/vamp/build:\$$PYTHONPATH"
	@echo "export PARAMFN=$(CURDIR)/$$(ls $(HW_DIR)/src/rtl/garageworks/*.json | head -1)"

.PHONY: help hw prog sw timing uuid env
