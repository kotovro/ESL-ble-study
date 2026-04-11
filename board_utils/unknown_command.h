int unknown_command_executor(char* args, application_context_t* context, uint8_t* data, uint8_t data_len)
{
    bool is_binary_arg = ((data != NULL));
    static char msg[100];
    strcpy(msg, ERROR_MESSAGE);
    if (is_binary_arg) {
        NRF_LOG_INFO(ERROR_MESSAGE);
    } else {
        usb_serial_dumb_print(msg, strlen(msg));
    }
    return -1;
}