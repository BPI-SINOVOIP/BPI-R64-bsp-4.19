.PHONY: all clean help
.PHONY: u-boot kernel kernel-config
.PHONY: linux pack

include chosen_board.mk

SUDO=sudo
#CROSS_COMPILE?=arm-linux-gnueabi-
CROSS_COMPILE=$(COMPILE_TOOL)/arm-linux-
AARCH64_CROSS_COMPILE?=aarch64-linux-gnu-
U_CROSS_COMPILE=$(CROSS_COMPILE)
#K_CROSS_COMPILE=$(CROSS_COMPILE)
K_CROSS_COMPILE=$(AARCH64_CROSS_COMPILE)

OUTPUT_DIR=$(CURDIR)/output

U_O_PATH=u-boot-mt
K_O_PATH=linux-mt
U_CONFIG_H=$(U_O_PATH)/include/config.h
K_DOT_CONFIG=$(K_O_PATH)/.config

ROOTFS=$(CURDIR)/rootfs/linux/default_linux_rootfs.tar.gz

Q=
J=$(shell expr `grep ^processor /proc/cpuinfo  | wc -l` \* 2)

all: bsp

## DK, if u-boot and kernel KBUILD_OUT issue fix, u-boot-clean and kernel-clean
## are no more needed
clean: u-boot-clean kernel-clean
	rm -f chosen_board.mk

## pack
pack: mt-pack
	$(Q)scripts/mk_pack.sh

# u-boot
$(U_CONFIG_H): u-boot-mt
	$(Q)$(MAKE) -C u-boot-mt $(UBOOT_CONFIG) CROSS_COMPILE=$(U_CROSS_COMPILE) -j$J

u-boot: $(U_CONFIG_H)
	$(Q)$(MAKE) -C u-boot-mt all CROSS_COMPILE=$(U_CROSS_COMPILE) -j$J

u-boot-clean:
	$(Q)$(MAKE) -C u-boot-mt CROSS_COMPILE=$(U_CROSS_COMPILE) -j$J distclean

## linux
$(K_DOT_CONFIG): linux-mt
	$(Q)$(MAKE) -C linux-mt ARCH=arm64 $(KERNEL_CONFIG)

kernel: $(K_DOT_CONFIG)
#	$(Q)$(MAKE) -C linux-mt ARCH=arm64 CROSS_COMPILE=${K_CROSS_COMPILE} -j$J INSTALL_MOD_PATH=output UIMAGE_LOADADDR=0x40008000 uImage dtbs
	$(Q)$(MAKE) -C linux-mt ARCH=arm64 CROSS_COMPILE=${K_CROSS_COMPILE} -j$J INSTALL_MOD_PATH=output UIMAGE_LOADADDR=0x40008000 Image dtbs
	$(Q)$(MAKE) -C linux-mt ARCH=arm64 CROSS_COMPILE=${K_CROSS_COMPILE} -j$J INSTALL_MOD_PATH=output modules
	$(Q)$(MAKE) -C linux-mt ARCH=arm64 CROSS_COMPILE=${K_CROSS_COMPILE} -j$J INSTALL_MOD_PATH=output modules_install
#	$(Q)$(MAKE) -C linux-mt ARCH=arm64 CROSS_COMPILE=${K_CROSS_COMPILE} -j$J headers_install
#	mkdir -p linux-mt/output/lib/modules/4.19.49-BPI-R64-Kernel/extra
#	cp mtk_wifi/mt_wifi_ap/mt_wifi.ko linux-mt/output/lib/modules/4.19.49-BPI-R64-Kernel/extra/

kernel-clean:
	$(Q)$(MAKE) -C linux-mt ARCH=arm64 CROSS_COMPILE=${K_CROSS_COMPILE} -j$J distclean
	rm -rf linux-mt/output/

kernel-config: $(K_DOT_CONFIG)
	$(Q)$(MAKE) -C linux-mt ARCH=arm64 CROSS_COMPILE=${K_CROSS_COMPILE} -j$J menuconfig
	cp linux-mt/.config linux-mt/arch/arm64/configs/$(KERNEL_CONFIG)

## bsp
bsp: u-boot kernel

help:
	@echo ""
	@echo "Usage:"
	@echo "  make bsp             - Default 'make'"
	@echo "  make pack            - pack the images and rootfs to a PhenixCard download image."
	@echo "  make clean"
	@echo ""
	@echo "Optional targets:"
	@echo "  make kernel           - Builds linux kernel"
	@echo "  make kernel-config    - Menuconfig"
	@echo "  make u-boot          - Builds u-boot"
	@echo ""

