#include <stdio.h>
#include "pico/stdlib.h"

int pico_led_init() {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return PICO_OK;
}

void pico_set_led(bool val) {
    gpio_put(PICO_DEFAULT_LED_PIN, val);
}

int main() {
    stdio_init_all();

    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);

    long i = 0;

    bool val = false;
    while (true) {
        pico_set_led(val);
        val = !val;
        sleep_ms(100);
        printf("Hello world!\t%d\n", i);
        i++;
    }
}
