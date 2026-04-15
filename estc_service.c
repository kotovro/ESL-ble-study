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
#include "app_error.h"
#include "nrf_log.h"
#include "app_timer.h"
#include "nrfx_clock.h"
#include "ble.h"
#include "ble_gatts.h"
#include "ble_srv_common.h"

#include "estc_service.h"


APP_TIMER_DEF(processing_start_timer);

typedef struct {
    uint8_t command[MAX_BLE_COMMAND_LENGTH];
    uint8_t status;
} RESPONSE;

static uint8_t m_response_len;
static command_definition_t* m_command_definitions;
static command_executor m_default_executor;
static size_t m_command_definitions_size;
static application_context_t* m_application_context;
static RESPONSE m_response_data; 
static RESPONSE m_current_command_data; 
static ble_estc_service_t* m_service_instance;
static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service);
static ret_code_t estc_ble_add_current_color_characteristic(ble_estc_service_t *service);
static ret_code_t estc_ble_add_led_power_state_characteristic(ble_estc_service_t *service);
static ret_code_t estc_ble_add_command_characteristic(ble_estc_service_t *service);
static ble_context_t* m_ble_context;


void send_notification(uint16_t conn_handle, ble_gatts_char_handles_t* char_handle, uint8_t *data, uint16_t data_len)
{
    if (m_service_instance->connection_handle == BLE_CONN_HANDLE_INVALID) return;
    NRF_LOG_INFO("Sending notification, data len: %d", data_len);
    for (int i = 0; i < 3; ++i)
    {
        if (m_ble_context->characteristics_subscription_status[i].characteristic_handle == char_handle)
        { 
            NRF_LOG_INFO("Notifications for charactristic enabled: %d", m_ble_context->characteristics_subscription_status[i].is_notification_enabled);
            if (m_ble_context->characteristics_subscription_status[i].is_notification_enabled)
            {
                NRF_LOG_INFO("Sending notification, data len: %d", data_len);
                ble_gatts_hvx_params_t params;
                memset(&params, 0, sizeof(params));
                params.type = BLE_GATT_HVX_NOTIFICATION;
                params.handle = char_handle->value_handle;
                params.p_data = data;
                params.p_len = &data_len;

                ret_code_t err_code = sd_ble_gatts_hvx(conn_handle, &params);
                APP_ERROR_CHECK(err_code);
            }
            if (m_ble_context->characteristics_subscription_status[i].is_indication_enabled)
            {
                ble_gatts_hvx_params_t params;
                memset(&params, 0, sizeof(params));
                params.type = BLE_GATT_HVX_INDICATION;
                params.handle = char_handle->value_handle;
                params.p_data = data;
                params.p_len = &data_len;

                ret_code_t err_code = sd_ble_gatts_hvx(conn_handle, &params);
                APP_ERROR_CHECK(err_code);
            }
        }
    }
}

void send_color_notification(void)
{
    NRF_LOG_INFO("The service is null: %d", m_service_instance == NULL);
    NRF_LOG_INFO("Connection handle is: %d, while invalid is: %d", m_service_instance->connection_handle, BLE_CONN_HANDLE_INVALID);
    send_notification(
        m_service_instance->connection_handle,
        &m_service_instance->current_color_characteristic_handle,
        (uint8_t*)m_application_context->current_color_description,
        sizeof(COLOR_DESCRIPTION));
}

void send_power_state_notification(void)
{
    send_notification(m_service_instance->connection_handle,
        &m_service_instance->led_power_state_characteristic_handle,
        (uint8_t*)m_application_context->led_power_mode,
        sizeof(uint8_t));
}


