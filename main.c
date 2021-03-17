#include <stdio.h>

#include "hardware/gpio.h"

#include "pico/stdlib.h"
#include "pico/binary_info.h"

int main(void) {

    /* Debugging metadata that gets baked into the binary. */
    bi_decl(bi_program_name("flicker"));
    bi_decl(bi_program_version_string("0.1"));
    bi_decl(bi_program_description("Lighting flicker meter"));

    /* TODO: bi_decl(bi_1pin_with_name(...)) for known pins. */

    /* Debugging output will go to the USB console. */
    stdio_init_all();

    /* Trivial output until we have something better to do. */
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    while(1) {
        printf("Greetings, human!\n");
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(1000);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(1000);
    }
}
