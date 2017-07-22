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
#include <string.h>
#include <bootloader/shared.h>
#include <stdlib.h>

#ifdef STM32F3
#define APP_PAGE_SIZE 2048
#endif

#define CANBUS_AUTOBAUD_SWITCH_INTERVAL_US 1000000
#define CANBUS_AUTOBAUD_TIMEOUT_US 10000000

struct app_header_s {
    uint32_t stacktop;
    uint32_t entrypoint;
};

// NOTE: _app_sec and _app_sec_end symbols shall be defined in the ld script
extern uint8_t _app_sec[], _app_sec_end;

// NOTE: _hw_info defined in the board config file
const struct shared_hw_info_s _hw_info = BOARD_CONFIG_HW_INFO_STRUCTURE;

static bool restart_req = false;
static uint32_t restart_req_us;

static struct {
    bool in_progress;
    uint32_t ofs;
    uint8_t transfer_id;
    uint8_t retries;
    uint32_t last_req_ms;
    uint8_t source_node_id;
    int32_t last_erased_page;
    char path[201];
} flash_state;

static struct {
    bool enable;
    uint32_t start_ms;
    uint32_t length_ms;
} boot_timer_state;

static enum shared_msg_t shared_msgid;
static union shared_msg_payload_u shared_msg;
static bool shared_msg_valid;

static struct {
    const struct shared_app_descriptor_s* shared_app_descriptor;
    uint64_t image_crc_computed;
    bool image_crc_correct;
} app_info;

static void write_data_to_flash(uint32_t ofs, const uint8_t* data, uint32_t data_len)
{
    for (uint16_t i=0; i<data_len; i+=sizeof(uint16_t)) {
        uint16_t* src_ptr = (uint16_t*)&data[i];
        uint16_t* dest_ptr = (uint16_t*)&_app_sec[i+ofs];

        flash_program_half_word(dest_ptr, src_ptr);
    }
}

static void start_boot_timer(uint32_t length_ms) {
    boot_timer_state.enable = true;
    boot_timer_state.start_ms = millis();
    boot_timer_state.length_ms = length_ms;
}

static bool check_and_start_boot_timer(void) {
    if (app_info.shared_app_descriptor && app_info.image_crc_correct && app_info.shared_app_descriptor->boot_delay_sec != 0) {
        start_boot_timer(((uint32_t)app_info.shared_app_descriptor->boot_delay_sec)*1000);
        return true;
    }
    return false;
}

static uint32_t get_app_sec_size(void) {
    return (uint32_t)&_app_sec_end - (uint32_t)&_app_sec[0];
}

static void update_uavcan_node_info_and_status(void)
{
    struct uavcan_node_info_s uavcan_node_info;
    memset(&uavcan_node_info, 0, sizeof(uavcan_node_info));
    uavcan_node_info.hw_name = _hw_info.hw_name;
    uavcan_node_info.hw_major_version = _hw_info.hw_major_version;
    uavcan_node_info.hw_minor_version = _hw_info.hw_minor_version;

    if (app_info.shared_app_descriptor && app_info.image_crc_correct) {
        uavcan_node_info.sw_major_version = app_info.shared_app_descriptor->major_version;
        uavcan_node_info.sw_minor_version = app_info.shared_app_descriptor->minor_version;
        uavcan_node_info.sw_vcs_commit_available = true;
        uavcan_node_info.sw_vcs_commit = app_info.shared_app_descriptor->vcs_commit;
        uavcan_node_info.sw_image_crc_available = true;
        uavcan_node_info.sw_image_crc = app_info.shared_app_descriptor->image_crc;
    }

    uavcan_set_node_info(uavcan_node_info);

    if (flash_state.in_progress) {
        uavcan_set_node_mode(UAVCAN_MODE_SOFTWARE_UPDATE);
        uavcan_set_node_health(UAVCAN_HEALTH_OK);
    } else {
        uavcan_set_node_mode(UAVCAN_MODE_MAINTENANCE);
        uavcan_set_node_health(app_info.image_crc_correct ? UAVCAN_HEALTH_OK : UAVCAN_HEALTH_CRITICAL);
    }
}

