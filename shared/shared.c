#include <bootloader/shared.h>

#include <string.h>

#define SHARED_MSG_MAGIC 0xDEADBEEF

struct shared_msg_header_s {
    uint64_t crc64;
    uint32_t magic;
    uint8_t msgid;
} SHARED_MSG_PACKED;

struct shared_msg_s {
    struct shared_msg_header_s header;
    union shared_msg_payload_u payload;
} SHARED_MSG_PACKED;

// NOTE: _app_bl_shared_sec symbol shall be defined by the ld script
extern struct shared_msg_s _app_bl_shared_sec;

static int16_t get_payload_length(enum shared_msg_t msgid) {
    switch(msgid) {
        case SHARED_MSG_BOOT:
            return sizeof(struct shared_boot_msg_s);
        case SHARED_MSG_FIRMWAREUPDATE:
            return sizeof(struct shared_firmwareupdate_msg_s);
        case SHARED_MSG_BOOT_INFO:
            return sizeof(struct shared_boot_info_msg_s);
    };

    return -1;
}

static uint32_t compute_mailbox_crc64(int16_t payload_len) {
    return shared_crc64_we((uint8_t*)(&_app_bl_shared_sec.header.crc64+1), sizeof(_app_bl_shared_sec.header)+payload_len-sizeof(uint64_t), 0);
}

static bool mailbox_valid(void) {
    if (_app_bl_shared_sec.header.magic != SHARED_MSG_MAGIC) {
        return false;
    }

    int16_t payload_len = get_payload_length((enum shared_msg_t)_app_bl_shared_sec.header.msgid);
    if (payload_len == -1) {
        return false;
    }

    if (compute_mailbox_crc64(payload_len) != _app_bl_shared_sec.header.crc64) {
        return false;
    }

    return true;
}

bool shared_msg_check_and_retreive(enum shared_msg_t* msgid, union shared_msg_payload_u* msg_payload) {
    if (!mailbox_valid()) {
        return false;
    }

    *msgid = (enum shared_msg_t)_app_bl_shared_sec.header.msgid;
    *msg_payload = _app_bl_shared_sec.payload;
    return true;
}

void shared_msg_finalize_and_write(enum shared_msg_t msgid, const union shared_msg_payload_u* msg_payload) {
    _app_bl_shared_sec.header.msgid = (uint8_t)msgid;
    memcpy(&_app_bl_shared_sec.payload, msg_payload, sizeof(union shared_msg_payload_u));
    _app_bl_shared_sec.header.magic = SHARED_MSG_MAGIC;
    _app_bl_shared_sec.header.crc64 = compute_mailbox_crc64(get_payload_length(msgid));
}

void shared_msg_clear(void) {
    memset(&_app_bl_shared_sec, 0, sizeof(_app_bl_shared_sec));
}

const struct shared_app_descriptor_s* shared_find_app_descriptor(uint8_t* buf, uint32_t buf_len)
{
    for (uint32_t i=0; i<buf_len-sizeof(struct shared_app_descriptor_s); i+=8) {
        struct shared_app_descriptor_s* app_descriptor = (struct shared_app_descriptor_s*)&buf[i];

        if (!memcmp(&app_descriptor->signature, SHARED_APP_DESCRIPTOR_SIGNATURE, sizeof(app_descriptor->signature))) {
            return app_descriptor;
        }
    }

    return 0;
}

const struct shared_onboard_periph_info_s* shared_hwinfo_find_periph_info(const struct shared_hw_info_s* hw_info, const char* periph_name) {
    for (uint8_t i=0; i<hw_info->num_onboard_periph_descs; i++) {
        if (!strcmp(hw_info->onboard_periph_descs[i].name, periph_name)) {
            return &hw_info->onboard_periph_descs[i];
        }
    }
    return 0;
}

const struct shared_onboard_periph_pin_info_s* shared_hwinfo_find_periph_pin_info(const struct shared_onboard_periph_info_s* periph_info, uint16_t pin_function) {
    for (uint8_t i=0; i<periph_info->num_pin_descs; i++) {
        if (periph_info->pin_descs[i].function == pin_function) {
            return &periph_info->pin_descs[i];
        }
    }
    return 0;
}

uint64_t shared_crc64_we(const uint8_t *buf, uint32_t len, uint64_t crc)
{
    uint32_t i;
    uint8_t j;

    crc = ~crc;

    for (i = 0; i < len; i++) {
        crc ^= ((uint64_t)buf[i]) << 56;
        for (j = 0; j < 8; j++) {
            crc = (crc & (1ULL<<63)) ? (crc<<1)^0x42F0E1EBA9EA3693ULL : (crc<<1);
        }
    }

    return ~crc;
}
