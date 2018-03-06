#pragma once

#include <stdint.h>

#ifndef APP_DESCRIPTOR_ALIGNED_AND_PACKED
#define APP_DESCRIPTOR_ALIGNED_AND_PACKED __attribute__((aligned(8),packed))
#endif

#ifndef APP_DESCRIPTOR_PACKED
#define APP_DESCRIPTOR_PACKED __attribute__((packed))
#endif

#define SHARED_APP_DESCRIPTOR_SIGNATURE "\x40\xa2\xe4\xf1\x64\x68\x91\x06"

#define SHARED_APP_PARAMETERS_FMT 2

struct shared_app_parameters_s {
    uint8_t boot_delay_sec;
    uint32_t canbus_baudrate : 32;
    uint8_t canbus_local_node_id;
} APP_DESCRIPTOR_PACKED;

struct shared_app_descriptor_s {
    char signature[8];
    uint64_t image_crc;
    uint32_t image_size;
    uint32_t vcs_commit;
    uint8_t major_version;
    uint8_t minor_version;
    uint8_t parameters_fmt:7;
    void* param_search_addr;
    uint32_t param_search_len;
} APP_DESCRIPTOR_ALIGNED_AND_PACKED;

const struct shared_app_descriptor_s* shared_find_app_descriptor(uint8_t* buf, uint32_t buf_len);
const struct shared_app_parameters_s* shared_get_parameters(const struct shared_app_descriptor_s* descriptor);
