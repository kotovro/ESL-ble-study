#ifndef BlE_UTILS_H__
#define BLE_UTILS_H__

#include <stdint.h>

#include "ble_advdata.h"
#include "ble_advertising.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_log.h"

#include "estc_service.h"

uint32_t send_notitification(uint16_t conn_handle, uint16_t char_value_handle, const uint8_t *data);
uint32_t send_indication(uint16_t conn_handle, uint16_t char_value_handle, const uint8_t *data);
void gap_params_init(void);
void gatt_init(nrf_ble_gatt_t* gatt);
void sleep_mode_enter(void);
void on_adv_evt(ble_adv_evt_t ble_adv_evt);
void services_init(nrf_ble_qwr_t* qwr, ble_estc_service_t* estc_service);

typedef void (*Adv_evt_handler_t)(ble_adv_evt_t);

void advertising_init(ble_uuid_t* adv_uuids, Adv_evt_handler_t adv_evt_handler, ble_advertising_t* advertising);


#endif /* BlE_UTILS_H__ */