// call on change to flash memory
static void update_app_info(void)
{
    memset(&app_info, 0, sizeof(app_info));

    app_info.shared_app_descriptor = shared_find_app_descriptor(_app_sec, get_app_sec_size());

    const struct shared_app_descriptor_s* descriptor = app_info.shared_app_descriptor;

    if (descriptor && descriptor->image_size <= get_app_sec_size()) {
        uint32_t pre_crc_len = ((uint32_t)&descriptor->image_crc) - ((uint32_t)_app_sec);
        uint32_t post_crc_len = descriptor->image_size - pre_crc_len - sizeof(uint64_t);
        uint8_t* pre_crc_origin = _app_sec;
        uint8_t* post_crc_origin = (uint8_t*)((&descriptor->image_crc)+1);
        uint64_t zero64 = 0;

        app_info.image_crc_computed = shared_crc64_we(pre_crc_origin, pre_crc_len, 0);
        app_info.image_crc_computed = shared_crc64_we((uint8_t*)&zero64, sizeof(zero64), app_info.image_crc_computed);
        app_info.image_crc_computed = shared_crc64_we(post_crc_origin, post_crc_len, app_info.image_crc_computed);

        app_info.image_crc_correct = (app_info.image_crc_computed == descriptor->image_crc);
    }
}

static void corrupt_app(void) {
    for (uint8_t i=0; i<4; i++) {
        uint16_t src = 0;
        uint16_t* dest_ptr = (uint16_t*)&_app_sec[i*2];
        flash_program_half_word(dest_ptr, &src);
    }

    update_app_info();
}

static void check_and_boot(void)
{
    if (!app_info.image_crc_correct) {
        return;
    }

    union shared_msg_payload_u msg;
    msg.boot_msg.canbus_info.local_node_id = uavcan_get_node_id();

    if (canbus_get_confirmed_baudrate()) {
        msg.boot_msg.canbus_info.baudrate = canbus_get_confirmed_baudrate();
    } else if (shared_msg_valid && canbus_baudrate_valid(shared_msg.canbus_info.baudrate)) {
        msg.boot_msg.canbus_info.baudrate = shared_msg.canbus_info.baudrate;
    } else {
        msg.boot_msg.canbus_info.baudrate = 0;
    }

    shared_msg_finalize_and_write(SHARED_MSG_BOOT, &msg);

    scb_reset_system();
}

static void do_boot(void)
{
    union shared_msg_payload_u msg;

    if (shared_msg_valid) {
        msg.canbus_info = shared_msg.canbus_info;
    } else {
        memset(&shared_msg.canbus_info, 0, sizeof(shared_msg.canbus_info));
    }

    msg.boot_info_msg.hw_info = &_hw_info;

    shared_msg_finalize_and_write(SHARED_MSG_BOOT_INFO, &msg);

    struct app_header_s* app_header = (struct app_header_s*)_app_sec;

    // offset the vector table
    SCB_VTOR = (uint32_t)&(app_header->stacktop);

    asm volatile(
        "msr msp, %0	\n"
        "bx	%1	\n"
        : : "r"(app_header->stacktop), "r"(app_header->entrypoint) :);
}

static void erase_app_page(uint32_t page_num) {
    flash_erase_page(&_app_sec[page_num*APP_PAGE_SIZE]);
    flash_state.last_erased_page = page_num;
}

static bool restart_request_handler(void)
{
    restart_req = true;
    restart_req_us = micros();
    return true;
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
    memset(&flash_state, 0, sizeof(flash_state));
    corrupt_app();
}

static void begin_flash_from_path(uint8_t source_node_id, const char* path)
{
    boot_timer_state.enable = false;
    memset(&flash_state, 0, sizeof(flash_state));
    flash_state.in_progress = true;
    flash_state.ofs = 0;
    flash_state.source_node_id = source_node_id;
    strncpy(flash_state.path, path, 200);
    flash_state.transfer_id = uavcan_send_file_read_request(flash_state.source_node_id, flash_state.ofs, flash_state.path);
    flash_state.last_req_ms = millis();
    corrupt_app();
    flash_state.last_erased_page = -1;
}

// static void concat_int64_hex(char* dest, uint64_t val) {
//     char temp[10];
//     for(int8_t i=15; i>=0; i--) {
//         strcat(dest,itoa((val>>(4*i))&0xf,temp,16));
//     }
// }

static void uavcan_ready_handler(void) {
    if (shared_msg_valid && shared_msgid == SHARED_MSG_FIRMWAREUPDATE) {
        begin_flash_from_path(shared_msg.firmwareupdate_msg.source_node_id, shared_msg.firmwareupdate_msg.path);
    }

    check_and_start_boot_timer();
}

