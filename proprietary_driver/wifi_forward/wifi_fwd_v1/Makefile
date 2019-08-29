ifneq ($(CONFIG_SUPPORT_OPENWRT),)

obj-m += wifi_forward.o
wifi_forward-objs := wifi_fwd.o hook.o

EXTRA_CFLAGS += -w
#CFLAGS += -w
else

PWD = $(shell pwd)
KERNEL_DIR = $(PWD)/../../../$(LINUXDIR)
ROMFS_DIR = $(PWD)/../../../romfs
#@echo $(PWD);
#ccflags-y := -I../proprietary_driver/drivers/cryptodev
wifi_forward-objs = wifi_fwd.o hook.o
obj-m += wifi_forward.o

modules:
	make -C $(KERNEL_DIR) M=$(PWD) modules

modules_install:
	make INSTALL_MOD_PATH=$(ROMFS_DIR) -C $(KERNEL_DIR) M=$(PWD) modules_install

clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean

endif