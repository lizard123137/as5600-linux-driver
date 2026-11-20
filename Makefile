KERNELDIR ?= /lib/modules/$(shell uname -r)/build
SRCDIR := $(shell pwd)/src
OBJDIR := $(shell pwd)/obj

default:
	@echo "Building module in $(OBJDIR)..."
	@mkdir -p $(OBJDIR)
	$(MAKE) -C $(KERNELDIR) M=$(SRCDIR) MO=$(OBJDIR) modules
clean:
	@echo "Cleaning build artifacts..."
	$(MAKE) -C $(KERNELDIR) M=$(SRCDIR) MO=$(OBJDIR) clean
	rmdir $(OBJDIR) 2>/dev/null || true

.PHONY: default clean
