#include <stdio.h>
#include <stdint.h>

#define M1S_IMPLS
#include "m1s.h"

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

void pico_led_init(void);
void pico_set_led(bool val);

void InitPins( void );
void InitADC( void );

#define P0_PIN 10
#define P1_PIN 11
#define P2_PIN 12

#define PN0_PIN 13
#define PN1_PIN 14
#define PN2_PIN 15

#define SENSE_PIN 16
#define DEBOUNCE 100

#define PWM_MAX 255

#define IIR_AMOUNT 8

const int pins[] = {P0_PIN, P1_PIN, P2_PIN, PN0_PIN, PN1_PIN, PN2_PIN};

repeating_timer_t timer = {0};

bool p[6] = {1, 1, 0, 0, 0, 0};
bool n[6] = {0, 0, 0, 1, 1, 0};

int pos = 0;

volatile u64 lastSenseTime = 0;

volatile u64 spinDurationRaw = 0;
volatile u64 spinDurationRawGated = 0;
volatile u64 spinDurationAvrg = 0;

volatile u64 stepDuration = 1000;

volatile u64 nextStepTime = 0;
u64 pulseStartTime = 0;
u64 pulseStopTime = 0;

volatile s32 step = 0;

volatile f32 freq = 0;

f32 driveLevel = 1.f;

volatile bool spinup = true;

void logging();
void spinUp();

void main()
{
    multicore_launch_core1(logging);

    pico_led_init();
    InitPins();

    bool led = false;

    pico_set_led(false);

    spinUp();
    driveLevel = 0.3f;

    pico_set_led(true);

    while (true)
    {
        u64 now = time_us_64();

        if (now > nextStepTime)
        {
            nextStepTime = now + stepDuration;
            step++;
            step %= 6;

            f32 halfStep = 0.5f*(f32)stepDuration;

            pulseStartTime = now+((1.f-driveLevel)*halfStep);
            pulseStopTime  = now+((1.f+driveLevel)*halfStep);
        }

        gpio_put(P0_PIN,   p[(step)%6] && (now >= pulseStartTime) && (now <= pulseStopTime));
        gpio_put(P1_PIN, p[(step+2)%6] && (now >= pulseStartTime) && (now <= pulseStopTime));
        gpio_put(P2_PIN, p[(step+4)%6] && (now >= pulseStartTime) && (now <= pulseStopTime));

        gpio_put(PN0_PIN,   n[(step)%6] && (now >= pulseStartTime) && (now <= pulseStopTime));
        gpio_put(PN1_PIN, n[(step+2)%6] && (now >= pulseStartTime) && (now <= pulseStopTime));
        gpio_put(PN2_PIN, n[(step+4)%6] && (now >= pulseStartTime) && (now <= pulseStopTime));
    }
}

void spinUp()
{
    stepDuration = 4000;
    driveLevel = 1.f;
    spinup = true;

    while(stepDuration > 1000)
    {
        u64 now = time_us_64();

        if (now > nextStepTime)
        {
            stepDuration -= 5;
            nextStepTime = now + stepDuration;
            step++;
            step %= 6;

            f32 halfStep = 0.5f*(f32)stepDuration;

            pulseStartTime = now+(u64)((1.f-driveLevel)*halfStep);
            pulseStopTime = now+(u64)((1.f+driveLevel)*halfStep);
        }

        gpio_put(P0_PIN,   p[(step)%6] && (now >= pulseStartTime) && (now <= pulseStopTime));
        gpio_put(P1_PIN, p[(step+2)%6] && (now >= pulseStartTime) && (now <= pulseStopTime));
        gpio_put(P2_PIN, p[(step+4)%6] && (now >= pulseStartTime) && (now <= pulseStopTime));

        gpio_put(PN0_PIN,   n[(step)%6] && (now >= pulseStartTime) && (now <= pulseStopTime));
        gpio_put(PN1_PIN, n[(step+2)%6] && (now >= pulseStartTime) && (now <= pulseStopTime));
        gpio_put(PN2_PIN, n[(step+4)%6] && (now >= pulseStartTime) && (now <= pulseStopTime));
    }

    spinup = false;
}

void logging()
{
    stdio_init_all();

    while (true)
    {
        printf("%llu, %llu, %llu, %llu | %llu, %llu, %llu\n",
            (spinDurationAvrg>>IIR_AMOUNT),
            spinDurationRaw,
            spinDurationRawGated,
            stepDuration,
            pulseStartTime,
            pulseStopTime,
            pulseStopTime-pulseStartTime);
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

    if (spinDurationRaw < stepDuration*4 || spinDurationRaw > stepDuration*7)
    { lastSenseTime = now; return; }

    spinDurationRawGated = spinDurationRaw;
    spinDurationAvrg = spinDurationAvrg + spinDurationRaw - (spinDurationAvrg>>IIR_AMOUNT);
    lastSenseTime = now;

    if (spinup) return;

    f32 targetStepDuration = (f32)(spinDurationAvrg>>IIR_AMOUNT)/6.f;
    if (stepDuration < targetStepDuration)
        stepDuration += 10;
    else
        stepDuration -= 10;
    stepDuration = Max(stepDuration, 100);
    step = 0;
}

void InitPins(void)
{
    for (int i = 0; i < 6; ++i)
    {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_OUT);
    }

    // for (int i = 0; i < 3; ++i)
    // {
    //     gpio_set_function(pins[i], GPIO_FUNC_PWM);
    //     u32 sliceNum = pwm_gpio_to_slice_num(pins[i]);
    //     pwm_set_clkdiv(sliceNum, 50);
    //     pwm_set_wrap(sliceNum, PWM_MAX);
    //     pwm_set_gpio_level(pins[i], 0);
    //     pwm_set_enabled(sliceNum, true);
    // }

    gpio_init(SENSE_PIN);
    gpio_set_dir(SENSE_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(SENSE_PIN, GPIO_IRQ_EDGE_RISE, true, &senseCb);

    // gpio_init(SENSE_PIN+1);
    // gpio_set_dir(SENSE_PIN+1, GPIO_OUT);
    // gpio_put(SENSE_PIN+1, false);
}
