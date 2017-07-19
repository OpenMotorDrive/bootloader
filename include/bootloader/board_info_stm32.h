// NOTE: this is an early attempt at a data structure to describe an STM32-based board's device tree

// enum shared_periph_cal_data_fmt_t {
//     SHARED_PERIPH_CAL_DATA_FMT_NO_DATA = 0,
//     SHARED_PERIPH_CAL_DATA_FMT_FLOAT = 1,
// };
//
// enum shared_periph_info_pin_function_t {
//     SHARED_PERIPH_INFO_PIN_FUNCTION_UNKNOWN = 0,
//     SHARED_PERIPH_INFO_PIN_FUNCTION_EXT_OSC,
//     SHARED_PERIPH_INFO_PIN_FUNCTION_CAN1_RX,
//     SHARED_PERIPH_INFO_PIN_FUNCTION_CAN1_TX,
//     SHARED_PERIPH_INFO_PIN_FUNCTION_CAN2_RX,
//     SHARED_PERIPH_INFO_PIN_FUNCTION_CAN2_TX,
//     SHARED_PERIPH_INFO_PIN_FUNCTION_CAN3_RX,
//     SHARED_PERIPH_INFO_PIN_FUNCTION_CAN3_TX,
//     SHARED_PERIPH_INFO_PIN_FUNCTION_SPI1_MISO = 21,
//     SHARED_PERIPH_INFO_PIN_FUNCTION_SPI1_MOSI,
//     SHARED_PERIPH_INFO_PIN_FUNCTION_SPI1_SCLK,
//     SHARED_PERIPH_INFO_PIN_FUNCTION_SPI1_NCS,
// };
//
// struct shared_onboard_periph_pin_info_s {
//     uint16_t function;
//     uint8_t port;
//     uint8_t pin;
// } SHARED_MSG_PACKED;
//
// struct shared_onboard_periph_info_s {
//     const char* const name;
//     uint8_t rev;
//     uint8_t bus_addr;
//     uint8_t num_pin_descs;
//     const struct shared_onboard_periph_pin_info_s* pin_descs;
//     uint8_t cal_data_fmt; // < 127 reserved for standard calibration data formats defined by enum shared_periph_cal_data_fmt_t
//     const void* const cal_data;
// } SHARED_MSG_PACKED;

// const struct shared_onboard_periph_info_s* shared_hwinfo_find_periph_info(const struct shared_hw_info_s* const hw_info, const char* const periph_name);
// const struct shared_onboard_periph_pin_info_s* shared_hwinfo_find_periph_pin_info(const struct shared_onboard_periph_info_s* periph_info, uint16_t pin_function);

// NOTE: .c file contents below

// const struct shared_onboard_periph_info_s* shared_hwinfo_find_periph_info(const struct shared_hw_info_s* hw_info, const char* periph_name) {
//     for (uint8_t i=0; i<hw_info->num_onboard_periph_descs; i++) {
//         if (!strcmp(hw_info->onboard_periph_descs[i].name, periph_name)) {
//             return &hw_info->onboard_periph_descs[i];
//         }
//     }
//     return 0;
// }
//
// const struct shared_onboard_periph_pin_info_s* shared_hwinfo_find_periph_pin_info(const struct shared_onboard_periph_info_s* periph_info, uint16_t pin_function) {
//     for (uint8_t i=0; i<periph_info->num_pin_descs; i++) {
//         if (periph_info->pin_descs[i].function == pin_function) {
//             return &periph_info->pin_descs[i];
//         }
//     }
//     return 0;
// }

// NOTE: example config file contents below

