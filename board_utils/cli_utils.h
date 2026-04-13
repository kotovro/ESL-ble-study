#ifndef CLI_UTILS_H
#define CLI_UTILS_H

#include "commons.h"


#define CMD_SET_RGB 0
#define CMD_SET_HSV 1
#define CMD_HELP 2
// #define CMD_ADD_RGB 3
// #define CMD_ADD_HSV 4
// #define CMD_APPLY_COLOR 5
// #define CMD_LIST_COLORS 6
// #define CMD_SAVE_COLORS 7
#define CMD_POWER_SWITCH 3
#define CMD_SAVE_SETTINGS 8
#define CMD_UNKNOWN 255 
#define ERROR_MESSAGE "Unknown command\r\n"
    

typedef int  (*command_executor)(char*, application_context_t*, uint8_t*, uint8_t);
typedef void (*Command_Finalizer)(void);

typedef struct
{
    bool is_required;
    uint16_t  min_value;
    uint16_t  max_value;
} ARG;

typedef struct
{
    uint8_t command_type;
    command_executor executor;    
    char* name;
    char* description;
    Command_Finalizer finalizer;
} command_definition_t;




void init_usb_cli(command_definition_t* known_commands, size_t known_commands_size,
                command_executor default_command, application_context_t* application_context);
void usb_ev_handler(app_usbd_class_inst_t const * p_inst,
                           app_usbd_cdc_acm_user_event_t event);
void usb_serial_dumb_print(char const * p_buffer, size_t len);
#endif // CLI_UTILS_H