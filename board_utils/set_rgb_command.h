#include "command_utils.h"
#include "color_utils.h"

void set_rgb_executor(char* args, COMMAND_CONTEXT* context)
{
    char msg[100];
    COLOR_RGB color = 
    {
        .r = 0,
        .g = 0,
        .b = 0,
    };
    bool is_args_valid = true;
    int cur_pos = 0;
    if(!try_parse_int_arg(args, &cur_pos, &(color.r)) || color.r > 255){
        is_args_valid = false;
    }     
    if(!is_args_valid || !try_parse_int_arg(args, &cur_pos, &(color.g)) || color.g > 255){
        is_args_valid = false;
    } 
    
    if(!is_args_valid || !try_parse_int_arg(args, &cur_pos, &(color.b)) || color.b > 255){
        is_args_valid = false;
    }

    if (is_args_valid)
    {
        COLOR_HSV hsv_color = rgb_to_hsv(color);
        context->current_color_description->first_component = hsv_color.h;
        context->current_color_description->second_component = hsv_color.s;
        context->current_color_description->third_component = hsv_color.v;
        show_color(context->current_color_description);
        snprintf(msg, sizeof(msg),
                "Color set to: R=%u, G=%u, B=%u\r\n", color.r, color.g, color.b);
        
        usb_serial_dumb_print(msg, strlen(msg));
    }
    else 
    {
        unknown_command_executor(args, context);
        NRF_LOG_INFO("Invalid set rgb arguments detected: %s", args);
    }
}

COMMAND_DEFINITION set_rgb_command = 
{
    .command_type = CMD_SET_RGB,
    .name = "RGB",
    .description = "RGB <red> <green> <blue> - the device sets current color to specified one.\r\n",
    .executor = set_rgb_executor
};