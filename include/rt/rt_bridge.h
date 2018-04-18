/*
 * Copyright (C) 2018 ETH Zurich and University of Bologna
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Copyright (C) 2018 GreenWaves Technologies
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __RT_RT_BRIDGE_H__
#define __RT_RT_BRIDGE_H__


#include <stdint.h>
#include "rt/data/rt_data_bridge.h"

/**
* @ingroup groupKernel
*/

/**
 * @defgroup bridge External bridge
 *
 * This API provides support for interacting with an external bridge for example to
 * access files on the workstation.
 *
 */

/**        
 * @addtogroup bridge
 * @{        
 */

/**@{*/



/** \brief Connect to the external bridge.
 *
 * This must be called before calling any other function of this API in order to 
 * connect the runtime to the external bridge.
 * The specified event will terminate only when the runtime has established the
 * connection with the external bridge.
 * Can only be called from fabric controller.
 * This operation is asynchronous and its termination can be managed through an event.
 *
 * \param event     The event used for managing termination.
 */
void rt_bridge_connect(rt_event_t *event);



/** \brief Disconnect from the external bridge.
 *
 * This can be called close down the connection with the external bridge.
 * No other function from this API can be called after this one is called unless
 * the connection is opened again.
 * Can only be called from fabric controller.
 * This operation is asynchronous and its termination can be managed through an event.
 *
 * \param event     The event used for managing termination.
 */
void rt_bridge_disconnect(rt_event_t *event);



/** \brief Open a file on the workstation.
 *
 * This can be called to ask the debug bridge to open a file on the workstation.
 * Can only be called from fabric controller.
 * This operation is asynchronous and its termination can be managed through an event.
 *
 * \param name      The path to the file on the workstation. If it is a relative path, it will be relative to the platform working directory.
 * \param flags     The access modes, see the documentation of open on the workstation for more details.
 * \param mode      Access rights for file creation, see the documentation of open on the workstation for more details.
 * \param event     The event used for managing termination.
 * \return          If the event parameter is NULL, this returns the return value of the call to open, otherwise it is undefined.
 */
int rt_bridge_open(const char* name, int flags, int mode, rt_event_t *event);



/** \brief Wait for file open termination.
 *
 * This can be called to block the execution until the file opening associated to the specified event is finished.
 * Can only be called from fabric controller.
 *
 * \param event     The event specified when the file was openened.
 * \return          This returns the return value of the call to open.
 */
int rt_bridge_open_wait(rt_event_t *event);



/** \brief Close a file on the workstation.
 *
 * This can be called to ask the debug bridge to close a file on the workstation.
 * Can only be called from fabric controller.
 * This operation is asynchronous and its termination can be managed through an event.
 *
 * \param file      The file descriptor returned when the file was opened.
 * \param event     The event used for managing termination.
 * \return          If the event parameter is NULL, this returns the return value of the call to close, otherwise it is undefined.
 */
int rt_bridge_close(int file, rt_event_t *event);



/** \brief Wait for file close termination.
 *
 * This can be called to block the execution until the file closing associated to the specified event is finished.
 * Can only be called from fabric controller.
 *
 * \param event     The event specified when the file was closed.
 * \return          This returns the return value of the call to close.
 */
int rt_bridge_close_wait(rt_event_t *event);



/** \brief Read from a file on the workstation.
 *
 * This can be called to ask the debug bridge to read data from a file on the workstation.
 * Can only be called from fabric controller.
 * This operation is asynchronous and its termination can be managed through an event.
 *
 * \param file      The file descriptor returned when the file was opened.
 * \param ptr       The address in the chip where the data read from the file must be written.
 * \param len       The number of bytes which must be read from the file.
 * \param event     The event used for managing termination.
 * \return          If the event parameter is NULL, this returns the return value of the call to read, otherwise it is undefined.
 */
int rt_bridge_read(int file, void* ptr, int len, rt_event_t *event);



/** \brief Wait for file read termination.
 *
 * This can be called to block the execution until the file reading associated to the specified event is finished.
 * Can only be called from fabric controller.
 *
 * \param event     The event specified when the file was read.
 * \return          This returns the return value of the call to read.
 */
int rt_bridge_read_wait(rt_event_t *event);



/** \brief Write to a file on the workstation.
 *
 * This can be called to ask the debug bridge to write data to a file on the workstation.
 * Can only be called from fabric controller.
 * This operation is asynchronous and its termination can be managed through an event.
 *
 * \param file      The file descriptor returned when the file was opened.
 * \param ptr       The address in the chip where the data to be written to the file must be read.
 * \param len       The number of bytes which must be written to the file.
 * \param event     The event used for managing termination.
 * \return          If the event parameter is NULL, this returns the return value of the call to write, otherwise it is undefined.
 */
int rt_bridge_write(int file, void* ptr, int len, rt_event_t *event);



/** \brief Wait for file write termination.
 *
 * This can be called to block the execution until the file writing associated to the specified event is finished.
 * Can only be called from fabric controller.
 *
 * \param event     The event specified when the file was written.
 * \return          This returns the return value of the call to write.
 */
int rt_bridge_write_wait(rt_event_t *event);



//!@}

/**        
 * @} end of bridge group        
 */

#endif