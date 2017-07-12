#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifndef SHARED_MSG_PACKED
#define SHARED_MSG_PACKED __attribute__((packed))
#endif

enum shared_msg_t {
    SHARED_MSG_BOOT = 0,
    SHARED_MSG_FIRMWAREUPDATE = 1,
    SHARED_MSG_BOOT_INFO = 2,
};

struct shared_boot_msg_s {
}  SHARED_MSG_PACKED;

struct shared_firmwareupdate_msg_s {
    uint8_t local_node_id;
    uint8_t source_node_id;
    char path[201];
} SHARED_MSG_PACKED;

enum shared_periph_cal_data_fmt_t {
    SHARED_PERIPH_CAL_DATA_FMT_NO_DATA = 0,
    SHARED_PERIPH_CAL_DATA_FMT_FLOAT = 1,
};

enum shared_periph_info_pin_function_t {
    SHARED_PERIPH_INFO_PIN_FUNCTION_UNKNOWN = 0,
    SHARED_PERIPH_INFO_PIN_FUNCTION_EXT_OSC,
    SHARED_PERIPH_INFO_PIN_FUNCTION_CAN1_RX,
    SHARED_PERIPH_INFO_PIN_FUNCTION_CAN1_TX,
    SHARED_PERIPH_INFO_PIN_FUNCTION_CAN2_RX,
    SHARED_PERIPH_INFO_PIN_FUNCTION_CAN2_TX,
    SHARED_PERIPH_INFO_PIN_FUNCTION_CAN3_RX,
    SHARED_PERIPH_INFO_PIN_FUNCTION_CAN3_TX,
    SHARED_PERIPH_INFO_PIN_FUNCTION_SPI1_MISO = 21,
    SHARED_PERIPH_INFO_PIN_FUNCTION_SPI1_MOSI,
    SHARED_PERIPH_INFO_PIN_FUNCTION_SPI1_SCLK,
    SHARED_PERIPH_INFO_PIN_FUNCTION_SPI1_NCS,
};

struct shared_onboard_periph_pin_info_s {
    uint16_t function;
    uint8_t port;
    uint8_t pin;
} SHARED_MSG_PACKED;

struct shared_onboard_periph_info_s {
    const char* name;
    uint8_t rev;
    uint8_t bus_addr;
    uint8_t num_pin_descs;
    const struct shared_onboard_periph_pin_info_s* pin_descs;
    uint8_t cal_data_fmt; // < 127 reserved for standard calibration data formats defined by enum shared_periph_cal_data_fmt_t
    const void* cal_data;
} SHARED_MSG_PACKED;

struct shared_hw_info_s {
    const char* hw_name;
    uint8_t hw_major_version;
    uint8_t hw_minor_version;
    const char* mcu_model;
    const char* mcu_clock;
    uint8_t onboard_periph_desc_fmt;
    uint8_t num_onboard_periph_descs;
    const struct shared_onboard_periph_info_s* onboard_periph_descs;
} SHARED_MSG_PACKED;

struct shared_boot_info_msg_s {
    uint8_t local_node_id;
    const struct shared_hw_info_s* hw_info;
} SHARED_MSG_PACKED;

union shared_msg_payload_u {
    struct shared_boot_msg_s boot_msg;
    struct shared_firmwareupdate_msg_s firmwareupdate_msg;
    struct shared_boot_info_msg_s boot_info_msg;
};

bool shared_msg_check_and_retreive(enum shared_msg_t* msgid, union shared_msg_payload_u* msg_payload);
void shared_msg_finalize_and_write(enum shared_msg_t msgid, const union shared_msg_payload_u* msg_payload);
void shared_msg_clear(void);

const struct shared_onboard_periph_info_s* shared_hwinfo_find_periph_info(const struct shared_hw_info_s* hw_info, const char* periph_name);
const struct shared_onboard_periph_pin_info_s* shared_hwinfo_find_periph_pin_info(const struct shared_onboard_periph_info_s* periph_info, uint16_t pin_function);