// ///command_context
void estc_execute_command(void * p_context)
{
    NRF_LOG_INFO("Global mode is: %d", *m_application_context->mode_global);
    NRF_LOG_INFO("Len of command: %d", m_response_len);
    if (m_response_len < 1) 
    {
        m_current_command_data.status = BLE_COMMAND_NOT_RECOGNIZED;
    
    }

    else 
    {
        NRF_LOG_INFO("Will try find: %d", m_current_command_data.command[0]);
        command_definition_t* command = NULL;
        for (size_t i = 0; i < m_command_definitions_size; ++i)
        {
            if (m_command_definitions[i].command_type == m_current_command_data.command[0]) 
            {
                command = &m_command_definitions[i];
                NRF_LOG_INFO("Found command at index %d", i);
                break;
            }
        }

        if (command != NULL && command->command_type != CMD_UNKNOWN)
        {
            NRF_LOG_INFO("Command %s", command->name);
            int ret_code = command->executor(NULL, m_application_context, m_current_command_data.command + 1, m_response_len - 1);
            if (ret_code == 0) 
            {
                m_current_command_data.status = BLE_COMMAND_SUCCESS;
            }
            else if (ret_code == -1)
            {
                m_current_command_data.status = BLE_COMMAND_NOT_RECOGNIZED;
            }
        }
        else
        {
            m_current_command_data.status = BLE_COMMAND_NOT_RECOGNIZED;
            NRF_LOG_INFO("Command not recognized, type: %d", m_current_command_data.command[0]);
        }
        NRF_LOG_INFO("Global mode is: %d", *m_application_context->mode_global);
    }    
    
    memcpy(&m_response_data, &m_current_command_data, sizeof(m_current_command_data));
    
    ble_gatts_value_t gatts_value = {.len = sizeof(m_current_command_data), .offset = 0, .p_value = (uint8_t*)&m_response_data};
    ret_code_t error_code = sd_ble_gatts_value_set(m_service_instance->connection_handle, m_service_instance->command_characteristic_handle.value_handle, &gatts_value);
    APP_ERROR_CHECK(error_code);
    

    send_notification(
        m_service_instance->connection_handle,
        &m_service_instance->command_characteristic_handle,
        (uint8_t*)&m_current_command_data,
        sizeof(m_current_command_data));
    send_notification(
        m_service_instance->connection_handle,
        &m_service_instance->current_color_characteristic_handle,
        (uint8_t*)m_application_context->current_color_description,
        sizeof(COLOR_DESCRIPTION));
    send_notification(m_service_instance->connection_handle,
        &m_service_instance->led_power_state_characteristic_handle,
        (uint8_t*)m_application_context->led_power_mode,
        sizeof(uint8_t));
    // gatts_value = set_response();
    // error_code = sd_ble_gatts_value_set(m_service_instance->connection_handle, m_service_instance->command_characteristic_handle.value_handle, &gatts_value);
    // APP_ERROR_CHECK(error_code);
    
}

void init_processing_timers(void)
{
    ret_code_t err_code;
    
    err_code = app_timer_create(&processing_start_timer,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                estc_execute_command);
    APP_ERROR_CHECK(err_code);

    //app_timer_create(&processing_start_timer, APP_TIMER_MODE_SINGLE_SHOT, NULL);
}

ret_code_t estc_ble_service_init(ble_estc_service_t *service, command_definition_t* known_commands, size_t known_commands_size,
                command_executor default_command, application_context_t* application_context, ble_context_t* ble_context)
{
    m_command_definitions = known_commands;
    m_command_definitions_size = known_commands_size;
    m_default_executor = default_command;
    m_application_context = application_context;
    m_ble_context = ble_context;
    service->connection_handle = BLE_CONN_HANDLE_INVALID;
    
    ret_code_t error_code = NRF_SUCCESS;

    ble_uuid_t service_uuid = { . uuid = ESTC_SERVICE_UUID, };
    // TODO: 3. Add service UUIDs to the BLE stack table using `sd_ble_uuid_vs_add`
    // TODO: 4. Add service to the BLE stack using `sd_ble_gatts_service_add`
    ble_uuid128_t custom_service_uuid = { .uuid128 = ESTC_BASE_UUID };
    error_code = sd_ble_uuid_vs_add(&custom_service_uuid, &service_uuid.type);
    service->type_handle = service_uuid.type;
    APP_ERROR_CHECK(error_code);
    
    error_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &service_uuid, &service->service_handle);
    APP_ERROR_CHECK(error_code);

    NRF_LOG_INFO("%s:%d | Service UUID: 0x%04x", __FUNCTION__, __LINE__, service_uuid.uuid);
    NRF_LOG_INFO("%s:%d | Service UUID type: 0x%02x", __FUNCTION__, __LINE__, service_uuid.type);
    NRF_LOG_INFO("%s:%d | Service handle: 0x%04x", __FUNCTION__, __LINE__, service->service_handle);

    
    m_service_instance = service;
    init_processing_timers();
    return estc_ble_add_characteristics(service);
}

