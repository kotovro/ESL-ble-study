/**
 * Copyright (c) 2015 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/**
 * @brief Blinky Sample Application main file.
 *
 * This file contains the source code for a sample server application using the LED Button service.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "app_timer.h"
#include "fds.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "bsp_btn_ble.h"
#include "sensorsim.h"
#include "ble_conn_state.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_pwr_mgmt.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"

#include "estc_service.h"
#include "ble_utils.h"
#include "board_utils.h"

// APP_TIMER_DEF(m_indication_timer_id);
APP_TIMER_DEF(m_notification_timer_id);                                                          
NRF_BLE_GATT_DEF(m_gatt);                                                       /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);         
BLE_ADVERTISING_DEF(m_advertising);                                                   /**< Context for the Queued Write module.*/

bool hue_d = DECREASE;
bool saturation_d = DECREASE;
bool value_d = DECREASE;
int mode_global = SLEEP;
COLOR_DESCRIPTION color_palette[AVAILABLE_COLOR_SLOTS];
SETTINGS settings;
COLOR_DESCRIPTION color_description = {1, "LOL\0", 22, 100, 100};
COMMAND_CONTEXT application_context = {&mode_global, color_palette, &color_description, &settings, &hue_d, &saturation_d, &value_d};

/// simplest option is to increment this value anf then write it
static uint8_t variable_changed_on_indication = 0;

static ble_uuid_t m_adv_uuids[] =                                               /**< Universally unique service identifiers. */
{
    {BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE},
    {ESTC_SERVICE_UUID, BLE_UUID_TYPE_BLE},
};


ble_estc_service_t m_estc_service; /**< ESTC example BLE service */

static bool m_indication_enabled = false;
static bool m_notification_enabled = false;
static bool m_indication_pending = false;
/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for the LEDs initialization.
 *
 * @details Initializes all LEDs used by the application.
 */
static void leds_init(COLOR_DESCRIPTION* color_description, COMMAND_CONTEXT* context)
{
    init_leds_init(context);
    init_pwm_leds(color_description);
    // bsp_board_init(BSP_INIT_LEDS);
}

static void buttons_init(void)
{
   init_button(double_click_executor, button_press_executor);
}

/**@brief Function for the Timer callback.
 *
 * @details Increments the indication variable and sends indication with the new value. This function will be called each time the timer expires.
 */
void estc_indicate_update_on_timer(void *context)
{
    variable_changed_on_indication++;    


    estc_update_timer_dependent_characteristic_value(
        &m_estc_service,
        &variable_changed_on_indication);
    if (!m_indication_enabled && m_indication_pending)
        return;
    // NRF_LOG_INFO("Will try to update value: %d", variable_changed_on_indication);


    // NRF_LOG_INFO("Will try to indicate value: %d", variable_changed_on_indication);
    ret_code_t err = send_indication(
        m_estc_service.connection_handle,
        m_estc_service.characteristic_timer_dependent_handle.value_handle,
        &variable_changed_on_indication);
    if (err != NRF_SUCCESS)
    {
        m_indication_pending = true;
    }
}

static uint8_t notification_value = 0; 
void estc_notify_update_on_timer(void *context)
{
    notification_value++;
    estc_update_characteristic_1_value(
        &m_estc_service,
        (int32_t *)&notification_value);

    if (!m_notification_enabled)
        return;

    ret_code_t err = send_notitification(
        m_estc_service.connection_handle,
        m_estc_service.characteristic_with_notification_handle.value_handle,
        (uint8_t *)&notification_value);
    APP_ERROR_CHECK(err);
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module.
 */
static void ble_timers_init(void)
{
    // Initialize timer module, making it use the scheduler
    ret_code_t err_code = app_timer_init(); ///
    APP_ERROR_CHECK(err_code);
    // err_code = app_timer_create(&m_indication_timer_id, APP_TIMER_MODE_REPEATED, estc_indicate_update_on_timer);
    // APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_notification_timer_id, APP_TIMER_MODE_REPEATED, estc_notify_update_on_timer);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
    ret_code_t           err_code;

    err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
    // err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG); - low-level call of gap
    APP_ERROR_CHECK(err_code);

    pattern_slow_blinking();
    
}


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
            err_code = app_button_enable();
            APP_ERROR_CHECK(err_code);
            app_timer_start(m_notification_timer_id, APP_TIMER_TICKS(NOTIFICATION_FREQUENCY_MS), NULL);
            // app_timer_start(m_indication_timer_id, APP_TIMER_TICKS(INDICATION_FREQUENCY_MS), NULL);   

            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected");
            // bsp_board_led_off(CONNECTED_LED);
            pattern_slow_blinking();
            m_estc_service.connection_handle = BLE_CONN_HANDLE_INVALID;
            app_timer_stop(m_notification_timer_id);
            // app_timer_stop(m_indication_timer_id);
            m_indication_enabled = false;
            m_notification_enabled = false;
            advertising_start();
            
            break;

        case BLE_GATTS_EVT_WRITE:
        {
            const ble_gatts_evt_write_t *write = &p_ble_evt->evt.gatts_evt.params.write;
            
            // NRF_LOG_INFO("Received write event, data: %d", m_estc_service.characteristic_timer_dependent_handle.cccd_handle);
            if (write->handle == m_estc_service.characteristic_with_notification_handle.value_handle)
            {
                estc_update_characteristic_1_value(&m_estc_service, (int32_t *)write->data); 
                if (m_notification_enabled)
                {
                    err_code = send_notitification(m_estc_service.connection_handle, m_estc_service.characteristic_with_notification_handle.value_handle, write->data);
                    APP_ERROR_CHECK(err_code);
                }
            }
            else if (write->handle == m_estc_service.characteristic_with_notification_handle.cccd_handle)
            {
                const uint8_t *cccd = p_ble_evt->evt.gatts_evt.params.write.data;
                m_notification_enabled = ble_srv_is_notification_enabled(cccd);
            }
            else if (write->handle == m_estc_service.characteristic_timer_dependent_handle.cccd_handle)
            {
                const uint8_t *cccd = p_ble_evt->evt.gatts_evt.params.write.data;
                m_indication_enabled = ble_srv_is_indication_enabled(cccd);
            }
        }
        break;

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


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
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


static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
	LOG_BACKEND_USB_PROCESS();
}

/**@brief Function for application main entry.
 */
int main(void)
{
    // Initialize.

    log_init();
    leds_init(&color_description, &application_context);
    ble_timers_init();
    init_button_executors(&application_context);
    buttons_init();
    power_management_init();

    ble_stack_init();
    gap_params_init();
    gatt_init(&m_gatt);
    services_init(&m_qwr, &m_estc_service);
    advertising_init(m_adv_uuids, on_adv_evt, &m_advertising);
    conn_params_init(&m_estc_service.connection_handle);

    // Start execution.
    NRF_LOG_INFO("Blinky example started.");

    advertising_start();

    // Enter main loop.
    for (;;)
    {
        idle_state_handle();
    }
}


/**
 * @}
 */
