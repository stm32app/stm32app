#include <actor/lib/fsmc.h>
#include <actor/module/sdram.h>
#include <libopencm3/stm32/fsmc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

/*
 * This is just syntactic sugar but it helps, all of these
 * GPIO pins get configured in exactly the same way.
 */
static struct {
    uint32_t gpio;
    uint16_t pins;
} sdram_pins[4] = {
    {GPIOD, GPIO0 | GPIO1 | GPIO4 | GPIO5 				| GPIO8 | GPIO9 | GPIO10 | GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15},
    {GPIOE, GPIO0 | GPIO1 |								  GPIO7 | GPIO8 | GPIO9 | GPIO10 | GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15},
    {GPIOF, GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 | GPIO5 | GPIO12 | GPIO13 | GPIO14 | GPIO15},
    {GPIOG, GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 | GPIO5 | GPIO10},
};

#define Bank1_SRAM3_ADDR ((u32)(0x68000000))
void start_sdram(void) {
    int i;
    uint32_t cr_tmp, tr_tmp; /* control, timing registers */

    uint32_t bank = 2;  // NORSRAM3
    /*
     * First all the GPIO pins that end up as SDRAM pins
     */
    rcc_periph_clock_enable(RCC_GPIOD);
    rcc_periph_clock_enable(RCC_GPIOE);
    rcc_periph_clock_enable(RCC_GPIOF);
    rcc_periph_clock_enable(RCC_GPIOG);
    rcc_peripheral_enable_clock(&RCC_AHB3ENR, RCC_AHB3ENR_FSMCEN);
		
    for (i = 0; i < 4; i++) {
        gpio_mode_setup(sdram_pins[i].gpio, GPIO_MODE_AF, GPIO_PUPD_PULLUP, sdram_pins[i].pins);
        gpio_set_output_options(sdram_pins[i].gpio, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, sdram_pins[i].pins);
        gpio_set_af(sdram_pins[i].gpio, GPIO_AF12, sdram_pins[i].pins);
    }

    /* Enable the SDRAM Controller */
    rcc_periph_clock_enable(RCC_FSMC);

    fsmc_set_address_setup_phase_duration(bank, 1);
    fsmc_set_address_hold_phase_duration(bank, 0);
    fsmc_set_data_phase_duration(bank, 5);
    fsmc_set_turnaround_phase_duration(bank, 0);
    fsmc_set_clock_divide_ratio(bank, 0);
    fsmc_set_data_latency(bank, 0);
    fsmc_set_access_mode(bank, FSMC_BTx_ACCMOD_A);

    fsmc_address_data_multiplexing_enable(bank);

    fsmc_set_memory_width(bank, FMC_SDCR_MWID_16b);
    fsmc_async_wait_disable(bank);
    fsmc_write_enable(bank);
    fsmc_wait_signal_polarity_low(bank);
    fsmc_wait_timing_configuration_enable(bank);

    fsmc_wrapped_burst_mode_disable(bank);
    fsmc_wait_disable(bank);
    fsmc_extended_mode_disable(bank);
    fsmc_write_burst_disable(bank);

    fsmc_memory_bank_enable(bank);

		/*uint8_t *byte = ((uint8_t *) 0x68000000);
		for (uint32_t z = 0; z < 0x100000; z++) {
				volatile uint8_t *ptr = byte + z;
			for (uint32_t j = 0; j < 8; j++) {
				volatile uint8_t v = 1 << j;
				*ptr = v;
				if (*ptr != v) {
					__asm("BKPT #0\n"); // Break into the debuggezr
					while (1) {
					}
				}
			}
		}*/
/*
		sdram_command(SDRAM_BANK1, SDRAM_CLK_CONF, 1, 0);
    sdram_command(SDRAM_BANK1, SDRAM_PALL, 1, 0);
    sdram_command(SDRAM_BANK1, SDRAM_AUTO_REFRESH, 4, 0);
    sdram_command(SDRAM_BANK1, SDRAM_LOAD_MODE, 1, SDRAM_MODE_BURST_LENGTH_2              |
                SDRAM_MODE_BURST_TYPE_SEQUENTIAL    |
                SDRAM_MODE_CAS_LATENCY_3        |
                SDRAM_MODE_OPERATING_MODE_STANDARD  |
                SDRAM_MODE_WRITEBURST_MODE_SINGLE);

    FMC_SDRTR = 683; */

/*
    sdram_command(SDRAM_BANK1, SDRAM_CLK_CONF, 1, 0);
    sdram_command(SDRAM_BANK1, SDRAM_PALL, 1, 0);
    sdram_command(SDRAM_BANK1, SDRAM_AUTO_REFRESH, 4, 0);*/
}