static void on_update_complete(void) {
    flash_state.in_progress = false;
    update_app_info();
    check_and_boot();
}

static void file_read_response_handler(uint8_t transfer_id, int16_t error, const uint8_t* data, uint16_t data_len, bool eof)
{
    if (flash_state.in_progress && transfer_id == flash_state.transfer_id) {
        if (error != 0 || flash_state.ofs+data_len > get_app_sec_size()) {
            do_fail_update();
            return;
        }

        int32_t curr_page = (flash_state.ofs+data_len)/APP_PAGE_SIZE;
        if (curr_page > flash_state.last_erased_page) {
            for (int32_t i=flash_state.last_erased_page+1; i<=curr_page; i++) {
                erase_app_page(i);
            }
        }

        write_data_to_flash(flash_state.ofs, data, data_len);

        if (eof) {
            on_update_complete();
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

    if (shared_msg_valid && shared_msgid == SHARED_MSG_BOOT) {
        do_boot();
    }
}

static void bootloader_init(void)
{
    init_clock();
    timing_init();

    check_and_start_boot_timer();
    update_app_info();

    uint32_t initial_canbus_baud;
    if (shared_msg_valid && canbus_baudrate_valid(shared_msg.canbus_info.baudrate)) {
        initial_canbus_baud = shared_msg.canbus_info.baudrate;
    } else if (app_info.shared_app_descriptor && canbus_baudrate_valid(app_info.shared_app_descriptor->canbus_baudrate)) {
        initial_canbus_baud = app_info.shared_app_descriptor->canbus_baudrate;
    } else {
        initial_canbus_baud = 1000000;
    }

    bool canbus_autobaud_enable;
    if (shared_msg_valid && canbus_baudrate_valid(shared_msg.canbus_info.baudrate)) {
        canbus_autobaud_enable = false;
    } else if (app_info.shared_app_descriptor && app_info.shared_app_descriptor->canbus_disable_auto_baud) {
        canbus_autobaud_enable = false;
    } else {
        canbus_autobaud_enable = true;
    }

    uint32_t canbus_baud = initial_canbus_baud;
    uint32_t autobaud_begin_us = micros();
    if (canbus_autobaud_enable) {
        struct canbus_autobaud_state_s autobaud_state;

        canbus_autobaud_start(&autobaud_state, canbus_baud, CANBUS_AUTOBAUD_SWITCH_INTERVAL_US);

        while (!(autobaud_state.success || micros()-autobaud_begin_us > CANBUS_AUTOBAUD_TIMEOUT_US)) {
            canbus_baud = canbus_autobaud_update(&autobaud_state);
        }

        if (!autobaud_state.success) {
            canbus_baud = initial_canbus_baud;
        }
    }

    canbus_init(canbus_baud, false);
    uavcan_init();

    uavcan_set_uavcan_ready_cb(uavcan_ready_handler);
    uavcan_set_restart_cb(restart_request_handler);
    uavcan_set_file_beginfirmwareupdate_cb(file_beginfirmwareupdate_handler);
    uavcan_set_file_read_response_cb(file_read_response_handler);
    update_uavcan_node_info_and_status();

    if (shared_msg_valid && shared_msg.canbus_info.local_node_id > 0 && shared_msg.canbus_info.local_node_id <= 127) {
        uavcan_set_node_id(shared_msg.canbus_info.local_node_id);
    } else if (app_info.shared_app_descriptor && app_info.shared_app_descriptor->canbus_local_node_id > 0 && app_info.shared_app_descriptor->canbus_local_node_id <= 127) {
        uavcan_set_node_id(app_info.shared_app_descriptor->canbus_local_node_id);
    }
}

static void bootloader_update(void)
{
    update_uavcan_node_info_and_status();
    uavcan_update();

    if (restart_req && (micros() - restart_req_us) > 1000) {
        // try to boot if image is valid
        check_and_boot();
        // otherwise, just reset
        scb_reset_system();
    }

    if (flash_state.in_progress) {
        if (millis()-flash_state.last_req_ms > 500) {
            do_resend_read_request();
            if (flash_state.retries > 10) { // retry for 5 seconds
                do_fail_update();
            }
        }
    } else {
        if (boot_timer_state.enable && millis()-boot_timer_state.start_ms > boot_timer_state.length_ms) {
            check_and_boot();
        }
    }
}

int main(void)
{
    bootloader_pre_init();
    bootloader_init();

    // main loop
    while(1) {
        bootloader_update();
    }

    return 0;
}