/**@brief Geberalized function for adding characteristic
 *
 * **/
static ret_code_t estc_ble_add_characteristic(
    ble_estc_service_t          *service,
    uint16_t                     uuid,
    bool                         is_read_allowed,
    bool                         is_write_allowed,
    bool                         is_notification_enabled,
    bool                         is_indication_enabled,
    uint8_t                     *p_user_buffer,
    const uint8_t               *p_init_value,
    uint16_t                     value_len,
    const uint8_t               *p_user_desc,
    uint16_t                     user_desc_len,
    ble_gatts_char_handles_t    *p_char_handles);

static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service)
{
    ret_code_t error_code = NRF_SUCCESS;
 
    error_code = estc_ble_add_current_color_characteristic(service);
    APP_ERROR_CHECK(error_code);
    error_code = estc_ble_add_led_power_state_characteristic(service);
    APP_ERROR_CHECK(error_code);
    error_code = estc_ble_add_command_characteristic(service);
    APP_ERROR_CHECK(error_code);

    return error_code;
}


static ret_code_t estc_ble_add_characteristic(
    ble_estc_service_t          *service,
    uint16_t                     uuid,
    bool                         is_read_allowed,
    bool                         is_write_allowed,
    bool                         is_notification_enabled,
    bool                         is_indication_enabled,
    uint8_t                     *p_user_buffer,
    const uint8_t               *p_init_value,
    uint16_t                     value_len,
    const uint8_t               *p_user_desc,
    uint16_t                     user_desc_len,
    ble_gatts_char_handles_t    *p_char_handles)
{
    ret_code_t error_code = NRF_SUCCESS;

    ble_uuid_t characteristic_uuid = { .uuid = uuid, .type = service->type_handle };

    ble_gatts_char_md_t char_md = { 0 };
    char_md.char_props.read     = is_read_allowed ? 1 : 0;
    char_md.char_props.write    = is_write_allowed ? 1 : 0;
    char_md.char_props.notify   = is_notification_enabled ? 1 : 0;
    char_md.char_props.indicate = is_indication_enabled ? 1 : 0;

    ble_gatts_attr_md_t cccd_md = { 0 };
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.vloc      = BLE_GATTS_VLOC_STACK;
    char_md.p_cccd_md = &cccd_md;

    ble_gatts_attr_md_t attr_md = { 0 };
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&attr_md.write_perm);
    attr_md.vlen = 1;

    ble_gatts_attr_t attr_char_value = { 0 };
    attr_char_value.p_uuid    = &characteristic_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = value_len;
    attr_char_value.init_offs = 0;

    if (p_user_buffer != NULL)
    {
        memcpy(p_user_buffer, p_init_value, value_len);
        attr_md.vloc              = BLE_GATTS_VLOC_USER;
        attr_char_value.p_value   = p_user_buffer;
        attr_char_value.max_len   = value_len;
    }
    else
    {
        attr_md.vloc              = BLE_GATTS_VLOC_STACK;
        attr_char_value.p_value   = (uint8_t *)p_init_value;
        attr_char_value.max_len   = value_len;
    }

    ble_gatts_attr_md_t user_desc_md = { 0 };
    user_desc_md.vloc = BLE_GATTS_VLOC_STACK;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&user_desc_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&user_desc_md.write_perm);

    char_md.p_char_user_desc        = (uint8_t *)p_user_desc;
    char_md.char_user_desc_size     = user_desc_len;
    char_md.char_user_desc_max_size = user_desc_len;
    char_md.p_user_desc_md          = &user_desc_md;
    
    error_code = sd_ble_gatts_characteristic_add(
        service->service_handle, &char_md, &attr_char_value, p_char_handles);
    return error_code;
}

