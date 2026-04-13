#include "command_utils.h"

int power_switch_executor(char* args, application_context_t* context, uint8_t* data, uint8_t data_len)
{
    bool is_binary = data != NULL;
    bool is_args_valid = !is_binary || !((*context->mode_global == POWER_OFF) && (*data == 0)) || ((*context->mode_global != POWER_OFF) && (*data == 1));
    if (is_args_valid)
    {
        if (*(context->mode_global) == POWER_OFF)
        {
            *(context->mode_global) = SLEEP;
        }
        else if (*(context->mode_global) != POWER_OFF)
        {
            *(context->mode_global) = POWER_OFF;
        }
        show_color(context->current_color_description);
    }
    if (!is_binary || !is_args_valid)
    {   
        NRF_LOG_INFO("Current led mode is: %d", *context->led_power_mode);
        *context->led_power_mode = !(*context->led_power_mode);
        NRF_LOG_INFO("Now led mode is: %d", *context->led_power_mode);
    }
    return 0;
}

command_definition_t power_switch_command  = 
{
        .command_type = CMD_POWER_SWITCH,
        .name = "POWER_SWITCH",
        .description = "POWER_SWITCH - changes led.\r\n",
        .executor =  power_switch_executor
};