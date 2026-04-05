
#include <string.h>

#include "app_config.h"

#include "app_error.h"
#include "app_timer.h"
#include "ble_gatts.h"
#include "bsp_btn_ble.h"
#include "ble_conn_params.h"

#include "ble_utils.h"

uint16_t* connection_handle;

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

/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
void gap_params_init(void)
{
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));
    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the GATT module.
 */
void gatt_init(nrf_ble_gatt_t* gatt)
{
    ret_code_t err_code = nrf_ble_gatt_init(gatt, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
void sleep_mode_enter(void)
{
    ret_code_t err_code;

    err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    ret_code_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("ADV Event: Start fast advertising");
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_ADV_EVT_IDLE:
            NRF_LOG_INFO("ADV Event: idle, no connectable advertising is ongoing");
            sleep_mode_enter();
            break;

        default:
            break;
    }
}

/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
void advertising_init(ble_uuid_t* adv_uuids, Adv_evt_handler_t adv_evt_handler, ble_advertising_t* advertising)
{
    ret_code_t             err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type               = BLE_ADVDATA_NO_NAME;
    init.advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

    // TODO: 8. Consider moving the device characteristics to the Scan Response if necessary
    init.advdata.uuids_complete.uuid_cnt = sizeof(*adv_uuids) / sizeof(adv_uuids[0]);
    init.advdata.uuids_complete.p_uuids  = adv_uuids;

    init.srdata.name_type               = BLE_ADVDATA_FULL_NAME;

    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;
    


    init.evt_handler = adv_evt_handler;

    err_code = ble_advertising_init(advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(advertising, APP_BLE_CONN_CFG_TAG);
}

/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing services that will be used by the application.
 */
void services_init(nrf_ble_qwr_t* qwr, ble_estc_service_t* estc_service, COMMAND_DEFINITION* known_commands, size_t known_commands_size,
                Command_Executor default_command, COMMAND_CONTEXT* application_context)
{
    ret_code_t         err_code;
    nrf_ble_qwr_init_t qwr_init = {0};

    // // Initialize Queued Write Module.

    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    err_code = estc_ble_service_init(estc_service, known_commands, known_commands_size, default_command, application_context);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module that
 *          are passed to the application.
 *
 * @note All this function does is to disconnect. This could have been done by simply
 *       setting the disconnect_on_fail config parameter, but instead we use the event
 *       handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    ret_code_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(*connection_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
void conn_params_init(uint16_t* conn_handle)
{
    ret_code_t             err_code;
    ble_conn_params_init_t cp_init;
    connection_handle  = conn_handle;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}