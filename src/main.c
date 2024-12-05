#include <stdio.h>
#include <stdint.h>

#define M1S_IMPLS
#include "m1s.h"

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

void pico_led_init(void);
void pico_set_led(bool val);

void InitPins( void );
void InitADC( void );

void uartHandler();

#define PWM_PERIOD_PO2 6

#define P0_PIN 10
#define P1_PIN 11
#define P2_PIN 12

#define PN0_PIN 13
#define PN1_PIN 14
#define PN2_PIN 15

#define SENSE_PIN 18
#define DEBOUNCE 100

#define PWM_MAX 32

#define IIR_AMOUNT 8

#define CUART uart0

int8_t targetSpeed = 0;

const int pins[] = {P0_PIN, P1_PIN, P2_PIN, PN0_PIN, PN1_PIN, PN2_PIN};

repeating_timer_t timer = {0};

volatile u64 lastSenseTime = 0;
volatile u64 spinDurationRaw = 0;
volatile u64 spinDurationAvrg = 0;

s32 step = 0;

s32 advanceLoadCompensation = 0;
s32 targetAdvance = 50;
s32 advance = 100;

int trapezoid(int val);
void logging();
int map(int x, int in_min, int in_max, int out_min, int out_max);

void main()
{
    multicore_launch_core1(logging);

    uart_init(CUART, 9600);
    gpio_set_function(4, UART_FUNCSEL_NUM(CUART, 16));
    gpio_set_function(5, UART_FUNCSEL_NUM(CUART, 17));

    // irq_set_exclusive_handler(CUART_IRQ, uartHandler);
    // irq_set_enabled(CUART_IRQ, true);

    pico_led_init();
    InitPins();

    bool led = false;

    pico_set_led(true);

    while (true)
    {
        uartHandler();
        if (!targetSpeed) {
            gpio_put(P0_PIN, 0);
            gpio_put(P1_PIN, 0);
            gpio_put(P2_PIN, 0);

            pwm_set_gpio_level(PN0_PIN, 0);
            pwm_set_gpio_level(PN1_PIN, 0);
            pwm_set_gpio_level(PN2_PIN, 0);
        } else {
            s32 l0 = trapezoid(step + (0<<PWM_PERIOD_PO2));
            s32 l1 = trapezoid(step + ((targetSpeed < 0 ? 4 : 2)<<PWM_PERIOD_PO2));
            s32 l2 = trapezoid(step + ((targetSpeed < 0 ? 2 : 4)<<PWM_PERIOD_PO2));

            targetAdvance = map(Abs(targetSpeed), 0, 128, 100, 40);

            gpio_put(P0_PIN, l0 > PWM_MAX ? 1 : 0);
            gpio_put(P1_PIN, l1 > PWM_MAX ? 1 : 0);
            gpio_put(P2_PIN, l2 > PWM_MAX ? 1 : 0);

            pwm_set_gpio_level(PN0_PIN, l0 <= PWM_MAX ? map(PWM_MAX-l0, 0, PWM_MAX, 0, PWM_MAX) : 0);
            pwm_set_gpio_level(PN1_PIN, l1 <= PWM_MAX ? map(PWM_MAX-l1, 0, PWM_MAX, 0, PWM_MAX) : 0);
            pwm_set_gpio_level(PN2_PIN, l2 <= PWM_MAX ? map(PWM_MAX-l2, 0, PWM_MAX, 0, PWM_MAX) : 0);

            step++;
            if (step >= (6<<PWM_PERIOD_PO2)) step = 0;
            u64 nextStep = time_us_64()+Min(advance+advanceLoadCompensation, 100);
            while (nextStep > time_us_64());

            if (advance < targetAdvance)
            {
                advance += 1;
                advance = Min(advance, targetAdvance);
            }
            if (advance > targetAdvance)
            {
                advance -= 1;
                advance = Max(advance, targetAdvance);
            }
            advanceLoadCompensation = (spinDurationAvrg>>IIR_AMOUNT) > 100 ? ((spinDurationAvrg>>IIR_AMOUNT)-100)/4 : 0;
        }
    }
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

void logging()
{
    stdio_init_all();

    while (true)
    {
        printf("%f, %llu, %d, %d, %d\n",
            1/((f32)((6<<PWM_PERIOD_PO2)*(u64)advance)*(1e-6f)),
            (6<<PWM_PERIOD_PO2)*(u64)advance,
            advance,
            advanceLoadCompensation,
            targetSpeed);
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

void senseCb(uint gpio, u32 events)
{
    u64 now = time_us_64();

    for (s32 i = 0; i < DEBOUNCE; ++i) if (!gpio_get(SENSE_PIN)) return;

    spinDurationRaw = (now - lastSenseTime);
    spinDurationAvrg = spinDurationAvrg + spinDurationRaw - (spinDurationAvrg>>IIR_AMOUNT);
    lastSenseTime = now;
}

void InitPins(void)
{
    for (int i = 0; i < 3; ++i)
    {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_OUT);
    }

    for (int i = 3; i < 6; ++i)
    {
        gpio_set_function(pins[i], GPIO_FUNC_PWM);
        u32 sliceNum = pwm_gpio_to_slice_num(pins[i]);
        pwm_set_clkdiv(sliceNum, 50);
        pwm_set_wrap(sliceNum, PWM_MAX);
        pwm_set_gpio_level(pins[i], 0);
        pwm_set_enabled(sliceNum, true);
    }

    gpio_init(SENSE_PIN);
    gpio_set_dir(SENSE_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(SENSE_PIN, GPIO_IRQ_EDGE_RISE, true, &senseCb);

    // gpio_init(SENSE_PIN+1);
    // gpio_set_dir(SENSE_PIN+1, GPIO_OUT);
    // gpio_put(SENSE_PIN+1, false);
}

int map(int x, int in_min, int in_max, int out_min, int out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void uartHandler()
{
    if (uart_is_readable(CUART)) {
        uart_read_blocking(CUART, (uint8_t*)&targetSpeed, 1);
    }
}
