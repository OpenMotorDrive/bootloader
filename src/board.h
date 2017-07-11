#pragma once
#include "shared.h"

#define APP_PAGE_SIZE 2048
#define HW_NAME "org.jc.esc"
#define HW_MAJOR_VER 1
#define HW_MINOR_VER 0

static const struct onboard_periph_pin_info_s drv8305_pin_descriptions[] = {
    {.function = 0, .port = 0, .pin = 8},  // PA8->PWM_A
    {.function = 1, .port = 0, .pin = 9},  // PA9->PWM_B
    {.function = 2, .port = 0, .pin = 10}, // PA10->PWM_C
};

static const struct onboard_periph_info_s onboard_periph_descriptions[] = {
    {
        .name = "DRV8305",
        .rev = 0,
        .bus_addr = 0,
        .bus_type = PERIPH_INFO_BUS_TYPE_SPI, // SPI
        .bus_num = 0,
        .num_pin_descriptions = sizeof(drv8305_pin_descriptions)/sizeof(struct onboard_periph_pin_info_s),
        .pin_descriptions = drv8305_pin_descriptions,
        .cal_data_fmt = 0,
        .calibration_data = nullptr
    },
};

static const struct hw_info_s hw_info = {
    .hw_name = HW_NAME,
    .hw_major_version = HW_MAJOR_VER,
    .hw_minor_version = HW_MINOR_VER,
    .onboard_periph_description_fmt = 0,
    .num_onboard_periph_descriptions = sizeof(onboard_periph_descriptions)/sizeof(struct onboard_periph_info_s),
    .onboard_periph_descriptions = onboard_periph_descriptions
};
