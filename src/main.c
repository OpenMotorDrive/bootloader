/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stdbool.h>
#include "timing.h"
#include "init.h"
#include "can.h"
#include "uavcan.h"
#include "flash.h"
#include <libopencm3/cm3/scb.h>
#include "helpers.h"
#include <string.h>
#include "shared.h"

// TODO move into some config file
#define BOOT_DELAY_MS 3000
#define APP_PAGE_SIZE 1024

struct jump_info_s {
    uint32_t stacktop;
    uint32_t entrypoint;
};

// NOTE: _app_sec and _app_sec_end symbols shall be defined in the ld script
extern uint8_t _app_sec[], _app_sec_end;

static bool restart_req = false;
static uint32_t restart_req_us;

static void do_jump(uint32_t stacktop, uint32_t entrypoint)
{
    // offset the vector table
    SCB_VTOR = (uint32_t)&_app_sec;

    asm volatile(
        "msr msp, %0	\n"
        "bx	%1	\n"
        : : "r"(stacktop), "r"(entrypoint) :);

    for (;;) ;
}

static void do_boot(void) {
    union shared_msg_payload_u msg;
    // boot message is an empty struct, nothing to fill in
    shared_msg_finalize_and_write(SHARED_MSG_BOOT, &msg);
    scb_reset_system();
}

static struct {
    bool in_progress;
    uint8_t first_words[sizeof(struct jump_info_s)];
    uint32_t ofs;
    uint8_t transfer_id;
    uint8_t retries;
    uint32_t last_req_ms;
    uint8_t source_node_id;
    char path[201];
} flash_state;

static uint32_t boot_timer_start_ms;
static enum shared_msg_t shared_msgid;
static union shared_msg_payload_u shared_msg;
static bool shared_msg_valid;

static uint32_t get_app_sec_size(void) {
    return (uint32_t)&_app_sec_end - (uint32_t)&_app_sec[0];
}

static bool write_data_to_flash(uint32_t ofs, const uint8_t* data, uint32_t data_len)
{
    if (ofs%2 != 0 || data_len%2 != 0) {
        return false;
    }
    for (uint16_t i=0; i<data_len; i+=sizeof(uint16_t)) {
        uint16_t* src_ptr = (uint16_t*)&data[i];
        uint16_t* dest_ptr = (uint16_t*)&_app_sec[i+ofs];

        flash_program_half_word(dest_ptr, src_ptr);

        if (*dest_ptr != *src_ptr) {
            return false;
        }
    }
    return true;
}

static void erase_app_sec(void) {
    for (uint32_t i=0; i<get_app_sec_size(); i+=APP_PAGE_SIZE) {
        flash_erase_page(&_app_sec[i]);
    }
}

static bool restart_request_handler(void)
{
    restart_req = true;
    restart_req_us = micros();
    return true;
}

static bool app_is_valid(void) {
    struct jump_info_s* app_sec_jump_info = (struct jump_info_s*)_app_sec;
    return app_sec_jump_info->stacktop != 0xffffffff && app_sec_jump_info->entrypoint != 0xffffffff;
}

static void do_resend_read_request(void) {
    flash_state.transfer_id = uavcan_send_file_read_request(flash_state.source_node_id, flash_state.ofs, flash_state.path);
    flash_state.last_req_ms = millis();
    flash_state.retries++;
}

static void do_send_read_request(void) {
    do_resend_read_request();
    flash_state.retries = 0;
}

static void do_fail_update(void) {
    uavcan_set_node_mode(UAVCAN_MODE_INITIALIZATION);
    memset(&flash_state, 0, sizeof(flash_state));
    erase_app_sec();
}

static void do_finalize_flash(void) {
    if(!write_data_to_flash(0, flash_state.first_words, sizeof(flash_state.first_words))) {
        do_fail_update();
    }
}

static void begin_flash_from_path(uint8_t source_node_id, const char* path)
{
    uavcan_set_node_mode(UAVCAN_MODE_SOFTWARE_UPDATE);
    memset(&flash_state, 0, sizeof(flash_state));
    flash_state.in_progress = true;
    flash_state.ofs = 0;
    flash_state.source_node_id = source_node_id;
    strncpy(flash_state.path, path, 200);
    flash_state.transfer_id = uavcan_send_file_read_request(flash_state.source_node_id, flash_state.ofs, flash_state.path);
    flash_state.last_req_ms = millis();

    erase_app_sec();
}

