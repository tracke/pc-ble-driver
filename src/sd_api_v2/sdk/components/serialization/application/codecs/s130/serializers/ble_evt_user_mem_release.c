/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of other
 *   contributors to this software may be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 *   4. This software must only be used in or with a processor manufactured by Nordic
 *   Semiconductor ASA, or in or with a processor manufactured by a third party that
 *   is used in combination with a processor manufactured by Nordic Semiconductor.
 *
 *   5. Any software provided in binary or object form under this license must not be
 *   reverse engineered, decompiled, modified and/or disassembled.
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

#include "ble_serialization.h"
#include "ble_struct_serialization.h"
#include "cond_field_serialization.h"
#include "app_util.h"
#include "ble_evt_app.h"
#include "app_ble_user_mem.h"

extern ser_ble_user_mem_t m_app_user_mem_table[];

uint32_t ble_evt_user_mem_release_dec(uint8_t const * const p_buf,
                                      uint32_t              packet_len,
                                      ble_evt_t * const     p_event,
                                      uint32_t * const      p_event_len)
{
    SER_ASSERT_NOT_NULL(p_buf);
    SER_ASSERT_NOT_NULL(p_event_len);

    uint32_t index        = 0;
    uint32_t err_code     = NRF_SUCCESS;

    uint32_t event_len = (uint16_t) (offsetof(ble_evt_t, evt.common_evt.params.user_mem_release)) +
                         sizeof (ble_evt_user_mem_release_t) -
                         sizeof (ble_evt_hdr_t);

    if (p_event == NULL)
    {
        *p_event_len = event_len;
        return NRF_SUCCESS;
    }

    p_event->header.evt_id  = BLE_EVT_USER_MEM_RELEASE;
    p_event->header.evt_len = event_len;
    ble_evt_user_mem_release_t * p_user_mem_rel = &(p_event->evt.common_evt.params.user_mem_release);

    err_code = uint16_t_dec(p_buf, packet_len, &index, &(p_event->evt.common_evt.conn_handle));
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);

    err_code = uint8_t_dec(p_buf, packet_len, &index, &(p_user_mem_rel->type));
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);

    // Decoding order of mem block is different than structure elements order - length is decoded first
    err_code = uint16_t_dec(p_buf, packet_len, &index, &(p_user_mem_rel->mem_block.len));
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);

    if (p_buf[index++] == SER_FIELD_PRESENT)
    {
        // Using connection handle find which mem block to release in Application Processor
        uint32_t user_mem_table_index;
        err_code = app_ble_user_mem_context_find(p_event->evt.common_evt.conn_handle, &user_mem_table_index);
        SER_ASSERT(err_code == NRF_SUCCESS, err_code);
        p_user_mem_rel->mem_block.p_mem = m_app_user_mem_table[user_mem_table_index].mem_block.p_mem;
    }
    else
    {
        p_user_mem_rel->mem_block.p_mem = NULL;
    }

    // Now user memory context can be released
    err_code = app_ble_user_mem_context_destroy(p_event->evt.common_evt.conn_handle);
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);

    SER_ASSERT_LENGTH_EQ(index, packet_len);
    *p_event_len = event_len;

    return err_code;
}
