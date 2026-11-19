#include "battery.h"

#include <errno.h>
#include <stdbool.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>

enum {
    SAMPLE_PERIOD_MS = 30000,
    PACK_DIVIDER_UPPER_KOHMS = 1000,
    PACK_DIVIDER_LOWER_KOHMS = 56,
};

static const struct adc_dt_spec pack_adc = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));
static uint32_t last_sample;
static bool sampled;

int battery_start(void)
{
    return adc_is_ready_dt(&pack_adc) ? adc_channel_setup_dt(&pack_adc) : -ENODEV;
}

void battery_poll(uint32_t now)
{
    if (sampled && now - last_sample < SAMPLE_PERIOD_MS) {
        return;
    }
    sampled = true;
    last_sample = now;

    int16_t sample;
    struct adc_sequence sequence = {.buffer = &sample, .buffer_size = sizeof(sample)};
    int status = adc_sequence_init_dt(&pack_adc, &sequence);
    if (status == 0) {
        status = adc_read_dt(&pack_adc, &sequence);
    }
    if (status < 0) {
        printk("Battery ADC error: %d\n", status);
        return;
    }
    if (sample <= 0) {
        printk("Battery: pack voltage unavailable\n");
        return;
    }

    int32_t sense_uv = sample;
    status = adc_raw_to_microvolts_dt(&pack_adc, &sense_uv);
    if (status < 0) {
        printk("Battery conversion error: %d\n", status);
        return;
    }
    const int32_t pack_mv = sense_uv * (PACK_DIVIDER_UPPER_KOHMS + PACK_DIVIDER_LOWER_KOHMS) /
                            (1000 * PACK_DIVIDER_LOWER_KOHMS);
    printk("Battery: %d mV\n", (int)pack_mv);
}
