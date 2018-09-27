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
#define GPIO_BUTTON		49
#define GPIO_LED		2
#define GPIO_NRST 		66
#define GPIO_BOOT 		61

static ulong measureTimeButtonPressed(void)
{
	int buttonState;
	ulong start;
	ulong timeMeasure = 0;

	start = get_timer(0);
	buttonState = gpio_get_value(GPIO_BUTTON);
	while(buttonState > 0)
	{
		//run here waiting for button released
		buttonState = gpio_get_value(GPIO_BUTTON); //Update button state
	}
	timeMeasure = get_timer(start);

	printf("GPIO_BUTTON: Time taken: %lu millisec\n", timeMeasure);

	return timeMeasure;
}

static void check_gpio_pin_rst(void)
{
	ulong val;

	gpio_request(GPIO_LED, "led");
	gpio_request(GPIO_BUTTON, "button");

	gpio_direction_input(GPIO_BUTTON);
	gpio_direction_output(GPIO_LED, 1);

	val = measureTimeButtonPressed();

	if(val > 3000)
	{
		printf("RESTORE FIRMWARE NOW, PLEASE KEEP CABLE WHILE RESTORING\n");
		gpio_direction_output(GPIO_LED, 0);
	}
	else {
		gpio_direction_output(GPIO_LED, 1);
	}
	gpio_free(GPIO_LED);
	gpio_free(GPIO_BUTTON);
}

static void mic_array_power_pins(void)
{
	gpio_request(GPIO_NRST, "nrst");
	gpio_request(GPIO_BOOT, "boot");

	gpio_direction_output(GPIO_BOOT, 0);
	gpio_direction_output(GPIO_NRST, 0);
	udelay(10);
	gpio_direction_output(GPIO_NRST, 1);

	gpio_free(GPIO_BOOT);
	gpio_free(GPIO_NRST);
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
