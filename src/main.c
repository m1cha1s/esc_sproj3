#include <stdio.h>
#include <stdint.h>

#define M1S_IMPLS
#include "m1s.h"

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/dma.h"

void pico_led_init(void);
void pico_set_led(bool val);

void InitPins(void);
void InitADC(void);

#define P0_PIN 10
#define P1_PIN 11
#define P2_PIN 12

#define PN0_PIN 13
#define PN1_PIN 14
#define PN2_PIN 15

const int pins[] = {P0_PIN, P1_PIN, P2_PIN, PN0_PIN, PN1_PIN, PN2_PIN};

#define ADC_PIN 26

#define PWM_PERIOD_PO2 9

#define PWM_PIN PICO_DEFAULT_LED_PIN

#define Lerp(a, b, t) (a+t*(b-a))

repeating_timer_t timer = {0};

/* 0 1 1 0 0 0 */

bool p[6] = {1, 1, 0, 0, 0, 0};
bool n[6] = {0, 0, 0, 1, 1, 0};

int pos = 0;

volatile bool trig = false;

void core1();

int main()
{
    stdio_init_all();
    pico_led_init();
    InitPins();
    InitADC();

    multicore_launch_core1(core1);
    while(1)
    {
        // printf("%d: %d\n", multicore_fifo_pop_blocking(), multicore_fifo_pop_blocking());
        printf("%d\n", multicore_fifo_pop_blocking());
        // u32 val = multicore_fifo_pop_blocking();
        // u32 n = (val>>16);
        // u32 counts = (val & 0xffff);
        // printf("%d: %d\n", n, counts);
    }
}

void usb_log(s32 val) 
{
    if (multicore_fifo_wready())
    {
        /* multicore_fifo_push_blocking(spinningness); */
        multicore_fifo_push_blocking(val);
    }
}

void core1()
{
    bool led = false;
    f32 time = 0;
    f32 step = 0.001;
    s32 next_check = 0;
    s32 adc_values[6] = {0};

    f32 ramp = 0;

    while (true)
    {
        if (time > (f32)next_check+0.3f)
        {
            adc_values[next_check] = adc_read();
            // adc_values[next_check] /= 2;
            next_check += 1;

            // u32 val = 0;
            // val |= ((next_check-1)<<16);
            // val |= (0xffff & adc_values[next_check-1]);
            // usb_log(val);

            // usb_log(next_check-1);
            // usb_log(adc_values[next_check-1]);
            
            if (next_check >= 6)
            {
                next_check = 0;
                
                s32 avrg = 0;
                for (s32 i = 0; i < Len(adc_values); ++i)
                {
                    avrg += adc_values[i]/Len(adc_values);
                }
                s32 maxDelta = 0;
                for (s32 i = 0; i < Len(adc_values); ++i)
                {
                    maxDelta = Max(Abs(avrg-adc_values[i]), maxDelta);
                }

                usb_log(avrg);
                
                // s32 spinningness = adc_values[1]-adc_values[4];

                // if (spinningness < 0)
                // {
                //     usb_log(adc_values[1]);
                // }
               
                // usb_log(spinningness);
            
                // if (spinningness < 50 && ramp >= 0.15)
                // {
                //      ramp = 0;
                // }
            }
        }

        gpio_put(P0_PIN, p[((int)time)%6]);
        gpio_put(P1_PIN, p[((int)time+2)%6]);
        gpio_put(P2_PIN, p[((int)time+4)%6]);

        gpio_put(PN0_PIN, n[((int)time)%6]);
        gpio_put(PN1_PIN, n[((int)time+2)%6]);
        gpio_put(PN2_PIN, n[((int)time+4)%6]);
        
        time += Lerp(step, 0.01, ramp);
        if (time > 6.f)
        {
            time = 0;
            if (ramp < 1) ramp += 0.002;
        }
    }
}

void pico_led_init(void)
{
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
}

void pico_set_led(bool val)
{
    gpio_put(PICO_DEFAULT_LED_PIN, val);
}

void InitPins(void)
{

    for (int i = 0; i < Len(pins); ++i)
    {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_OUT);
    }
}

void InitADC(void)
{
    adc_init();

    adc_gpio_init(ADC_PIN);
    adc_select_input(0);

    adc_set_clkdiv(0);
}

