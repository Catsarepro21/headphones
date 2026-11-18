#include "codec.h"

#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

enum {
    AIC3204_ADDRESS = 0x18,
    CODEC_RESET_PIN = 8,
    PAGE_SELECT = 0,
    SOFTWARE_RESET = 1,
    CLOCK_SOURCE = 4,
    NDAC = 11,
    MDAC = 12,
    DOSR_HIGH = 13,
    DOSR_LOW = 14,
    NADC = 18,
    MADC = 19,
    AOSR = 20,
    AUDIO_INTERFACE = 27,
    BCLK_SOURCE = 29,
    BCLK_DIV = 30,
    POWERED_DIVIDER = 0x80,
};

static const struct device *const i2c = DEVICE_DT_GET(DT_NODELABEL(i2c0));
static const struct device *const gpio = DEVICE_DT_GET(DT_NODELABEL(gpio0));

static const uint8_t digital_registers[][2] = {
    {CLOCK_SOURCE, 0},
    {NDAC, POWERED_DIVIDER | 1},
    {MDAC, POWERED_DIVIDER | 2},
    {DOSR_HIGH, 0},
    {DOSR_LOW, 128},
    {NADC, POWERED_DIVIDER | 1},
    {MADC, POWERED_DIVIDER | 2},
    {AOSR, 128},
    {BCLK_SOURCE, 0x05},
    {BCLK_DIV, POWERED_DIVIDER | 4},
    {AUDIO_INTERFACE, 0x0c},
};

static int write_register(uint8_t reg, uint8_t value)
{
    return i2c_reg_write_byte(i2c, AIC3204_ADDRESS, reg, value);
}

static int configure_clocks(void)
{
    int status = write_register(PAGE_SELECT, 0);
    if (status != 0) {
        return status;
    }
    status = write_register(SOFTWARE_RESET, 1);
    if (status != 0) {
        return status;
    }
    k_msleep(2);
    for (size_t i = 0; i < ARRAY_SIZE(digital_registers); ++i) {
        const uint8_t reg = digital_registers[i][0];
        const uint8_t expected = digital_registers[i][1];
        status = write_register(reg, expected);
        if (status != 0) {
            return status;
        }
        uint8_t value;
        status = i2c_reg_read_byte(i2c, AIC3204_ADDRESS, reg, &value);
        if (status != 0) {
            return status;
        }
        if (value != expected) {
            return -EIO;
        }
    }
    return 0;
}

int codec_prepare(void)
{
    if (!device_is_ready(i2c) || !device_is_ready(gpio)) {
        return -ENODEV;
    }
    int status = gpio_pin_configure(gpio, CODEC_RESET_PIN, GPIO_OUTPUT_LOW | GPIO_OPEN_DRAIN);
    if (status != 0) {
        return status;
    }
    k_msleep(2);
    status = gpio_pin_set_raw(gpio, CODEC_RESET_PIN, 1);
    if (status != 0) {
        return status;
    }
    k_msleep(10);
    status = configure_clocks();
    if (status != 0) {
        (void)gpio_pin_set_raw(gpio, CODEC_RESET_PIN, 0);
    }
    return status;
}
