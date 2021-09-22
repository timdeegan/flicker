#include "hardware/adc.h"
#include "hardware/dma.h"

#include "assertions.h"
#include "sample.h"

/* DMA channel used to carry samples to RAM. */
static unsigned int channel;
static dma_channel_config config;

/* Set up the ADC hardware once at boot time. */
void sample_init(unsigned int pin)
{
    /* Valid ADC pins are 26-29, a.k.a. ADC inputs 0-3. */
    ASSERT(pin >= 26 && pin <= 29);
    unsigned int input = pin - 26;

    /* GPIO pin. */
    adc_gpio_init(pin);

    /* ADC engine. */
    adc_init();
    adc_run(false);
    adc_select_input(input);
    adc_fifo_setup(
        true,   /* FIFO enabled. */
        true,   /* DMA requests enabled. */
        1,      /* DMA when FIFO level >= 1. */
        true,   /* Use bit 15 as error flag. */
        false); /* Use all 12 data bits. */

    /* DMA engine config.
     * This just populates config, doesn't prod hardware yet. */
    channel = dma_claim_unused_channel(true);
    config = dma_channel_get_default_config(channel);
    channel_config_set_read_increment(&config, false);
    channel_config_set_write_increment(&config, true);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_16);
    channel_config_set_dreq(&config, DREQ_ADC);
}

/* Take @count ADC samples at @hz Hz.
 * Blocks until sampling is complete. */
void sample(unsigned int count, float hz, uint16_t *dest)
{
    /* The ADC samples every (1 + clkdiv) 48MHz cycles, on average.
     * Minimum clkdiv is 95 -> 48MHz/96 = 500kHz. */
    ASSERT(hz <= 500e3);
    float cycles = 48e6/hz;
    adc_set_clkdiv(cycles - 1.0);

    /* Clear old state, just in case. */
    adc_run(false);
    adc_fifo_drain();

    /* Start the DMA engine. */
    dma_channel_configure(
        channel,        /* On our reserved channel. */
        &config,        /* Using our config for inbound DMA. */
        dest,           /* To the caller's buffer. */
        &adc_hw->fifo,  /* From the ADC engine's FIFO. */
        count,          /* As many samples as we were asked for. */
        true);          /* Starting now. */

    /* Start sampling. */
    adc_run(true);

    /* Wait for all our samples to arrive. */
    dma_channel_wait_for_finish_blocking(channel);

    /* Stop the ADC. */
    adc_run(false);
}