// enum {
//     DRV8305_PIN_FUNCTION_INHA = 2048,
//     DRV8305_PIN_FUNCTION_INHB,
//     DRV8305_PIN_FUNCTION_INHC,
//     DRV8305_PIN_FUNCTION_CSA_A,
//     DRV8305_PIN_FUNCTION_CSA_B,
//     DRV8305_PIN_FUNCTION_CSA_C,
//     VSENSE_PIN_FUNCTION_VSENSE,
// };
//
// #define LEN(x) (sizeof(x)/sizeof(x[0]))
//
// static const struct shared_onboard_periph_pin_info_s osc_pin_descs[] = {
//     {.function = SHARED_PERIPH_INFO_PIN_FUNCTION_EXT_OSC,   .port = 5, .pin = 0},  // PF0->CLK
// };
//
// static const struct shared_onboard_periph_pin_info_s can_pin_descs[] = {
//     {.function = SHARED_PERIPH_INFO_PIN_FUNCTION_CAN1_RX,   .port = 0, .pin = 11},  // PA11->CAN_RX
//     {.function = SHARED_PERIPH_INFO_PIN_FUNCTION_CAN1_TX,   .port = 0, .pin = 12},  // PA12->CAN_TX
// };
//
// static const struct shared_onboard_periph_pin_info_s vsense_pin_descs[] = {
//     {.function = VSENSE_PIN_FUNCTION_VSENSE,         .port = 0, .pin =  3}, // PA3->VSENSE
// };
//
// static const struct shared_onboard_periph_pin_info_s drv8305_pin_descs[] = {
//     {.function = SHARED_PERIPH_INFO_PIN_FUNCTION_SPI1_NCS,  .port = 1, .pin =  0}, // PB0->DRV8305_CS
//     {.function = SHARED_PERIPH_INFO_PIN_FUNCTION_SPI1_SCLK, .port = 1, .pin =  3}, // PB3->SCK
//     {.function = SHARED_PERIPH_INFO_PIN_FUNCTION_SPI1_MISO, .port = 1, .pin =  4}, // PB4->MISO
//     {.function = SHARED_PERIPH_INFO_PIN_FUNCTION_SPI1_MOSI, .port = 1, .pin =  5}, // PB5->MOSI
//     {.function = DRV8305_PIN_FUNCTION_INHA,          .port = 0, .pin =  8},  // PA8->PWM_A
//     {.function = DRV8305_PIN_FUNCTION_INHB,          .port = 0, .pin =  9},  // PA9->PWM_B
//     {.function = DRV8305_PIN_FUNCTION_INHC,          .port = 0, .pin = 10}, // PA10->PWM_C
//     {.function = DRV8305_PIN_FUNCTION_CSA_A,         .port = 0, .pin =  0}, // PA0->I_A
//     {.function = DRV8305_PIN_FUNCTION_CSA_B,         .port = 0, .pin =  1}, // PA1->I_B
//     {.function = DRV8305_PIN_FUNCTION_CSA_C,         .port = 0, .pin =  2}, // PA2->I_C
// };
//
// static const struct shared_onboard_periph_pin_info_s ma700_pin_descs[] = {
//     {.function = SHARED_PERIPH_INFO_PIN_FUNCTION_SPI1_NCS,  .port = 0, .pin =  5}, // PA5->MA700_CS
//     {.function = SHARED_PERIPH_INFO_PIN_FUNCTION_SPI1_SCLK, .port = 1, .pin =  3}, // PB3->SCK
//     {.function = SHARED_PERIPH_INFO_PIN_FUNCTION_SPI1_MISO, .port = 1, .pin =  4}, // PB4->MISO
//     {.function = SHARED_PERIPH_INFO_PIN_FUNCTION_SPI1_MOSI, .port = 1, .pin =  5}, // PB5->MOSI
// };
//
// static const float vsense_div = 20.0f;
// static const float csa_r = 0.001f;
// static const float osc_hz = 8e6f;
//
// static const struct shared_onboard_periph_info_s onboard_periph_descs[] = {
//     {
//         .name = "OSC_HSE",
//         .num_pin_descs = LEN(can_pin_descs),
//         .pin_descs = can_pin_descs,
//         .cal_data_fmt = SHARED_PERIPH_CAL_DATA_FMT_FLOAT,
//         .cal_data = &osc_hz
//     },
//     {
//         .name = "CAN1",
//         .num_pin_descs = LEN(can_pin_descs),
//         .pin_descs = can_pin_descs,
//     },
//     {
//         .name = "VSENSE",
//         .num_pin_descs = LEN(vsense_pin_descs),
//         .pin_descs = vsense_pin_descs,
//         .cal_data_fmt = SHARED_PERIPH_CAL_DATA_FMT_FLOAT,
//         .cal_data = &vsense_div
//     },
//     {
//         .name = "DRV8305",
//         .num_pin_descs = LEN(drv8305_pin_descs),
//         .pin_descs = drv8305_pin_descs,
//         .cal_data_fmt = SHARED_PERIPH_CAL_DATA_FMT_FLOAT,
//         .cal_data = &csa_r
//     },
//     {
//         .name = "MA700",
//         .num_pin_descs = LEN(ma700_pin_descs),
//         .pin_descs = ma700_pin_descs,
//     }
// };
