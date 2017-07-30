#pragma once

#include <stdbool.h>
#include <stdint.h>

// TODO: split this file up
// message sharing section

#ifndef SHARED_MSG_PACKED
#define SHARED_MSG_PACKED __attribute__((packed))
#endif

enum shared_msg_t {
    SHARED_MSG_BOOT = 0,
    SHARED_MSG_FIRMWAREUPDATE = 1,
    SHARED_MSG_BOOT_INFO = 2,
    SHARED_MSG_CANBUS_INFO = 3
};

struct shared_canbus_info_s {
    uint32_t baudrate;
    uint8_t local_node_id;
} SHARED_MSG_PACKED;

enum shared_hw_info_board_desc_fmt_t {
    SHARED_HW_INFO_BOARD_DESC_FMT_NONE = 0
};

enum shared_boot_reason_t {
    SHARED_BOOT_REASON_TIMEOUT = 0,
    SHARED_BOOT_REASON_REBOOT_COMMAND = 1,
    SHARED_BOOT_REASON_APPLICATION_COMMAND = 2,
    SHARED_BOOT_REASON_FIRMWARE_UPDATE = 3,
};

struct shared_hw_info_s {
    const char* const hw_name;
    uint8_t hw_major_version;
    uint8_t hw_minor_version;
    uint8_t board_desc_fmt;
    const void* const board_desc;
} SHARED_MSG_PACKED;

struct shared_boot_msg_s {
    struct shared_canbus_info_s canbus_info;
    uint8_t boot_reason; // >= 127 are vendor/application-specific codes
}  SHARED_MSG_PACKED;

struct shared_firmwareupdate_msg_s {
    struct shared_canbus_info_s canbus_info;
    uint8_t source_node_id;
    char path[201];
} SHARED_MSG_PACKED;

struct shared_boot_info_msg_s {
    struct shared_canbus_info_s canbus_info;
    const struct shared_hw_info_s* hw_info;
    uint8_t boot_reason; // >= 127 are vendor/application-specific codes
} SHARED_MSG_PACKED;

union shared_msg_payload_u {
    struct shared_boot_msg_s boot_msg;
    struct shared_firmwareupdate_msg_s firmwareupdate_msg;
    struct shared_boot_info_msg_s boot_info_msg;
    struct shared_canbus_info_s canbus_info;
};

bool shared_msg_check_and_retreive(enum shared_msg_t* msgid, union shared_msg_payload_u* msg_payload);
void shared_msg_finalize_and_write(enum shared_msg_t msgid, const union shared_msg_payload_u* msg_payload);
void shared_msg_clear(void);


// app descriptor section

#ifndef APP_DESCRIPTOR_ALIGNED_AND_PACKED
#define APP_DESCRIPTOR_ALIGNED_AND_PACKED __attribute__((aligned(8),packed))
#endif

#ifndef APP_DESCRIPTOR_PACKED
#define APP_DESCRIPTOR_PACKED __attribute__((packed))
#endif

#define SHARED_APP_DESCRIPTOR_SIGNATURE "\x40\xa2\xe4\xf1\x64\x68\x91\x06"

#define SHARED_APP_PARAMETERS_FMT 1

struct shared_app_parameters_s {
    // this index is incremented on every param write - if two param structure pointers are provided,
    // use a signed integer comparison to determine the most recently written structure
    uint8_t param_idx;
    uint8_t boot_delay_sec;
    uint32_t canbus_disable_auto_baud : 1;
    uint32_t canbus_baudrate : 31;
    uint8_t canbus_local_node_id;
    uint64_t crc64;
} APP_DESCRIPTOR_PACKED;

struct shared_app_descriptor_s {
    char signature[8];
    uint64_t image_crc;
    uint32_t image_size;
    uint32_t vcs_commit;
    uint8_t major_version;
    uint8_t minor_version;
    uint8_t parameters_fmt:7;
    uint8_t parameters_ignore_crc64:1;
    const struct shared_app_parameters_s* parameters[2];
} APP_DESCRIPTOR_ALIGNED_AND_PACKED;

const void* shared_find_marker(uint64_t marker, uint8_t* buf, uint32_t buf_len);
const struct shared_app_descriptor_s* shared_find_app_descriptor(uint8_t* buf, uint32_t buf_len);
const struct shared_app_parameters_s* shared_get_parameters(const struct shared_app_descriptor_s* descriptor);

// crc section

uint64_t shared_crc64_we(const uint8_t *buf, uint32_t len, uint64_t crc);
