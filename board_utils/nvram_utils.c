#include <string.h>

#include "nrf_soc.h"
#include "fds.h"

#include "led_utils.h"
#include "nvram_utils.h"

#define FILE_ID         0x0001  /* The ID of the file to write the records into. */
#define RECORD_KEY_VERSION    0x1111  /* A key for the first record. */
#define RECORD_KEY_SETTINGS    0x2222  /* A key for the second record. */
static uint32_t application_version;
// Simple event handler to handle errors during initialization.
static void fds_evt_handler(fds_evt_t const * p_fds_evt)
{
    switch (p_fds_evt->id)
    {
        case FDS_EVT_INIT:
            if (p_fds_evt->result != NRF_SUCCESS)
            {
                // Initialization failed.
                NRF_LOG_INFO("Initialization of FDS failed. Error code: %d", p_fds_evt->result);
            }
            break;
        case FDS_EVT_WRITE:
            NRF_LOG_INFO("Event id is: %d, result is: %d", p_fds_evt->id, p_fds_evt->result);
            break;
        default:
            NRF_LOG_INFO("Event id is: %d, result is: %d", p_fds_evt->id, p_fds_evt->result);
            break;
    }
}

void nvram_init(void)
{
    ret_code_t ret = fds_register(fds_evt_handler);
    if (ret != NRF_SUCCESS)
    {
        // Registering of the FDS event handler has failed.
        NRF_LOG_INFO("Couldn't register FDS event handler. Error code:  %d", ret);
    }

    ret = fds_init();
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("Return %d", ret);
    }
    fds_gc();
}


void update_record(fds_record_t record)
{
    fds_record_desc_t   record_desc;
    fds_find_token_t    ftok;

    /* It is required to zero the token before first use. */
    memset(&ftok, 0x00, sizeof(fds_find_token_t));
    ret_code_t find_result = fds_record_find(FILE_ID, record.key, &record_desc, &ftok);
    if (find_result == NRF_SUCCESS)
    {
        NRF_LOG_INFO("Entry found");
        fds_record_update(&record_desc, &record);
    } else
    {
        NRF_LOG_INFO("Entry wasnt't found, code: %d, while FDS_ERR_NO_SPACE_IN_FLASH code is: %d", find_result, FDS_ERR_NO_SPACE_IN_FLASH);
        fds_record_write(&record_desc, &record);
    }
}

void update_version(uint32_t version)
{
    application_version = version;
    fds_record_t        record;

    // Set up record.
    record.file_id           = FILE_ID;
    record.key               = RECORD_KEY_VERSION;
    record.data.p_data       = &application_version;
    record.data.length_words = 1;   /* one word is four bytes. */

    update_record(record);
}

void read_record(uint32_t record_key, uint32_t* data_p, uint32_t data_size)
{
    fds_flash_record_t  flash_record;
    fds_record_desc_t   record_desc;
    fds_find_token_t    ftok;

    /* It is required to zero the token before first use. */
    memset(&ftok, 0x00, sizeof(fds_find_token_t));

    ret_code_t find_result = fds_record_find(FILE_ID, record_key, &record_desc, &ftok);
    if (find_result == NRF_SUCCESS)
    {
        fds_record_open(&record_desc, &flash_record);
		memcpy(data_p, flash_record.p_data, data_size);
		fds_record_close(&record_desc);
    } else
    {
        NRF_LOG_INFO("No such datato read. Key: %d", record_key);
    }

}

bool is_version_changed(uint32_t version)
{
    read_record(RECORD_KEY_VERSION, &application_version, sizeof(application_version));

    bool is_version_changed = (application_version != version);
    NRF_LOG_INFO(is_version_changed ? "Version changed, set default color" 
                                    : "Version not changed, load saved color");
    LOG_BACKEND_USB_PROCESS();
    return is_version_changed;
}

void nvram_save_settings(uint32_t* settings, size_t settings_size)
{
    fds_record_t        record;

    // Set up record.
    record.file_id           = FILE_ID;
    record.key               = RECORD_KEY_SETTINGS;
    record.data.p_data       = settings;
    record.data.length_words = (settings_size + 3) / 4;   /* one word is four bytes. */
    
    NRF_LOG_INFO("Will enter loop");
    update_record(record);

    NRF_LOG_INFO("Settings successfully saved");
    LOG_BACKEND_USB_PROCESS();
}

void nvram_load_settings(uint32_t* settings, size_t settings_size)
{
   read_record(RECORD_KEY_SETTINGS, settings, settings_size);   
}





