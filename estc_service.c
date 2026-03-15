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

#include "estc_service.h"

#include "app_error.h"
#include "nrf_log.h"

#include "ble.h"
#include "ble_gatts.h"
#include "ble_srv_common.h"

static uint8_t m_ram_local_buffer[20];

// static uint8_t m_ram_local_buffer_1[20];
// static uint8_t m_ram_local_buffer_2[20];

static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service);
static ret_code_t estc_ble_add_notifications_characteristic(ble_estc_service_t *service);
static ret_code_t estc_ble_add_timer_dependent_characteristic(ble_estc_service_t *service);
// static ret_code_t estc_ble_add_another_characteristic(ble_estc_service_t *service);

ret_code_t estc_ble_service_init(ble_estc_service_t *service)
{
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
    // NRF_LOG_DEBUG("%s:%d | Service UUID: 0x%04x", __FUNCTION__, __LINE__, service_uuid.uuid);
    // NRF_LOG_DEBUG("%s:%d | Service UUID type: 0x%02x", __FUNCTION__, __LINE__, service_uuid.type);
    // NRF_LOG_DEBUG("%s:%d | Service handle: 0x%04x", __FUNCTION__, __LINE__, service->service_handle);

    NRF_LOG_INFO("%s:%d | Service UUID: 0x%04x", __FUNCTION__, __LINE__, service_uuid.uuid);
    NRF_LOG_INFO("%s:%d | Service UUID type: 0x%02x", __FUNCTION__, __LINE__, service_uuid.type);
    NRF_LOG_INFO("%s:%d | Service handle: 0x%04x", __FUNCTION__, __LINE__, service->service_handle);

    return estc_ble_add_characteristics(service);
}
// static ret_code_t estc_ble_add_characteristic(ble_estc_service_t *service, uint16_t char_uuid);
static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service)
{
    ret_code_t error_code = NRF_SUCCESS;
 
    error_code = estc_ble_add_notifications_characteristic(service);
    APP_ERROR_CHECK(error_code);
    error_code = estc_ble_add_timer_dependent_characteristic(service);
    APP_ERROR_CHECK(error_code);
    // error_code = estc_ble_add_characteristic(service, ESTC_GATT_CHAR_1_UUID);
    // APP_ERROR_CHECK(error_code);
    // error_code = estc_ble_add_characteristic(service, ESTC_GATT_CHAR_2_UUID);
    // APP_ERROR_CHECK(error_code);
    // error_code = estc_ble_add_another_characteristic(service);
    // APP_ERROR_CHECK(error_code);

    return error_code;
}
// static ret_code_t estc_ble_add_characteristic(ble_estc_service_t *service, uint16_t char_uuid)
// {
//     ret_code_t error_code = NRF_SUCCESS;

//     // TODO: 6.1. Add custom characteristic UUID using `sd_ble_uuid_vs_add`, same as in step 4
//     ble_uuid_t characteristic_uuid = { .uuid = char_uuid, };
//     characteristic_uuid.type = service->type_handle;
//     APP_ERROR_CHECK(error_code);

    
//     // TODO: 6.5. Configure Characteristic metadata (enable read and write)
//     ble_gatts_char_md_t char_md = { 0 };
//     char_md.char_props.read = 1;
//     char_md.char_props.write = 1;
//     char_md.char_props.notify = char_uuid == ESTC_GATT_CHAR_1_UUID ? 1 : 0;  // Enable notifications
//     char_md.char_props.indicate = char_uuid == ESTC_GATT_CHAR_2_UUID ? 1 : 0; 

//     // Configures attribute metadata. For now we only specify that the attribute will be stored in the softdevice
    
//     ble_gatts_attr_md_t cccd_md = {0};
//     // Allow client to READ and WRITE the CCCD (required for subscription)
//     BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
//     BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);

//     cccd_md.vloc = BLE_GATTS_VLOC_STACK; // Let SoftDevice store CCCD value internally
//     char_md.p_cccd_md = &cccd_md;        // Link CCCD metadata to characteristic

//     ble_gatts_attr_md_t attr_md = { 0 };
//     attr_md.vloc = BLE_GATTS_VLOC_USER;

    
//     // TODO: 6.6. Set read/write security levels to our attribute metadata using `BLE_GAP_CONN_SEC_MODE_SET_OPEN`
//     BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
//     BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

//     // TODO: 6.2. Configure the characteristic value attribute (set the UUID and metadata)
    
//     ble_gatts_attr_t attr_char_value = { 0 };
//     attr_char_value.p_uuid = &characteristic_uuid;
//     attr_char_value.p_attr_md = &attr_md;

