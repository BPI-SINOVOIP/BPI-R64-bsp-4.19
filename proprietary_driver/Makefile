-include ../config/.config

modules:
ifeq ($(CONFIG_PROPRIETARY_MODULES_CRYPTODEV),y)
	make -C drivers/cryptodev modules
	make -C drivers/cryptodev modules_install
endif

modules_install:
ifeq ($(CONFIG_PROPRIETARY_MODULES_CRYPTODEV),y)
	make -C drivers/cryptodev modules
	make -C drivers/cryptodev modules_install
endif

clean:
ifeq ($(CONFIG_PROPRIETARY_MODULES_CRYPTODEV),y)
	make -C drivers/cryptodev clean
endif

