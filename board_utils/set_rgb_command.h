#include "command_utils.h"
#include "color_utils.h"

int set_rgb_executor(char* args, COMMAND_CONTEXT* context, uint8_t* data, uint8_t data_len)
{
    char msg[100];
    COLOR_RGB color = 
    {
        .r = 0,
        .g = 0,
        .b = 0,
    };
    bool is_binary_arg = ((data != NULL) && (data_len == 3));
    bool is_args_valid = (((args != NULL) && (strlen(args) > 0)) || is_binary_arg);
    int cur_pos = 0;
    if (is_args_valid){
        if (is_binary_arg) 
        {
            color.r = data[0];
            color.g = data[1];
            color.b = data[2];
        } else {
            if(!try_parse_int_arg(args, &cur_pos, &(color.r))){
                is_args_valid = false;
            } 
        }
        if (is_args_valid){
            is_args_valid = color.r <= 255;
        }
    }
    
    if (is_args_valid){
        if (!is_binary_arg){
            if (!try_parse_int_arg(args, &cur_pos, &(color.g))){
                is_args_valid = false;
            }
        }
        if (is_args_valid){
             is_args_valid = color.g <= 255;
        }
    }

    if (is_args_valid){
        if (!is_binary_arg){
            if (!try_parse_int_arg(args, &cur_pos, &(color.b))){
                is_args_valid = false;
            }
        }
        if (is_args_valid){
             is_args_valid = color.b <= 255;
        }
    }

    if (is_args_valid)
    {
        COLOR_HSV hsv_color = rgb_to_hsv(color);
        context->current_color_description->colorType = 1; // HSV
        context->current_color_description->first_component = hsv_color.h;
        context->current_color_description->second_component = hsv_color.s;
        context->current_color_description->third_component = hsv_color.v;
        show_color(context->current_color_description);
        snprintf(msg, sizeof(msg),
                "Color set to: R=%u, G=%u, B=%u\r\n", color.r, color.g, color.b);
        
        if (is_binary_arg) {
            NRF_LOG_INFO("Color set to: R=%u, G=%u, B=%u", color.r, color.g, color.b);
        } else {
            usb_serial_dumb_print(msg, strlen(msg));
        }
        return 0;
    }
    else 
    {
        unknown_command_executor(args, context, data, data_len);
        if (is_binary_arg) {
            NRF_LOG_INFO("Invalid set rgb binary arguments detected: R=%u, G=%u, B=%u", color.r, color.g, color.b);
        } else {
            NRF_LOG_INFO("Invalid set rgb arguments detected: %s", args);
        }
        return -1;
    }
}

COMMAND_DEFINITION set_rgb_command = 
{
    .command_type = CMD_SET_RGB,
    .name = "RGB",
    .description = "RGB <red> <green> <blue> - the device sets current color to specified one.\r\n",
    .executor = set_rgb_executor
};