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

struct hwinfo_s {
    char hw_name[81];
    uint8_t hw_major_version;
    uint8_t hw_minor_version;
} SHARED_MSG_PACKED;

struct boot_info_msg_s {
    uint8_t local_node_id;
    const struct hwinfo_s* hwinfo_ptr;
} SHARED_MSG_PACKED;

union shared_msg_payload_u {
    struct boot_msg_s boot_msg;
    struct firmwareupdate_msg_s firmwareupdate_msg;
    struct boot_info_msg_s boot_info_msg;
};

bool shared_msg_check_and_retreive(enum shared_msg_t* msgid, union shared_msg_payload_u* msg_payload);
void shared_msg_finalize_and_write(enum shared_msg_t msgid, const union shared_msg_payload_u* msg_payload);
void shared_msg_clear(void);
