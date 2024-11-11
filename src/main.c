#include <stdio.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/dma.h"

#define Abs(val) ((val) < 0 ? -(val) : (val))
#define Min(a,b) ((a) < (b) ? (a) : (b))
#define Max(a,b) ((a) > (b) ? (a) : (b))
#define Len(arr) (sizeof(arr)/sizeof(*(arr)))
#define ToLower(c) (((c) >= 'A' && (c) <= 'Z') ? ((c)+32) : (c))
#define ToUpper(c) (((c) >= 'a' && (c) <= 'z') ? ((c)-32) : (c))

int trapezoid(int val);
void pico_led_init(void);
void pico_set_led(bool val);
void init_pwm(void);
void initADC(void);
bool callback(struct repeating_timer *t);

#define P0_PIN 10
#define P1_PIN 11
#define P2_PIN 12

#define PN0_PIN 13
#define PN1_PIN 14
#define PN2_PIN 15

#define ADC_PIN 26

#define PWM_PERIOD_PO2 9

#define PWM_PIN PICO_DEFAULT_LED_PIN

repeating_timer_t timer = {0};
int pos = 0;

int time = 1000;
volatile bool trig = false;

uint8_t adcVal = 0;

int main()
{
    stdio_init_all();

    //pico_led_init();
    init_pwm();
    initADC();
    add_repeating_timer_us(-time, callback, &pos, &timer);

    while (true) {
        static int upCheckNext = 0;
        static int upCheckNextId = 0;
        static int adcInfliction[6] = {0};

        int p0 = trapezoid(pos + (0<<PWM_PERIOD_PO2))-256;
        int p1 = trapezoid(pos + (2<<PWM_PERIOD_PO2))-256;
        int p2 = trapezoid(pos + (4<<PWM_PERIOD_PO2))-256;

        pwm_set_gpio_level(P0_PIN, p0 >= 90 ? p0 : 0);
        pwm_set_gpio_level(P1_PIN, p1 >= 90 ? p1 : 0);
        pwm_set_gpio_level(P2_PIN, p2 >= 90 ? p2 : 0);

        pwm_set_gpio_level(PN0_PIN, p0 < -90 ? -p0 : 0);
        pwm_set_gpio_level(PN1_PIN, p1 < -90 ? -p1 : 0);
        pwm_set_gpio_level(PN2_PIN, p2 < -90 ? -p2 : 0);

        pos++;
        if (pos >= (6<<PWM_PERIOD_PO2)) pos=0;

        if (pos == upCheckNext)
        {
            adcInfliction[upCheckNextId] = adc_read();

            upCheckNext += (1<<PWM_PERIOD_PO2);
            upCheckNextId++;

            if (upCheckNext == (6<<PWM_PERIOD_PO2))
            {
                upCheckNext = 0;
                upCheckNextId = 0;

                time-=1;
                if (time < 10) time = 1000;
            }
        }
        timer.delay_us=-Max((time),1);
    }
    while (!trig);
}

int trapezoid( int val )
{
	if( val >= ((1<<PWM_PERIOD_PO2)*6) )
	{
		val -= ((1<<PWM_PERIOD_PO2)*6);
	}
	if( val < ((1<<PWM_PERIOD_PO2)*3) )
	{
		if( val < ((1<<PWM_PERIOD_PO2)) )
		{
			return val;
		}
		else
		{
			return ((1<<PWM_PERIOD_PO2))-1;
		}
	}
	else
	{
		if( val < ((1<<PWM_PERIOD_PO2)*4) )
			return ((1<<PWM_PERIOD_PO2)*4) - 1 - val;
		else
			return 0;
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

void init_pwm(void)
{
    gpio_init(P0_PIN);
    gpio_set_function(P0_PIN, GPIO_FUNC_PWM);
    uint32_t slice_num = pwm_gpio_to_slice_num(P0_PIN);
    pwm_set_wrap(slice_num, 256);
    pwm_set_gpio_level(P0_PIN, 0);
    pwm_set_enabled(slice_num, true);

    gpio_init(P1_PIN);
    gpio_set_function(P1_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(P1_PIN);
    pwm_set_wrap(slice_num, 256);
    pwm_set_gpio_level(P1_PIN, 0);
    pwm_set_enabled(slice_num, true);

    gpio_init(P2_PIN);
    gpio_set_function(P2_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(P2_PIN);
    pwm_set_wrap(slice_num, 256);
    pwm_set_gpio_level(P2_PIN, 0);
    pwm_set_enabled(slice_num, true);

    gpio_init(PN0_PIN);
    gpio_set_function(PN0_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(PN0_PIN);
    pwm_set_wrap(slice_num, 256);
    pwm_set_gpio_level(PN0_PIN, 0);
    pwm_set_enabled(slice_num, true);

    gpio_init(PN1_PIN);
    gpio_set_function(PN1_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(PN1_PIN);
    pwm_set_wrap(slice_num, 256);
    pwm_set_gpio_level(PN1_PIN, 0);
    pwm_set_enabled(slice_num, true);

    gpio_init(PN2_PIN);
    gpio_set_function(PN2_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(PN2_PIN);
    pwm_set_wrap(slice_num, 256);
    pwm_set_gpio_level(PN2_PIN, 0);
    pwm_set_enabled(slice_num, true);
}

void initADC(void)
{
    adc_init();

    adc_gpio_init(ADC_PIN);
    adc_select_input(0);

    // adc_fifo_setup(true,
    //                true,
    //                1,
    //                false,
    //                true);

    adc_set_clkdiv(0);

    // int chan = dma_claim_unused_channel(true);
    // dma_channel_config c = dma_channel_get_default_config(chan);
    // channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    // channel_config_set_read_increment(&c, false);
    // channel_config_set_write_increment(&c, false);

    // channel_config_set_dreq(&c, DREQ_ADC);

    // channel_config_set_ring(&c, true, 1);

    // dma_channel_configure(chan, &c, &adcVal, &adc_hw->fifo, 10, true);
    // adc_run(true);
}


bool callback(struct repeating_timer *t)
{
    trig=true;
    return true;
}