static ret_code_t estc_ble_add_current_color_characteristic(ble_estc_service_t *service)
{
    static const uint8_t desc[]  = "Текущая характеристика цвета, которая обновляется при получении команды. Уведомление отправляется при каждом обновлении характеристики.";

    return estc_ble_add_characteristic(
        service,
        ESTC_GATT_CHAR_CURRENT_COLOR_UUID,
             //permissions: read, write, notify, indicate
        true, false,
        true, true,
        (uint8_t*)m_application_context->current_color_description, (uint8_t*)m_application_context->current_color_description, sizeof(*m_application_context->current_color_description),
        desc, sizeof(desc) - 1,
        &service->current_color_characteristic_handle);
}

static ret_code_t estc_ble_add_led_power_state_characteristic(ble_estc_service_t *service)
{
    static const uint8_t desc[]  = "Текущее состояние свтеодиода(*влкючен или выключен*)";

    return estc_ble_add_characteristic(
        service,
        ESTC_GATT_CHAR_LED_POWER_STATE_UUID,
        //permissions: read, write, notify, indicate
        true, false,
        true, true,
        m_application_context->led_power_mode, m_application_context->led_power_mode, sizeof(uint8_t),
        desc, sizeof(desc),
        &service->led_power_state_characteristic_handle);
}

static ret_code_t estc_ble_add_command_characteristic(ble_estc_service_t *service)
{
    static const uint8_t desc[]  = "Характеристика для записи команд";

    return estc_ble_add_characteristic(
        service,
        ESTC_GATT_CHAR_COMMAND_UUID,
        //permissions: read, write, notify, indicate
        true, true,
        true, true,
        (uint8_t*)&m_response_data, (uint8_t*)&m_response_data, sizeof(m_response_data),
        desc, sizeof(desc) - 1,
        &service->command_characteristic_handle);
        /// write either 00 or 01
}

void estc_process_command(ble_estc_service_t *service, const ble_gatts_evt_write_t *write)
{
    if (m_response_data.status != BLE_COMMAND_IN_PROCESS && m_response_data.status != BLE_COMMAND_BUSY)
    {
        m_response_data.status = BLE_COMMAND_IN_PROCESS;
        memcpy(&m_current_command_data, &m_response_data, sizeof(m_response_data));
        m_response_len = write->len;
        NRF_LOG_INFO("Write data len: %d", m_response_len);
        ret_code_t err_code = app_timer_start(processing_start_timer, APP_TIMER_TICKS(100), NULL);
        NRF_LOG_INFO("Timer start error code: %d", err_code);
    }
    else 
    {
        m_response_data.status = BLE_COMMAND_BUSY;
    }

    ble_gatts_value_t gatts_value = {0};
    gatts_value.len = sizeof(m_response_data);
    gatts_value.offset = 0;
    gatts_value.p_value = (uint8_t*)&m_response_data;

    ret_code_t error_code = sd_ble_gatts_value_set(m_service_instance->connection_handle, m_service_instance->command_characteristic_handle.value_handle, &gatts_value);
    APP_ERROR_CHECK(error_code);
    send_notification(m_service_instance->connection_handle, &m_service_instance->command_characteristic_handle, (uint8_t*)&m_response_data, sizeof(m_response_data));
    // APP_ERROR_CHECK(error_code);
}

void estc_update_led_power_state_characteristic_value(ble_estc_service_t *service, const ble_gatts_evt_write_t *write)
{
    NRF_LOG_INFO("We received data: %d", *write->data);

    m_response_data.command[0] = CMD_POWER_SWITCH;
    m_response_data.command[1] = *write->data; 
    estc_process_command(service, write);
}

