#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifndef SHARED_MSG_PACKED
#define SHARED_MSG_PACKED __attribute__((packed))
#endif

enum shared_msg_t {
    SHARED_MSG_BOOT = 0,
    SHARED_MSG_FIRMWAREUPDATE = 0,
};

struct boot_msg_s {
}  SHARED_MSG_PACKED;

struct firmwareupdate_msg_s {
    uint8_t local_node_id;
    uint8_t source_node_id;
    char path[201];
} SHARED_MSG_PACKED;

struct onboard_periph_pin_info_s {
    uint8_t function; // enum defined per-application
    uint8_t port : 3;
    uint8_t pin : 5;
} SHARED_MSG_PACKED;

struct onboard_periph_info_s {
    const char* name;
    uint8_t rev;
    uint8_t bus_addr;
    uint8_t bus_type : 4;
    uint8_t bus_num : 4;
    uint8_t num_pin_descriptions;
    const struct onboard_periph_pin_info_s* pin_descriptions;
    uint8_t cal_data_fmt;
    const void* calibration_data;
} SHARED_MSG_PACKED;

struct hw_info_s {
    const char* hw_name;
    uint8_t hw_major_version;
    uint8_t hw_minor_version;
    uint8_t onboard_periph_description_fmt;
    uint8_t num_onboard_periph_descriptions;
    const struct onboard_periph_info_s* onboard_periph_descriptions;
} SHARED_MSG_PACKED;

struct boot_info_msg_s {
    uint8_t local_node_id;
    const struct hw_info_s* hw_info;
} SHARED_MSG_PACKED;

union shared_msg_payload_u {
    struct boot_msg_s boot_msg;
    struct firmwareupdate_msg_s firmwareupdate_msg;
    struct boot_info_msg_s boot_info_msg;
};

bool shared_msg_check_and_retreive(enum shared_msg_t* msgid, union shared_msg_payload_u* msg_payload);
void shared_msg_finalize_and_write(enum shared_msg_t msgid, const union shared_msg_payload_u* msg_payload);
void shared_msg_clear(void);
