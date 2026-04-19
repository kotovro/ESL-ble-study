#ifndef BlE_UTILS_H__
#define BLE_UTILS_H__

#include <stdint.h>

#include "ble_advdata.h"
#include "ble_advertising.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_log.h"

#include "estc_service.h"

#define MAX_CONNECTIONS 1

uint32_t send_notitification(uint16_t conn_handle, uint16_t char_value_handle, const uint8_t *data);
uint32_t send_indication(uint16_t conn_handle, uint16_t char_value_handle, const uint8_t *data);
void gap_params_init(void);
void gatt_init(nrf_ble_gatt_t* gatt);
void sleep_mode_enter(void);
void on_adv_evt(ble_adv_evt_t ble_adv_evt);
void services_init(nrf_ble_qwr_t* qwr, ble_estc_service_t* estc_service, command_definition_t* known_commands, size_t known_commands_size,
                command_executor default_command, application_context_t* application_context);
void conn_params_init(void);

typedef void (*Adv_evt_handler_t)(ble_adv_evt_t);
typedef void (*indicate_function_t)(void);





void advertising_init(ble_uuid_t* adv_uuids, uint8_t total_uuid_cnt, Adv_evt_handler_t adv_evt_handler, ble_advertising_t* advertising);
void advertising_start(indicate_function_t indicate_function);
void ble_stack_init(void);
void ble_init(command_definition_t* command_definitions, size_t command_definitions_size, command_executor default_command_executor, application_context_t* application_context);


#endif /* BlE_UTILS_H__ */