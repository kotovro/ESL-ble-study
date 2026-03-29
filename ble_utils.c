
#include <string.h>
#include "ble_utils.h"
#include "ble_gatts.h"


uint32_t send_notitification(uint16_t conn_handle, uint16_t char_value_handle, const uint8_t *data) 
{
    ble_gatts_hvx_params_t hvx_params = {0};
    uint16_t len = sizeof(data);

    memset(&hvx_params, 0, sizeof(hvx_params));
    hvx_params.handle = char_value_handle;
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
    hvx_params.p_len  = &len;
    hvx_params.p_data = data;
    
    uint32_t err = sd_ble_gatts_hvx(conn_handle, &hvx_params);
    return err;
}


uint32_t send_indication(uint16_t conn_handle, uint16_t char_value_handle, const uint8_t *data) 
{
    ble_gatts_hvx_params_t hvx_params = {0};
    uint16_t len = sizeof(*data);

    memset(&hvx_params, 0, sizeof(hvx_params));
    hvx_params.handle = char_value_handle;
    hvx_params.type   = BLE_GATT_HVX_INDICATION;
    hvx_params.p_len  = &len;
    hvx_params.p_data = data;
    
    uint32_t err = sd_ble_gatts_hvx(conn_handle, &hvx_params);
    return err;
}