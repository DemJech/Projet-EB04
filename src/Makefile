ifneq ($(KERNELRELEASE),)
        obj-m += rtdm_driver_module.o


	EXTRA_CFLAGS := -I /usr/xenomai/include/
else

	XENOCONFIG=/usr/xenomai/bin/xeno-config
	CC=$(shell      $(XENOCONFIG) --cc)
	CFLAGS=$(shell  $(XENOCONFIG) --skin=posix --cflags)
	LDFLAGS=$(shell $(XENOCONFIG) --skin=posix --ldflags)
	LIBDIR=$(shell  $(XENOCONFIG) --skin=posix --libdir)

	CROSS_COMPILE ?=
	KERNEL_DIR ?= /usr/src/linux
	MODULE_DIR := $(shell pwd)

.PHONY: all
all:: modules executable

.PHONY: modules
modules:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(MODULE_DIR) CROSS_COMPILE=$(CROSS_COMPILE) modules

.PHONY: executable
executable: nrt_prog

XENOCONFIG=/usr/xenomai/bin/xeno-config

nrt_prog: nrt_prog.c
	$(CC) -c -o nrt_prog.o nrt_prog.c $(CFLAGS) $(LDFLAGS)
	/usr/xenomai/bin/wrap-link.sh -v $(CC) -o nrt_prog nrt_prog.o $(LDFLAGS)

.PHONY: clean
clean::
	rm -f  *.o  .*.o  .*.o.* *.ko  .*.ko  *.mod.* .*.mod.* .*.cmd *~
	rm -f Module.symvers Module.markers modules.order
	rm -rf .tmp_versions
	rm -f nrt_prog
	
endif
