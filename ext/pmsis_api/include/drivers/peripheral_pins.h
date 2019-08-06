/*
 * Copyright (c) 2018, GreenWaves Technologies, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of GreenWaves Technologies, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __PMSIS_PERIPHERALPINS_H__
#define __PMSIS_PERIPHERALPINS_H__

#include "drivers/pin_map.h"
#include "chips/gap8/drivers/peripheral_names.h"

/************RTC***************/
extern const pin_map pin_map_RTC[];

/************I2C***************/
extern const pin_map pin_map_I2C_SDA[];
extern const pin_map pin_map_I2C_SCL[];

/************UART***************/
extern const pin_map pin_map_UART_TX[];
extern const pin_map pin_map_UART_RX[];
/************SPIM0-QPI***************/
extern const pin_map pin_map_SPIQ_SDIO2[];
extern const pin_map pin_map_SPIQ_SDIO3[];
/************SPIM1***************/
extern const pin_map pin_map_SPI_SCLK[];
extern const pin_map pin_map_SPI_MOSI[];
extern const pin_map pin_map_SPI_MISO[];
extern const pin_map pin_map_SPI_SSEL[];
/************SPIS***************/
extern const pin_map pin_map_SPIS_SDIO0[];
extern const pin_map pin_map_SPIS_SDIO1[];
extern const pin_map pin_map_SPIS_SDIO2[];
extern const pin_map pin_map_SPIS_SDIO3[];
extern const pin_map pin_map_SPIS_CSN[];
extern const pin_map pin_map_SPIS_CLK[];

/************PWM***************/
extern const pin_map pin_map_PWM[];

/************HYPERBUS***************/
extern const pin_map pin_map_HYPERBUS_CLK[];
extern const pin_map pin_map_HYPERBUS_CLKN[];
extern const pin_map pin_map_HYPERBUS_DQ0[];
extern const pin_map pin_map_HYPERBUS_DQ1[];
extern const pin_map pin_map_HYPERBUS_DQ2[];
extern const pin_map pin_map_HYPERBUS_DQ3[];
extern const pin_map pin_map_HYPERBUS_DQ4[];
extern const pin_map pin_map_HYPERBUS_DQ5[];
extern const pin_map pin_map_HYPERBUS_DQ6[];
extern const pin_map pin_map_HYPERBUS_DQ7[];
extern const pin_map pin_map_HYPERBUS_CSN0[];
extern const pin_map pin_map_HYPERBUS_CSN1[];
extern const pin_map pin_map_HYPERBUS_RWDS[];
/************LVDS***************/
extern const pin_map pin_map_LVDS_TXD_P[];
extern const pin_map pin_map_LVDS_TXD_N[];
extern const pin_map pin_map_LVDS_TXCLK_P[];
extern const pin_map pin_map_LVDS_TXCLK_N[];
extern const pin_map pin_map_LVDS_RXD_P[];
extern const pin_map pin_map_LVDS_RXD_N[];
extern const pin_map pin_map_LVDS_RXCLK_P[];
extern const pin_map pin_map_LVDS_RXCLK_N[];
/************ORCA***************/
extern const pin_map pin_map_ORCA_TXSYNC[];
extern const pin_map pin_map_ORCA_RXSYNC[];
extern const pin_map pin_map_ORCA_TXI[];
extern const pin_map pin_map_ORCA_TXQ[];
extern const pin_map pin_map_ORCA_RXI[];
extern const pin_map pin_map_ORCA_RXQ[];
/************CPI***************/
extern const pin_map pin_map_CPI_PCLK[];
extern const pin_map pin_map_CPI_HSYNC[];
extern const pin_map pin_map_CPI_DATA0[];
extern const pin_map pin_map_CPI_DATA1[];
extern const pin_map pin_map_CPI_DATA2[];
extern const pin_map pin_map_CPI_DATA3[];
extern const pin_map pin_map_CPI_DATA4[];
extern const pin_map pin_map_CPI_DATA5[];
extern const pin_map pin_map_CPI_DATA6[];
extern const pin_map pin_map_CPI_DATA7[];
extern const pin_map pin_map_CPI_VSYNC[];
/************I2S***************/
extern const pin_map pin_map_I2S_SCK[];
extern const pin_map pin_map_I2S_WS[];
extern const pin_map pin_map_I2S_SDI[];

#endif
