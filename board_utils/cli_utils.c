#include "cli_utils.h"
#include <ctype.h>

static COMMAND_DEFINITION* m_command_definitions;
static Command_Executor m_default_executor;
static size_t m_command_definitions_size;
static COMMAND_CONTEXT* m_application_context;

#if ESTC_USB_CLI_ENABLED == 0
void init_usb_cli(COMMAND_DEFINITION* known_commands, size_t known_commands_size,
                Command_Executor default_command,  COMMAND_CONTEXT* application_context) {}
void usb_serial_dumb_print(char const * p_buffer, size_t len) {}
#else 

static char m_command_buffer[MAX_COMMAND_SIZE];
static char m_rx_buffer[READ_SIZE];
static volatile bool m_tx_done = true;

APP_TIMER_DEF(parse_command_timer_id);

APP_USBD_CDC_ACM_GLOBAL_DEF(usb_cdc_acm,
                        usb_ev_handler,
                        CDC_ACM_COMM_INTERFACE,
                        CDC_ACM_DATA_INTERFACE,
                        CDC_ACM_COMM_EPIN,
                        CDC_ACM_DATA_EPIN,
                        CDC_ACM_DATA_EPOUT,                   
                        APP_USBD_CDC_COMM_PROTOCOL_NONE);


static void parse_command_timer_handler(void * p_context);
void init_usb_cli(COMMAND_DEFINITION* known_commands, size_t known_commands_size,
                Command_Executor default_command, COMMAND_CONTEXT* application_context)
{                       
    m_command_definitions = known_commands;
    m_command_definitions_size = known_commands_size;
    m_default_executor = default_command;
    m_application_context = application_context;
    app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&usb_cdc_acm);
    ret_code_t ret = app_usbd_class_append(class_cdc_acm);
    APP_ERROR_CHECK(ret);
    
    ret = app_timer_create(&parse_command_timer_id,
                            APP_TIMER_MODE_SINGLE_SHOT,
                            parse_command_timer_handler);
    APP_ERROR_CHECK(ret);
}

bool is_command_found(COMMAND_DEFINITION cmd_def)
{
    if (strlen(m_command_buffer) < strlen(cmd_def.name)) 
    {
        return false;
    }
    for (size_t i = 0; i < strlen(cmd_def.name); i++) 
    {
        if (tolower((unsigned char) m_command_buffer[i]) != tolower((unsigned char) cmd_def.name[i])) 
        {
            return false;
        }
    }
    if (m_command_buffer[strlen(cmd_def.name)] != ' ' &&
        m_command_buffer[strlen(cmd_def.name)] != '\0') 
    {
        return false;
    }
    return true;
}

void get_argument_string(char* arguments, size_t cmd_name_length, int max_argument_length)
{
    int cur_pos = 0;
    
    while(m_command_buffer[cmd_name_length + cur_pos] == ' ')
    {
        cur_pos++;
    }
    int i = 0;
    while(m_command_buffer[cmd_name_length + cur_pos] != '\0' && i < max_argument_length - 2)
    {
        arguments[i] = m_command_buffer[cmd_name_length + cur_pos];
        cur_pos++;
        i++;
    }
    arguments[i] = '\0';
    
}

void usb_serial_dumb_print(char const * p_buffer, size_t len)
{
    ret_code_t ret = app_usbd_cdc_acm_write(&usb_cdc_acm,
                                            p_buffer,
                                            len);
    APP_ERROR_CHECK(ret);
}


void parse_command()
{
    Command_Executor executor = m_default_executor;
    char arguments[32];
    size_t cmd_name_length = 0;

    bool is_command_recognized = false;
    for (int i = 0; i < m_command_definitions_size; i++) 
    {
        if (is_command_found(m_command_definitions[i])) 
        {
            executor = m_command_definitions[i].executor;
            cmd_name_length = strlen(m_command_definitions[i].name);
            is_command_recognized = true;
            break;
        }
    }
    if (is_command_recognized) 
    {
        get_argument_string(arguments, cmd_name_length, 32);
    }
    executor(arguments, m_application_context, NULL, 0);
}

static void parse_command_timer_handler(void * p_context)
{
    // NRF_LOG_INFO("We will try to parse, m_counter = %d", m_counter);
    parse_command();

    m_command_buffer[0] = '\0';
}

void usb_ev_handler(app_usbd_class_inst_t const * p_inst,
                           app_usbd_cdc_acm_user_event_t event)
{
    switch (event)
    {
    case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
    {
        m_command_buffer[0] = '\0';
        ret_code_t ret;
        ret = app_usbd_cdc_acm_read(&usb_cdc_acm, m_rx_buffer, READ_SIZE);
        UNUSED_VARIABLE(ret);
        break;
    }
    case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
    {
        break;
    }
    case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
    {
        m_tx_done = true;
        NRF_LOG_INFO("tx done");
        break;
    }
    case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
    {
        ret_code_t ret;
        do
        {
            /*Get amount of data transfered*/
            size_t size = app_usbd_cdc_acm_rx_size(&usb_cdc_acm);
            UNUSED_VARIABLE(size);
            NRF_LOG_INFO("rx: %d", m_rx_buffer[0]);
            /* It's the simple version of an echo. Note that writing doesn't
             * block execution, and if we have a lot of characters to read and
             * write, some characters can be missed.
             */
            if (m_rx_buffer[0] == '\r' || m_rx_buffer[0] == '\n')
            {
                ret = app_usbd_cdc_acm_write(&usb_cdc_acm, "\r\n", 2);
                m_command_buffer[strlen(m_command_buffer)] = '\0';
                NRF_LOG_INFO("Received command len: %d", strlen(m_command_buffer));
                app_timer_start(parse_command_timer_id, 5, NULL);
            }
            else
            {
                ret = app_usbd_cdc_acm_write(&usb_cdc_acm,
                                             m_rx_buffer,
                                             READ_SIZE);

                int cur_len = strlen(m_command_buffer);
                if (cur_len < MAX_COMMAND_SIZE - 1)
                {
                    m_command_buffer[cur_len] = m_rx_buffer[0];
                    m_command_buffer[cur_len + 1] = '\0';
                }                         
            }

            /* Fetch data until internal buffer is empty */
            ret = app_usbd_cdc_acm_read(&usb_cdc_acm,
                                        m_rx_buffer,
                                        READ_SIZE);
        } while (ret == NRF_SUCCESS);

        break;
    }
    default:
        break;
    }
}

#endif