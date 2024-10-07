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
    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);

    bool val = false;
    while (true) {
        pico_set_led(val);
        val = !val;
        sleep_ms(100);
    }
}
