#ifndef SAVE_CURRENT_COLOR_COMMAND_H
#define SAVE_CURRENT_COLOR_COMMAND_H
#include "nvram_utils.h"

int save_settings(char* args, application_context_t* context, uint8_t* data, uint8_t data_len)
{
    context->settings->saved_color = context->current_color_description->hsv;
    context->settings->led_mode = *context->led_power_mode;
    NRF_LOG_INFO("Current led mode: %d", context->settings->led_mode);
    nvram_save_settings((uint32_t*)context->settings, sizeof(*(context->settings)));
    bool is_binary_arg = ((data != NULL) && (data_len == 4));
    // bool is_args_valid = (((args != NULL) && (strlen(args) > 0)) || is_binary_arg);
    if (is_binary_arg)
    {
        static char msg[100];
        snprintf(msg, sizeof(msg),
                    "Current settings successfully saved\r\n");    
        usb_serial_dumb_print(msg, strlen(msg));
    } else 
    {
        NRF_LOG_INFO("Current settings successfully saved");
    }
    return 0;
}


command_definition_t save_settings_command =
{
        .command_type = CMD_SAVE_SETTINGS,
        .name = "SAVE",
        .description = "SAVE - save current settings to NVRAM.\r\n",
        .executor = save_settings
};

#endif