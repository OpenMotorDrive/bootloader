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
