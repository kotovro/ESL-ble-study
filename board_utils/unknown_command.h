void unknown_command_executor(char* args, COMMAND_CONTEXT* context, uint8_t* data, uint8_t data_len)
{
    char msg[100];
    strcpy(msg, ERROR_MESSAGE);
    usb_serial_dumb_print(msg, strlen(msg));
}