//     uint8_t local_buffer[20] = {0xAA, 0xBB, 0xCC}; // Initial value of the characteristic

//     // static uint8_t initial_value = 100;
//     attr_md.vlen = 1;
//     attr_char_value.init_len  = sizeof(local_buffer); 
//     attr_char_value.init_offs = 0;
//     attr_char_value.max_len   = sizeof(local_buffer);
//     attr_char_value.p_value   = char_uuid == ESTC_GATT_CHAR_1_UUID ? m_ram_local_buffer_1 : m_ram_local_buffer_2; 
//     memcpy(attr_char_value.p_value, local_buffer, sizeof(local_buffer));
//     // TODO: 6.7. Set characteristic length in number of bytes in attr_char_value structure
    
//     attr_char_value.max_len = BLE_GATTS_FIX_ATTR_LEN_MAX;

//     static uint8_t user_desc[] = "Значение характеристики 1";

//     ble_gatts_attr_md_t user_desc_md = {0};
//     user_desc_md.vloc = BLE_GATTS_VLOC_STACK;

//     BLE_GAP_CONN_SEC_MODE_SET_OPEN(&user_desc_md.read_perm);
//     BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&user_desc_md.write_perm);

//     char_md.p_char_user_desc        = user_desc;
//     char_md.char_user_desc_size     = sizeof(user_desc) - 1;
//     char_md.char_user_desc_max_size = sizeof(user_desc) - 1;
//     char_md.p_user_desc_md          = &user_desc_md;
//     // TODO: 6.4. Add new characteristic to the service using `sd_ble_gatts_characteristic_add`

//     error_code = sd_ble_gatts_characteristic_add(service->service_handle, &char_md, &attr_char_value, &service->characteristic_with_notification_handle);
//     return error_code;
// }

static ret_code_t estc_ble_add_notifications_characteristic(ble_estc_service_t *service)
{
    ret_code_t error_code = NRF_SUCCESS;

    // TODO: 6.1. Add custom characteristic UUID using `sd_ble_uuid_vs_add`, same as in step 4
    ble_uuid_t characteristic_uuid = { .uuid = ESTC_GATT_CHAR_1_UUID, };
    characteristic_uuid.type = service->type_handle;
    APP_ERROR_CHECK(error_code);

    
    // TODO: 6.5. Configure Characteristic metadata (enable read and write)
    ble_gatts_char_md_t char_md = { 0 };
    char_md.char_props.read = 1;
    char_md.char_props.write = 1;
    char_md.char_props.notify = 1;  // Enable notifications
    char_md.char_props.indicate = 0; // Enable indications (optional, use one or both)

    // Configures attribute metadata. For now we only specify that the attribute will be stored in the softdevice
    
    ble_gatts_attr_md_t cccd_md = {0};
    // Allow client to READ and WRITE the CCCD (required for subscription)
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);

    cccd_md.vloc = BLE_GATTS_VLOC_STACK; // Let SoftDevice store CCCD value internally
    char_md.p_cccd_md = &cccd_md;        // Link CCCD metadata to characteristic

    ble_gatts_attr_md_t attr_md = { 0 };
    attr_md.vloc = BLE_GATTS_VLOC_USER;

    
    // TODO: 6.6. Set read/write security levels to our attribute metadata using `BLE_GAP_CONN_SEC_MODE_SET_OPEN`
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    // TODO: 6.2. Configure the characteristic value attribute (set the UUID and metadata)
    
    ble_gatts_attr_t attr_char_value = { 0 };
    attr_char_value.p_uuid = &characteristic_uuid;
    attr_char_value.p_attr_md = &attr_md;

    uint8_t local_buffer[20] = {0xAA, 0xBB, 0xCC}; // Initial value of the characteristic

    // static uint8_t initial_value = 100;
    attr_md.vlen = 1;
    attr_char_value.init_len  = sizeof(local_buffer); 
    attr_char_value.init_offs = 0;
    attr_char_value.max_len   = sizeof(local_buffer);
    attr_char_value.p_value   = m_ram_local_buffer; 
    memcpy(attr_char_value.p_value, local_buffer, sizeof(local_buffer));
    // TODO: 6.7. Set characteristic length in number of bytes in attr_char_value structure
    
    attr_char_value.max_len = BLE_GATTS_FIX_ATTR_LEN_MAX;

    static uint8_t user_desc[] = "Значение характеристики 1";

    ble_gatts_attr_md_t user_desc_md = {0};
    user_desc_md.vloc = BLE_GATTS_VLOC_STACK;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&user_desc_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&user_desc_md.write_perm);

    char_md.p_char_user_desc        = user_desc;
    char_md.char_user_desc_size     = sizeof(user_desc) - 1;
    char_md.char_user_desc_max_size = sizeof(user_desc) - 1;
    char_md.p_user_desc_md          = &user_desc_md;
    // TODO: 6.4. Add new characteristic to the service using `sd_ble_gatts_characteristic_add`

    error_code = sd_ble_gatts_characteristic_add(service->service_handle, &char_md, &attr_char_value, &service->characteristic_with_notification_handle);
    return error_code;
}

