/**
 * Copyright 2022 Evgeniy Morozov
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE
*/

#ifndef ESTC_SERVICE_H__
#define ESTC_SERVICE_H__

#include <stdint.h>

#include "ble.h"
#include "sdk_errors.h"

#include "cli_utils.h"
// TODO: 1. Generate random BLE UUID (Version 4 UUID) and define it in the following format:
// #define ESTC_BASE_UUID { 0xF6, 0xCE, 0x0F, 0xC4, 0xCE, 0x9F, /* - */ 0xC3, 0x99, /* - */ 0xF7, 0x4D, /* - */ 0xDB, 0xB9, /* - */ 0x00, 0x00, 0xEC, 0x39 } // UUID: EC39xxxx-B9DB-4DF7-99C3-9FCEC40FCEF6
// bf488398-8f43-42ed-9165-4ebdba397e23
#define ESTC_BASE_UUID { 0x23, 0x7e, 0x39, 0xba, 0xbd, 0x4e, 0x65, 0x91, 0xed, 0x42, 0x43, 0x8f, 0x98, 0x83, 0x48, 0xbf }
// TODO: 2. Pick a random service 16-bit UUID and define it:
// #define ESTC_SERVICE_UUID 0xabcd
#define ESTC_SERVICE_UUID 0x230F

#define ESTC_GATT_CHAR_CURRENT_COLOR_UUID  0x5cd7
#define ESTC_GATT_CHAR_LED_POWER_STATE_UUID  0x5cd8
#define ESTC_GATT_CHAR_COMMAND_UUID  0x5cd9




typedef struct
{
    uint16_t type_handle;
    uint16_t service_handle;
    uint16_t connection_handle;
    ble_gatts_char_handles_t current_color_characteristic_handle;
    ble_gatts_char_handles_t led_power_state_characteristic_handle;
    ble_gatts_char_handles_t command_characteristic_handle;
} ble_estc_service_t;


typedef struct  {
    ble_gatts_char_handles_t* characteristic_handle;
    bool is_notification_enabled;
    bool is_indication_enabled;
} ble_characteristic_subscription_status_t;

typedef struct  {
    ble_estc_service_t* estc_service;
    ble_characteristic_subscription_status_t characteristics_subscription_status[3];
} ble_context_t;

void send_color_notification(void);
void send_power_state_notification(void);

volatile static bool m_is_processing; 

ret_code_t estc_ble_service_init(ble_estc_service_t *service, command_definition_t* known_commands, size_t known_commands_size,
                command_executor default_command, application_context_t* application_context, ble_context_t* ble_context);

void estc_ble_service_on_ble_event(const ble_evt_t *ble_evt, void *ctx);

void estc_process_command(ble_estc_service_t *service, const ble_gatts_evt_write_t *write);

//in current paradigm it's redundant
// void estc_update_led_power_state_characteristic_value(ble_estc_service_t *service, const ble_gatts_evt_write_t *write); 

#endif /* ESTC_SERVICE_H__ */