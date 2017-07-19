#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifndef SHARED_MSG_PACKED
#define SHARED_MSG_PACKED __attribute__((packed))
#endif

#ifndef APP_DESCRIPTOR_ALIGNED_AND_PACKED
#define APP_DESCRIPTOR_ALIGNED_AND_PACKED __attribute__((aligned(8),packed))
#endif

enum shared_msg_t {
    SHARED_MSG_BOOT = 0,
    SHARED_MSG_FIRMWAREUPDATE = 1,
    SHARED_MSG_BOOT_INFO = 2
};

struct shared_canbus_info_s {
    uint8_t local_node_id;
    int32_t baudrate;
} SHARED_MSG_PACKED;

struct shared_boot_msg_s {
    struct shared_canbus_info_s canbus_info;
}  SHARED_MSG_PACKED;

struct shared_firmwareupdate_msg_s {
    struct shared_canbus_info_s canbus_info;
    uint8_t source_node_id;
    char path[201];
} SHARED_MSG_PACKED;

struct shared_hw_info_s {
    const char* const hw_name;
    uint8_t hw_major_version;
    uint8_t hw_minor_version;
    uint8_t board_desc_fmt;
    const void* const board_desc;
} SHARED_MSG_PACKED;

struct shared_boot_info_msg_s {
    struct shared_canbus_info_s canbus_info;
    const struct shared_hw_info_s* hw_info;
} SHARED_MSG_PACKED;

union shared_msg_payload_u {
    struct shared_boot_msg_s boot_msg;
    struct shared_firmwareupdate_msg_s firmwareupdate_msg;
    struct shared_boot_info_msg_s boot_info_msg;
};

#define SHARED_APP_DESCRIPTOR_SIGNATURE "\xd7\xe4\xf7\xba\xd0\x0f\x9b\xee"

struct shared_app_descriptor_s {
    char signature[8];
    uint64_t image_crc;
    uint32_t image_size;
    uint32_t vcs_commit;
    uint8_t major_version;
    uint8_t minor_version;
    uint8_t boot_delay_sec;
} APP_DESCRIPTOR_ALIGNED_AND_PACKED;

bool shared_msg_check_and_retreive(enum shared_msg_t* msgid, union shared_msg_payload_u* msg_payload);
void shared_msg_finalize_and_write(enum shared_msg_t msgid, const union shared_msg_payload_u* msg_payload);
void shared_msg_clear(void);

const struct shared_app_descriptor_s* shared_find_app_descriptor(uint8_t* buf, uint32_t buf_len);

uint64_t shared_crc64_we(const uint8_t *buf, uint32_t len, uint64_t crc);
