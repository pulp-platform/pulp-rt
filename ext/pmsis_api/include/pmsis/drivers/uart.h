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
#ifndef _PMSIS_UART_H_
#define _PMSIS_UART_H_

#include "pmsis/pmsis_types.h"

/*!
 * @addtogroup uart_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief UART driver version 1.0.0. */
/*@}*/


/*! @brief UART configuration structure. */

struct pi_uart_conf
{
    uint32_t src_clock_Hz;
    uint32_t baudrate_bps;
    uint32_t stop_bit_count; /*!< Number of stop bits, 1 stop bit (default) or 2 stop bits  */
    uint8_t parity_mode;
    uint8_t uart_id;
    uint8_t enable_rx;
    uint8_t enable_tx;
};

typedef struct pi_cl_uart_req_s pi_cl_uart_req_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* _cplusplus */


void pi_uart_conf_init(struct pi_uart_conf *conf);

int pi_uart_open(struct pi_device *device);

int pi_uart_write(struct pi_device *device, void *buffer, uint32_t size);

int pi_uart_write_async(struct pi_device *device, void *buffer, uint32_t size, pi_task_t* callback);

int pi_uart_read(struct pi_device *device, void *buffer, uint32_t size);

int pi_uart_read_async(struct pi_device *device, void *buffer, uint32_t size, pi_task_t* callback);

int pi_uart_write_byte(struct pi_device *device, uint8_t *byte);

int pi_uart_write_byte_async(struct pi_device *device, uint8_t *byte, pi_task_t* callback);

int pi_uart_read_byte(struct pi_device *device, uint8_t *byte);

void pi_uart_close(struct pi_device *device);


// cluster counterparts

int pi_cl_uart_write(pi_device_t *device, void *buffer, uint32_t size, pi_cl_uart_req_t *req);

int pi_cl_uart_write_byte(pi_device_t *device, uint8_t *byte, pi_cl_uart_req_t *req);

static inline void pi_cl_hyperram_write_wait(pi_cl_uart_req_t *req);

int pi_cl_uart_read(pi_device_t *device, void *addr, uint32_t size, pi_cl_uart_req_t *req);

int pi_cl_uart_read_byte(pi_device_t *device, uint8_t *byte, pi_cl_uart_req_t *req);

static inline void pi_cl_uart_read_wait(pi_cl_uart_req_t *req);



/*!
 * @brief Gets the default configuration structure.
 *
 * This function initializes the UART configuration structure to a default value.
 *
 * @param config Pointer to configuration structure.
 */
void pi_uart_conf_init(struct pi_uart_conf *conf);

/* @} */

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _GAP_UART_H_ */
