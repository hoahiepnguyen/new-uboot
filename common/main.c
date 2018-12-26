/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/* #define	DEBUG	*/

#include <common.h>
#include <autoboot.h>
#include <cli.h>
#include <console.h>
#include <version.h>
#include <asm/gpio.h>
#include <asm/io.h>

DECLARE_GLOBAL_DATA_PTR;

/*
 * Board-specific Platform code can reimplement show_boot_progress () if needed
 */
__weak void show_boot_progress(int val) {}

static void run_preboot_environment_command(void)
{
#ifdef CONFIG_PREBOOT
	char *p;

	p = getenv("preboot");
	if (p != NULL) {
# ifdef CONFIG_AUTOBOOT_KEYED
		int prev = disable_ctrlc(1);	/* disable Control C checking */
# endif

		run_command_list(p, -1, 0);

# ifdef CONFIG_AUTOBOOT_KEYED
		disable_ctrlc(prev);	/* restore Control C checking */
# endif
	}
#endif /* CONFIG_PREBOOT */
}

#define GPIO_NRST 		66
#define GPIO_BOOT 		49
#define GPIO_PULSE 		67


static void mic_array_power_pins(void)
{
	gpio_request(GPIO_NRST, "nrst");
	gpio_request(GPIO_BOOT, "boot");
	gpio_request(GPIO_PULSE, "pulse");

	gpio_direction_output(GPIO_BOOT, 0);
	gpio_direction_output(GPIO_NRST, 0);
	udelay(10);
	gpio_direction_output(GPIO_NRST, 1);
	gpio_direction_output(GPIO_PULSE, 1);

	gpio_free(GPIO_BOOT);
	gpio_free(GPIO_NRST);
	gpio_free(GPIO_PULSE);

}
/* We come here after U-Boot is initialised and ready to process commands */
void main_loop(void)
{
	const char *s;

	bootstage_mark_name(BOOTSTAGE_ID_MAIN_LOOP, "main_loop");

#ifdef CONFIG_VERSION_VARIABLE
	setenv("ver", version_string);  /* set version variable */
#endif /* CONFIG_VERSION_VARIABLE */

	cli_init();
	mic_array_power_pins();
	run_preboot_environment_command();

#if defined(CONFIG_UPDATE_TFTP)
	update_tftp(0UL, NULL, NULL);
#endif /* CONFIG_UPDATE_TFTP */

	s = bootdelay_process();
	udelay(30);

	if (cli_process_fdt(&s))
		cli_secure_boot_cmd(s);

	autoboot_command(s);

	cli_loop();
	panic("No CLI available");
}
