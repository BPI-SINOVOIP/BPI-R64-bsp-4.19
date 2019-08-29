PWD = $(shell pwd)
KERNEL_DIR = $(PWD)/../../../$(LINUXDIR)
ROMFS_DIR = $(PWD)/../../../romfs
ccflags-y := -I../proprietary_driver/drivers/cryptodev
cryptodev-objs = ioctl.o main.o cryptlib.o authenc.o zc.o util.o
obj-m += cryptodev.o

modules:
	make -C $(KERNEL_DIR) M=$(PWD) modules

modules_install:
	make INSTALL_MOD_PATH=$(ROMFS_DIR) -C $(KERNEL_DIR) M=$(PWD) modules_install

clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean

