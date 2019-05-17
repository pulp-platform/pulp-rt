/*
 * Copyright (c) 2019, GreenWaves Technologies, Inc.
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

#ifndef _GAP_MALLOC_H_
#define _GAP_MALLOC_H_

/*!
 * @addtogroup pmsis_malloc
 * @{
 */


/*******************************************************************************
 * APIs
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*!
 * @brief Initialize the pmsis_malloc.
 *
 * This function initializes the FC and L2 memory allocators to allocate
 * from the FC or L2 memory heap.
 *
 * @note If this function is called first, those subfunctions do not need to be called.
 */
void pmsis_malloc_init(void *fc_heap_start, uint32_t fc_heap_size,
        void *l2_heap_start, uint32_t l2_heap_size);

/*!
 * @brief Allocate memory from FC or L2 memory allocator.
 *
 * This function allocates a memory chunk in FC if there is enough memory to allocate
 * required chunk of memory otherwise in L2.
 *
 * @param size   Size of the memory to be allocated.
 *
 * @return Pointer to an allocated memory chunk or NULL if there is not enough memory to allocate.
 */
void *pmsis_malloc(size_t size);

/*!
 * @brief Allocate memory from FC or L2 memory allocator with aligned address.
 *
 * This function allocates an adress aligned memory chunk in FC if there is
 * enough memory to allocate required chunk of memory otherwise in L2.
 *
 * @param size   Size of the memory to be allocated.
 * @param align  Memory alignement size.
 *
 * @return Pointer to an allocated memory chunk or NULL if there is not enough memory to allocate.
 */
void *pmsis_malloc_align(size_t size, uint32_t align);

/*!
 * @brief Free an allocated memory chunk.
 *
 * This function frees an allocated memory chunk.
 *
 * @param _chunk Start address of an allocated memory chunk.
 */
void pmsis_malloc_free(void *_chunk);

/*!
 * @brief Display allocated blocs.
 *
 * This function displays allocated blocs in either FC or L2 memory.
 */
void pmsis_malloc_display(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/* @} */

#endif /*_GAP_MALLOC_H_*/
