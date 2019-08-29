
#include <common.h>
#include <spl.h>

#include <asm/io.h>
#include <asm/arch/spl.h>
#include "../../../../../drivers/flash/mtk_nor.h"


DECLARE_GLOBAL_DATA_PTR;

extern int mtk_nor_init(void);

void board_init_f(ulong dummy)
{

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	/* Set global data pointer. */
	gd = &gdata;

	arch_cpu_init();
	preloader_console_init();

	mtk_nor_init();
	board_init_r(NULL, 0);
}

u32 spl_boot_device(void)
{
	int mode = -1;
	#if defined(CONFIG_CMD_NAND)
	mode = BOOT_DEVICE_NAND;
	#endif
	#if defined(CONFIG_CMD_NOR)
	mode = BOOT_DEVICE_NOR;
	#endif
	#ifdef CONFIG_GENERIC_MMC
	mode = BOOT_DEVICE_MMC1;
	#endif
	if (mode == -1)
	{	
	puts("Unsupported boot mode selected\n");
	hang();
	}

	return mode;
}

#ifdef CONFIG_SPL_OS_BOOT
int spl_start_uboot(void)
{
	/* boot linux */
	return 1;
}
#endif

