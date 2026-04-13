#include "command_utils.h"

int set_hsv_executor(char* args, application_context_t* context, uint8_t* data, uint8_t data_len)
{
    static char msg[100];
    color_hsv_t color = 
    {
        .h = 0,
        .s = 0,
        .v = 0,
    };
    
    int cur_pos = 0;
    bool is_binary_arg = ((data != NULL) && (data_len == 4));
    bool is_args_valid = (((args != NULL) && (strlen(args) > 0)) || is_binary_arg);
    
    NRF_LOG_INFO("data isn;t null=%d", data != NULL);
    NRF_LOG_INFO("data len is=%d", data_len);
   
    if (is_args_valid){
        if (is_binary_arg) 
        {
            NRF_LOG_INFO("H=%u, S=%u, V=%u", color.h, color.s, color.v);
            color.h = (uint16_t)data[0] | (uint16_t)(data[1] << 8);
            color.s = data[2];
            color.v = data[3];
        } else {
            if(!try_parse_int_arg(args, &cur_pos, &(color.h))){
                is_args_valid = false;
            } 
        }
        if (is_args_valid){
            is_args_valid = color.h <= 360;
        }
    }

    uint16_t parsed_value = 0;
    if (is_args_valid){
        if (!is_binary_arg){
            if (!try_parse_int_arg(args, &cur_pos, &parsed_value)){
                is_args_valid = false;
            } else {
                color.s = (uint8_t)parsed_value;
            }
        }
        if (is_args_valid){
             is_args_valid = color.s <= 100;
        }
    }

    if (is_args_valid){
        if (!is_binary_arg){
            if (!try_parse_int_arg(args, &cur_pos, &parsed_value)){
                is_args_valid = false;
            } else {
                color.v = (uint8_t)parsed_value;
            }
        }
        if (is_args_valid){
             is_args_valid = color.v <= 100;
        }
    }

    if (is_args_valid)
    {
        context->current_color_description->colorType = 1; // HSV
        context->current_color_description->hsv = color;
        ///for power off 
        // if ((context->current_color_description->first_component == 0) 
        //     && (context->current_color_description->second_component == 0) 
        //     && (context->current_color_description->third_component == 0) 
        //     && (*context->led_power_mode == LED_ON))
        //     {
        //         *context->led_power_mode = !(*context->led_power_mode);
        //         *context->mode_global = POWER_OFF;
        //     }
        
        // NRF_LOG_INFO("Cuurent mode is: %d", *context->led_power_mode);
        // if (((context->current_color_description->first_component != 0) 
        //     || (context->current_color_description->second_component != 0) 
        //     || (context->current_color_description->third_component != 0)) 
        //     && (*context->led_power_mode == LED_OFF))
        //     {
        //         *context->led_power_mode = !(*context->led_power_mode);
        //         *context->mode_global = SLEEP;
        //          NRF_LOG_INFO("Cuurent mode is: %d", *context->led_power_mode);
        //          NRF_LOG_INFO("Current glbal mode: %d", *context->mode_global);
        //     }
        
        show_color(context->current_color_description);
        
        if (is_binary_arg) {
            NRF_LOG_INFO("Color set to: H=%u, S=%u, V=%u", color.h, color.s, color.v);
        } else {
            snprintf(msg, sizeof(msg),
                "Color set to: H=%u, S=%u, V=%u\r\n", color.h, color.s, color.v);

            usb_serial_dumb_print(msg, strlen(msg));
        }
        return 0;
    }
    else 
    {
        unknown_command_executor(args, context, data, data_len);
        if (is_binary_arg) {
            NRF_LOG_INFO("Invalid set hsv binary arguments detected: H=%u, S=%u, V=%u", color.h, color.s, color.v);
        } else {
            NRF_LOG_INFO("Invalid set hsv arguments detected: %s", args);
        }
        return -1;
    }
    
}


command_definition_t set_hsv_command  = 
{
        .command_type = CMD_SET_HSV,
        .name = "HSV",
        .description = "HSV <hue> <saturation> <value> - the same with RGB, but color is specified in HSV.\r\n",
        .executor =  set_hsv_executor
};