
#include <string.h>

#include "app_config.h"
#include "nordic_common.h"
#include "nrf.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "fds.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "sensorsim.h"
#include "ble_conn_state.h"
#include "nrf_pwr_mgmt.h"
#include "app_error.h"
#include "ble_gatts.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"


#include "ble_utils.h"
#include "cli_utils.h" 
#include "board_utils.h"

NRF_BLE_GATT_DEF(m_gatt);                                                       /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);         
BLE_ADVERTISING_DEF(m_advertising);                                                   /**< Context for the Queued Write module.*/
uint16_t* connection_handle;

static bool m_indication_pending = false;


ble_estc_service_t m_estc_service; /**< ESTC example BLE service */
ble_context_t m_ble_context;
static ble_uuid_t m_adv_uuids[] =                                               /**< Universally unique service identifiers. */
{
    {BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE},
    {ESTC_SERVICE_UUID, BLE_UUID_TYPE_BLE},
};
/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code;
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected");
            // bsp_board_led_on(CONNECTED_LED);
            // bsp_board_led_off(ADVERTISING_LED);
            pattern_on();
            m_estc_service.connection_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_estc_service.connection_handle);
            APP_ERROR_CHECK(err_code);
           
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected");
            // bsp_board_led_off(CONNECTED_LED);
            pattern_slow_blinking();
            m_estc_service.connection_handle = BLE_CONN_HANDLE_INVALID;
            advertising_start(pattern_slow_blinking);
            
            break;

        case BLE_GATTS_EVT_WRITE:
        {
            const ble_gatts_evt_write_t *write = &p_ble_evt->evt.gatts_evt.params.write;
            
            // NRF_LOG_INFO("Received write event, data: %d", m_estc_service.characteristic_timer_dependent_handle.cccd_handle);
            if (write->handle == m_estc_service.command_characteristic_handle.value_handle)
            {
                NRF_LOG_INFO("Received write event");
                estc_process_command(&m_estc_service, write); 
                NRF_LOG_INFO("Received write event for characteristic with notification, value: %u", write->len);
                
            }
            else if (write->handle == m_estc_service.power_state_characteristic_handle.value_handle)
            {
                NRF_LOG_INFO("Received write event for characteristic with notification, value: %u", write->len);
                // estc_update_power_state_characteristic_value(&m_estc_service, write);
            }
            else 
            {
                for (int i = 0; i < 3; ++i) ///replace with CHARACTERISTIC_COUNT
                {
                    if (write->handle == m_ble_context.characteristics_subscription_status[i].characteristic_handle->cccd_handle)
                    {
                        const uint8_t *cccd = p_ble_evt->evt.gatts_evt.params.write.data;
                        m_ble_context.characteristics_subscription_status[i].is_notification_enabled = ble_srv_is_notification_enabled(cccd);
                        m_ble_context.characteristics_subscription_status[i].is_indication_enabled = ble_srv_is_indication_enabled(cccd);
                    }
                }
            }
        }
        break;//

        case BLE_GATTS_EVT_HVC:
                m_indication_pending = false;
            break;
        // case BLE_GATTS_EVT_HVN_TX_COMPLETE:
        //     m_indication_pending = false;
        //     break;
        
        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
        {
            ble_gap_sec_params_t sec_params = { 0 };
            sec_params.bond    = 0;
            sec_params.mitm    = 0;
            sec_params.io_caps = BLE_GAP_IO_CAPS_NONE;
            sec_params.min_key_size = BLE_NFC_SEC_PARAM_MIN_KEY_SIZE;
            sec_params.max_key_size = BLE_NFC_SEC_PARAM_MAX_KEY_SIZE;

            err_code = sd_ble_gap_sec_params_reply(
                p_ble_evt->evt.gap_evt.conn_handle,
                BLE_GAP_SEC_STATUS_SUCCESS,
                &sec_params,
                NULL);
            APP_ERROR_CHECK(err_code);
            break;    
        }
        // // Pairing not supported
            // err_code = sd_ble_gap_sec_params_reply(m_estc_service.connection_handle,
            //                                        BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP,
            //                                        NULL,
            //                                        NULL);
            // APP_ERROR_CHECK(err_code);
            // break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            err_code = sd_ble_gatts_sys_attr_set(m_estc_service.connection_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;
        
        default:
            NRF_LOG_INFO("Unhandled BLE event: 0x%04x", p_ble_evt->header.evt_id);
            // No implementation needed.
            break;
    }
}

void ble_init(COMMAND_DEFINITION* command_definitions, size_t command_definitions_size, command_executor default_command_executor, application_context_t* application_context)
{
    ble_stack_init();
    gap_params_init();
    gatt_init(&m_gatt);
    services_init(&m_qwr, &m_estc_service, command_definitions, command_definitions_size,
                default_command_executor, application_context);
    advertising_init(m_adv_uuids, on_adv_evt, &m_advertising);
    conn_params_init(&m_estc_service.connection_handle);
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
void ble_stack_init()
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
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
    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("ADV Event: Start fast advertising");
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
                command_executor default_command, application_context_t* application_context)
{
    ret_code_t         err_code;
    nrf_ble_qwr_init_t qwr_init = {0};

    // // Initialize Queued Write Module.

    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    
    m_ble_context.characteristics_subscription_status[0].characteristic_handle = &m_estc_service.command_characteristic_handle;
    m_ble_context.characteristics_subscription_status[1].characteristic_handle = &m_estc_service.current_color_characteristic_handle;
    m_ble_context.characteristics_subscription_status[2].characteristic_handle = &m_estc_service.power_state_characteristic_handle;
    for (int i = 0; i < 3; ++i)
    {
        m_ble_context.characteristics_subscription_status[i].is_indication_enabled = false;
        m_ble_context.characteristics_subscription_status[i].is_notification_enabled = false;
    }

    err_code = estc_ble_service_init(estc_service, known_commands, known_commands_size, default_command, application_context, &m_ble_context);
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

/**@brief Function for starting advertising.
 */
void advertising_start(indicate_function_t indicate_function)
{
    ret_code_t           err_code;

    err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
    // err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG); - low-level call of gap
    // APP_ERROR_CHECK(err_code); ///sometimes it;s safe just to ignore NRF_INALID_STATE in soft device, as forums suggest
    //for example, https://devzone.nordicsemi.com/f/nordic-q-a/14099/handling-nrf_error_invalid_state-error-code
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_INFO("adv_start error: %d\n", err_code);
    }

    if (indicate_function != NULL) indicate_function();    
}
