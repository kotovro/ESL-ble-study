#include "command_utils.h"

int print_help_message(char* args, application_context_t* context, uint8_t* data, uint8_t data_len)
{
    bool is_binary_arg = ((data != NULL) && (data_len == 4));
    char msg[1024];
    msg[0] = '\0';

    strncat(msg, "Supported commands:\r\n", sizeof(msg) - strlen(msg) - 1);
    for (size_t i = 0; i < sizeof(command_definitions) / sizeof(command_definition_t); i++)
    {
        if(command_definitions[i].command_type != CMD_UNKNOWN) 
        {
            strncat(msg, command_definitions[i].description, sizeof(msg) - strlen(msg) - 1);
        }
    }
    if (!is_binary_arg) {
        usb_serial_dumb_print(msg, strlen(msg));
    }
    return 0;
}

command_definition_t help_command = 
{
    .command_type = CMD_HELP,
    .name = "HELP",
    .description = "HELP - prints this message.\r\n",
    .executor = print_help_message
};