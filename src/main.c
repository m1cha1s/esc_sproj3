#include <stdio.h>
#include "pico/stdlib.h"

#include "hardware/pwm.h"

int pico_led_init() {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return PICO_OK;
}

void pico_set_led(bool val) {
    gpio_put(PICO_DEFAULT_LED_PIN, val);
}

void init_pwm(void) {
    gpio_set_function(
}


bool callback(struct repeating_timer *t) {
    bool *val = (bool*)t->user_data;
    pico_set_led(*val);
    *val = !(*val);

    return true;
}

int main() {
    bool val = false;
    struct repeating_timer timer = {0};
    add_repeating_timer_ms(-10, callback, &val, &timer);
    
    stdio_init_all();

    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);

    long i = 0;

    while (true) {
        sleep_ms(10);
        printf("Hello world!\t%d\n", i);
        i++;
    }
}
