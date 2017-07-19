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

#include "init.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <bootloader/shared.h>

void clock_init_stm32f302k8_8mhz_hse(void)
{
    rcc_osc_on(RCC_HSE);
    rcc_wait_for_osc_ready(RCC_HSE);
    rcc_set_sysclk_source(RCC_CFGR_SW_HSE);
    rcc_wait_for_sysclk_status(RCC_HSE);

    rcc_osc_off(RCC_PLL);
    rcc_wait_for_osc_not_ready(RCC_PLL);
    rcc_set_prediv(RCC_CFGR2_PREDIV_NODIV); // 8 Mhz
    rcc_set_pll_source(RCC_CFGR_PLLSRC_HSE_PREDIV);
    rcc_set_pll_multiplier(RCC_CFGR_PLLMUL_PLL_IN_CLK_X9); // 72 MHz

    rcc_set_hpre(RCC_CFGR_HPRE_DIV_NONE); // 72 MHz
    rcc_ahb_frequency = 72000000;

    rcc_set_ppre1(RCC_CFGR_PPRE1_DIV_2); // 36 MHz
    rcc_apb1_frequency = 36000000;

    rcc_set_ppre2(RCC_CFGR_PPRE2_DIV_NONE); // 72 MHz
    rcc_apb2_frequency = 72000000;

    rcc_osc_on(RCC_PLL);
    rcc_wait_for_osc_ready(RCC_PLL);
    flash_set_ws(FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2WS);
    rcc_set_sysclk_source(RCC_CFGR_SW_PLL);
    rcc_wait_for_sysclk_status(RCC_PLL);
}

// bool init_gpio(const struct shared_onboard_periph_pin_info_s* pin_info) {
//     uint32_t rcc_gpio_port_identifier = 0;
//     uint32_t gpio_port_identifier = 0;
//     switch(pin_info->port) {
//         case 0:
//             rcc_gpio_port_identifier = RCC_GPIOA;
//             gpio_port_identifier = GPIOA;
//             break;
//         case 1:
//             rcc_gpio_port_identifier = RCC_GPIOB;
//             gpio_port_identifier = GPIOB;
//             break;
//         case 2:
//             rcc_gpio_port_identifier = RCC_GPIOC;
//             gpio_port_identifier = GPIOC;
//             break;
//         case 3:
//             rcc_gpio_port_identifier = RCC_GPIOD;
//             gpio_port_identifier = GPIOD;
//             break;
//         case 4:
//             rcc_gpio_port_identifier = RCC_GPIOE;
//             gpio_port_identifier = GPIOE;
//             break;
//         case 5:
//             rcc_gpio_port_identifier = RCC_GPIOF;
//             gpio_port_identifier = GPIOF;
//             break;
//         case 6:
//             rcc_gpio_port_identifier = RCC_GPIOG;
//             gpio_port_identifier = GPIOG;
//             break;
//         case 7:
//             rcc_gpio_port_identifier = RCC_GPIOH;
//             gpio_port_identifier = GPIOH;
//             break;
//         default:
//             return false;
//     }
//
//     rcc_periph_clock_enable(rcc_gpio_port_identifier);
//     gpio_mode_setup(gpio_port_identifier, GPIO_MODE_AF, GPIO_PUPD_NONE, (1<<rx_pin)|(1<<tx_pin));
//     gpio_set_af(gpio_port_identifier, GPIO_AF9, (1<<rx_pin)|(1<<tx_pin));
//
//     return true;
// }
