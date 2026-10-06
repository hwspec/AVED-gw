# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Kazutomo Yoshii
# VAMP: libvamp.so (C), vamp.hpp (C++ header-only), pyaved.py. Requires sw/AMI/api (libami.so).
AMI_API := ../AMI/api
BUILD   := build
CFLAGS  := -I$(AMI_API)/include -Wall -Wextra -Werror -fPIC -O2
LDLIBS  := -L$(AMI_API)/build -lami -Wl,-rpath,$(abspath $(AMI_API)/build)

all: $(BUILD)/libvamp.so $(BUILD)/pyaved.py

$(AMI_API)/build/libami.so:
	$(MAKE) -C $(AMI_API)

$(BUILD)/libvamp.so: vamp.c vamp.h $(AMI_API)/build/libami.so
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -shared -o $@ vamp.c $(LDLIBS)

$(BUILD)/pyaved.py: pyaved.py
	@mkdir -p $(BUILD)
	cp $< $@

# C++ compile check of the header-only wrapper
check-cxx: $(BUILD)/libvamp.so
	echo '#include "vamp.hpp"\nint main(){return 0;}' | $(CXX) -std=c++17 -Wall -Werror -I. -I$(AMI_API)/include -x c++ - -L$(BUILD) -lvamp -o $(BUILD)/cxx_check

clean:
	rm -rf $(BUILD)

.PHONY: all check-cxx clean
