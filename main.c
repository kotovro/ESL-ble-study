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
#include "fds.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "sensorsim.h"
#include "ble_conn_state.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"

#include "ble_utils.h"

#include "board_utils.h"
#include "unknown_command.h"
#include "set_rgb_command.h"
#include "set_hsv_command.h"
#include "power_off_command.h"
#include "power_on_command.h"
#include "help_command.h"

bool hue_d = DECREASE;
bool saturation_d = DECREASE;
bool value_d = DECREASE;
int mode_global = SLEEP;
COLOR_DESCRIPTION color_palette[AVAILABLE_COLOR_SLOTS];
SETTINGS settings;
COLOR_DESCRIPTION color_description = {1, "LOL\0", 22, 100, 100};
COMMAND_CONTEXT application_context = {&mode_global, color_palette, &color_description, &settings, &hue_d, &saturation_d, &value_d};

NRF_BLE_GATT_DEF(m_gatt);                                                       /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);         
BLE_ADVERTISING_DEF(m_advertising);                                                   /**< Context for the Queued Write module.*/

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

    // err_code = app_timer_create(&m_notification_timer_id, APP_TIMER_MODE_REPEATED, estc_notify_update_on_timer);
    APP_ERROR_CHECK(err_code);
}

static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

void fill_command_definitions()
{
    init_command_definitions();
    command_definitions[0] = set_rgb_command;
    command_definitions[1] = set_hsv_command;
    command_definitions[2] = power_off_command;
    command_definitions[3] = power_on_command;
    command_definitions[4] = help_command;
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

    ble_init(command_definitions, sizeof(command_definitions) / sizeof(COMMAND_DEFINITION),
                unknown_command_executor, &application_context);

    fill_command_definitions();
    init_usb_cli(command_definitions, sizeof(command_definitions) / sizeof(COMMAND_DEFINITION),
                unknown_command_executor, &application_context);
    // Start execution.
    NRF_LOG_INFO("Blinky example started.");

    advertising_start(pattern_slow_blinking);

    // Enter main loop.
    for (;;)
    {
        idle_state_handle();
    }
}


/**
 * @}
 */
