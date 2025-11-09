.PHONY: all
.SILENT: clean

CURRENT = $(shell uname -r)
KDIR = /lib/modules/$(CURRENT)/build
PWD = $(shell pwd)

TARGET = hello
obj-m := $(TARGET).o
KCOMPILER= $(shell grep -i 'CONFIG_CC_VERSION' /boot/config-$(shell uname -r ) | cut -d '"' -f2 | awk '{print $$1}') 

all:
	$(MAKE) CC=$(KCOMPILER) -C $(KDIR) M=$(PWD) modules
	
clean: 
	rm -f *.ko *.cmd .*.cmd *.mod .*.mod *.mod.* *.o .*.o *.order *.symvers
install:
	sudo insmod $(TRAGET).ko

uninstall:
	sudo rmmod $(TARGET)