static ret_code_t estc_ble_add_timer_dependent_characteristic(ble_estc_service_t *service)
{
    ret_code_t error_code = NRF_SUCCESS;

    // TODO: 6.1. Add custom characteristic UUID using `sd_ble_uuid_vs_add`, same as in step 4
    ble_uuid_t characteristic_uuid = { .uuid = ESTC_GATT_CHAR_2_UUID, };
    characteristic_uuid.type = service->type_handle;
    APP_ERROR_CHECK(error_code);

    
    // TODO: 6.5. Configure Characteristic metadata (enable read and write)
    ble_gatts_char_md_t char_md = { 0 };
    char_md.char_props.read = 1;
    char_md.char_props.write = 0;
    char_md.char_props.notify = 0;  // Enable notifications
    char_md.char_props.indicate = 1; // Enable indications (optional, use one or both)

    // Configures attribute metadata. For now we only specify that the attribute will be stored in the softdevice
    
    ble_gatts_attr_md_t cccd_md = {0};
    // Allow client to READ and WRITE the CCCD (required for subscription)
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);

    cccd_md.vloc = BLE_GATTS_VLOC_STACK; // Let SoftDevice store CCCD value internally
    char_md.p_cccd_md = &cccd_md;        // Link CCCD metadata to characteristic

    ble_gatts_attr_md_t attr_md = { 0 };
    attr_md.vloc = BLE_GATTS_VLOC_STACK; //////

    
    // TODO: 6.6. Set read/write security levels to our attribute metadata using `BLE_GAP_CONN_SEC_MODE_SET_OPEN`
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    // TODO: 6.2. Configure the characteristic value attribute (set the UUID and metadata)
    attr_md.vlen = 1;
    
    
    static uint8_t initial_value = 112; // Initial value of the characteristic
    ble_gatts_attr_t attr_char_value = { 0 };
    attr_char_value.p_uuid = &characteristic_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = sizeof(initial_value); 
    attr_char_value.init_offs = 0;
    attr_char_value.max_len   = sizeof(initial_value);
    attr_char_value.p_value   = &initial_value; 
    // TODO: 6.7. Set characteristic length in number of bytes in attr_char_value structure

    static uint8_t user_desc[] = "Значение характеристики с индицкацией";

    ble_gatts_attr_md_t user_desc_md = {0};
    user_desc_md.vloc = BLE_GATTS_VLOC_STACK;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&user_desc_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&user_desc_md.write_perm);

    char_md.p_char_user_desc        = user_desc;
    char_md.char_user_desc_size     = sizeof(user_desc) - 1;
    char_md.char_user_desc_max_size = sizeof(user_desc) - 1;
    char_md.p_user_desc_md          = &user_desc_md;
    // TODO: 6.4. Add new characteristic to the service using `sd_ble_gatts_characteristic_add`

    error_code = sd_ble_gatts_characteristic_add(service->service_handle, &char_md, &attr_char_value, &service->characteristic_timer_dependent_handle);
    return error_code;
}


void estc_update_characteristic_1_value(ble_estc_service_t *service, int32_t *value)
{
    ble_gatts_value_t gatts_value = {0};
    gatts_value.len = sizeof(int32_t);
    gatts_value.offset = 0;
    gatts_value.p_value = (uint8_t *)value;

    ret_code_t error_code = sd_ble_gatts_value_set(service->connection_handle, service->characteristic_with_notification_handle.value_handle, &gatts_value);
    APP_ERROR_CHECK(error_code);
}

/// TODO: somehow come up with one method using, for instance, void pointer and pass characteristic handle as an argument, to avoid code duplication for each characteristic update function
void estc_update_timer_dependent_characteristic_value(ble_estc_service_t *service, uint8_t *value)
{
    ble_gatts_value_t gatts_value = {0};
    gatts_value.len = sizeof(uint8_t);
    gatts_value.offset = 0;
    gatts_value.p_value = value;

    ret_code_t error_code = sd_ble_gatts_value_set(service->connection_handle, service->characteristic_timer_dependent_handle.value_handle, &gatts_value);

    APP_ERROR_CHECK(error_code);
}

