# SPDX-License-Identifier: GPL-2.0
# Copyright (C) 2019 MediaTek Inc.

KERNEL_ENV_PATH := $(call my-dir)
KERNEL_ROOT_DIR := $(PWD)

define touch-kernel-image-timestamp
if [ -e $(1) ] && [ -e $(2) ] && cmp -s $(1) $(2); then \
 echo $(2) has no change;\
 mv -f $(1) $(2);\
else \
 rm -f $(1);\
fi
endef

# '\\' in command is wrongly replaced to '\\\\' in kernel/out/arch/arm/boot/compressed/.piggy.xzkern.cmd
define fixup-kernel-cmd-file
if [ -e $(1) ]; then cp $(1) $(1).bak; sed -e 's/\\\\\\\\/\\\\/g' < $(1).bak > $(1); rm -f $(1).bak; fi
endef

ifneq ($(strip $(TARGET_NO_KERNEL)),true)
  KERNEL_DIR := $(KERNEL_ENV_PATH)

  ifeq ($(KERNEL_TARGET_ARCH),arm64)
    TARGET_KERNEL_CROSS_COMPILE ?= $(KERNEL_ROOT_DIR)/prebuilts/gcc/$(HOST_PREBUILT_TAG)/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
    ifeq ($(strip $(TARGET_KERNEL_USE_CLANG)),true)
      TARGET_KERNEL_CLANG_COMPILE := CLANG_TRIPLE=aarch64-linux-gnu- CC=$(KERNEL_ROOT_DIR)/prebuilts/clang/host/linux-x86/clang-r349610b/bin/clang
    endif
  else
    TARGET_KERNEL_CROSS_COMPILE ?= $(KERNEL_ROOT_DIR)/prebuilts/gcc/$(HOST_PREBUILT_TAG)/arm/arm-linux-androideabi-4.9/bin/arm-linux-androidkernel-
  endif

  ifeq ($(wildcard $(TARGET_PREBUILT_KERNEL)),)
    KERNEL_OUT ?= $(if $(filter /% ~%,$(TARGET_OUT_INTERMEDIATES)),,$(KERNEL_ROOT_DIR)/)$(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ
    ifeq ($(KERNEL_TARGET_ARCH), arm64)
      ifeq ($(MTK_APPENDED_DTB_SUPPORT), yes)
        KERNEL_ZIMAGE_OUT := $(KERNEL_OUT)/arch/$(KERNEL_TARGET_ARCH)/boot/Image.gz-dtb
      else
        KERNEL_ZIMAGE_OUT := $(KERNEL_OUT)/arch/$(KERNEL_TARGET_ARCH)/boot/Image.gz
      endif
    else
      ifeq ($(MTK_APPENDED_DTB_SUPPORT), yes)
        KERNEL_ZIMAGE_OUT := $(KERNEL_OUT)/arch/$(KERNEL_TARGET_ARCH)/boot/zImage-dtb
      else
        KERNEL_ZIMAGE_OUT := $(KERNEL_OUT)/arch/$(KERNEL_TARGET_ARCH)/boot/zImage
      endif
    endif

    BUILT_KERNEL_TARGET := $(KERNEL_ZIMAGE_OUT).bin
    INSTALLED_KERNEL_TARGET := $(PRODUCT_OUT)/kernel
    TARGET_KERNEL_CONFIG := $(KERNEL_OUT)/.config
    KERNEL_CONFIG_FILE := $(KERNEL_DIR)/arch/$(KERNEL_TARGET_ARCH)/configs/$(KERNEL_DEFCONFIG)
    KERNEL_MAKE_OPTION := O=$(KERNEL_OUT) ARCH=$(KERNEL_TARGET_ARCH) CROSS_COMPILE=$(TARGET_KERNEL_CROSS_COMPILE) $(TARGET_KERNEL_CLANG_COMPILE) ROOTDIR=$(KERNEL_ROOT_DIR) $(if $(strip $(SHOW_COMMANDS)),V=1)
    IMAGE_GZ_PATH := $(KERNEL_OUT)/arch/$(KERNEL_TARGET_ARCH)/boot/Image.gz
    ifeq ($(MTK_APPEND_DTB),)
        MTK_APPEND_DTB_PATH :=
    else
        MTK_APPEND_DTB_PATH := $(KERNEL_OUT)/arch/$(KERNEL_TARGET_ARCH)/boot/dts/mediatek/$(MTK_APPEND_DTB)
    endif
    MTK_IMAGE_GZ_DTB_PATH := $(KERNEL_OUT)/arch/$(KERNEL_TARGET_ARCH)/boot/Image.gz-dtb

  else
    BUILT_KERNEL_TARGET := $(TARGET_PREBUILT_KERNEL)
  endif #TARGET_PREBUILT_KERNEL is empty

endif #TARGET_NO_KERNEL
