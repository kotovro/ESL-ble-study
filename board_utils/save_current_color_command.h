#ifndef SAVE_CURRENT_COLOR_COMMAND_H
#define SAVE_CURRENT_COLOR_COMMAND_H
#include "nvram_utils.h"

int save_current_color(char* args, application_context_t* context, uint8_t* data, uint8_t data_len)
{
    context->settings->saved_color.h = context->current_color_description->first_component;
    context->settings->saved_color.s = (uint8_t)context->current_color_description->second_component;
    context->settings->saved_color.v = (uint8_t)context->current_color_description->third_component;
    nvram_save_settings((uint32_t*)context->settings, sizeof(*(context->settings)));
    char msg[100];
    snprintf(msg, sizeof(msg),
                "Current color successfully saved\r\n");    
    //usb_serial_dumb_print(msg, strlen(msg));
    return 0;
}


COMMAND_DEFINITION save_current_color_command =
{
        .command_type = CMD_SAVE_CURRENT_COLOR,
        .name = "SAVE_COLOR",
        .description = "SAVE_COLOR - save current LED color to NVRAM.\r\n",
        .executor = save_current_color
};

#endif