static void uavcan_ready_handler(void) {
    boot_timer_start_ms = millis();

    if (shared_msg_valid && shared_msgid == SHARED_MSG_FIRMWAREUPDATE) {
        begin_flash_from_path(shared_msg.firmwareupdate_msg.source_node_id, shared_msg.firmwareupdate_msg.path);
    }
}

static void file_read_response_handler(uint8_t transfer_id, int16_t error, const uint8_t* data, uint16_t data_len, bool eof)
{
    if (flash_state.in_progress && transfer_id == flash_state.transfer_id) {
        if (error != 0 || flash_state.ofs+data_len > get_app_sec_size()) {
            do_fail_update();
            return;
        }

        if (flash_state.ofs == 0) {
            size_t first_words_size = sizeof(flash_state.first_words);
            memcpy(flash_state.first_words, data, first_words_size);
            if (!write_data_to_flash(flash_state.ofs+first_words_size, &data[first_words_size], data_len-first_words_size)) {
                do_fail_update();
            }
        } else {
            if (!write_data_to_flash(flash_state.ofs, data, data_len)) {
                do_fail_update();
            }
        }

        if (eof) {
            do_finalize_flash();
            do_boot();
        } else {
            flash_state.ofs += data_len;
            do_send_read_request();
        }
    }
}

static void file_beginfirmwareupdate_handler(struct uavcan_transfer_info_s transfer_info, uint8_t source_node_id, const char* path)
{
    if (!flash_state.in_progress) {
        if (source_node_id == 0) {
            source_node_id = transfer_info.remote_node_id;
        }

        uavcan_send_file_beginfirmwareupdate_response(&transfer_info, UAVCAN_BEGINFIRMWAREUPDATE_ERROR_OK, "");
        begin_flash_from_path(source_node_id, path);
    } else {
        uavcan_send_file_beginfirmwareupdate_response(&transfer_info, UAVCAN_BEGINFIRMWAREUPDATE_ERROR_IN_PROGRESS, "");
    }
}

static void bootloader_pre_init(void)
{
    // check for a valid shared message, jump immediately if it is a boot command
    shared_msg_valid = shared_msg_check_and_retreive(&shared_msgid, &shared_msg);
    shared_msg_clear();

    if (shared_msg_valid && shared_msgid == SHARED_MSG_BOOT && app_is_valid()) {
        struct jump_info_s* app_sec_jump_info = (struct jump_info_s*)_app_sec;
        do_jump(app_sec_jump_info->stacktop, app_sec_jump_info->entrypoint);
    }
}

static void bootloader_init(void)
{
    uavcan_set_uavcan_ready_cb(uavcan_ready_handler);
    uavcan_set_restart_cb(restart_request_handler);
    uavcan_set_file_beginfirmwareupdate_cb(file_beginfirmwareupdate_handler);
    uavcan_set_file_read_response_cb(file_read_response_handler);
    boot_timer_start_ms = millis();

    if (shared_msg_valid && shared_msgid == SHARED_MSG_FIRMWAREUPDATE && shared_msg.firmwareupdate_msg.local_node_id != 0) {
        uavcan_set_node_id(shared_msg.firmwareupdate_msg.local_node_id);
    }
}

static void bootloader_update(void)
{
    if (restart_req && (micros() - restart_req_us) > 1000) {
        scb_reset_system();
    }

    if (flash_state.in_progress) {
        if (millis()-flash_state.last_req_ms > 500) {
            do_resend_read_request();
        }
    } else {
        if (app_is_valid() && millis()-boot_timer_start_ms > BOOT_DELAY_MS) {
            do_boot();
        }
    }

}

int main(void)
{
    bootloader_pre_init();
    clock_init();
    timing_init();
    canbus_init();
    uavcan_init();
    bootloader_init();

    // main loop
    while(1) {
        uavcan_update();
        bootloader_update();
    }

    return 0;
}
