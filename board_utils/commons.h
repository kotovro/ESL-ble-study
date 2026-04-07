
#ifndef CONFIG_H
#define CONFIG_H
#include <stdint.h>
#include <stdbool.h>
#include "nrf.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"
#include "app_usbd.h"
#include "app_usbd_serial_num.h"
#include "app_usbd_cdc_acm.h"
#include "app_timer.h"
#include "nrfx_clock.h"

#define CURRENT_VERSION ((uint32_t)7)
#define AVAILABLE_COMMANDS_SLOTS 9
#define AVAILABLE_COLOR_SLOTS 10

// --- Button settings ---
#define BUTTON_PIN          38
#define BUTTON_PRESSED       0
#define DEBOUNCE_TIME_MS    50
#define DOUBLE_CLICK_MS    400

// --- Modes ---
#define SLEEP               0
#define PICKING_HUE         1
#define PICKING_SATURATION  2
#define PICKING_VALUE       3
#define POWER_OFF           4

// --- Color ---
#define COLOR_CHANGE_MS    100
#define COLOR_CHANGE_DELAY (DOUBLE_CLICK_MS + 50)

// --- Fade steps ---
// Allow user override
#ifndef FADE_STEPS
#define FADE_STEPS         32
#endif

#define STEP_OF_COLOR_CHANGE 1

// --- Directions ---
#define INCREASE true
#define DECREASE false

#define BLE_COMMAND_SUCCESS 0
#define BLE_COMMAND_IN_PROCESS 1
#define BLE_COMMAND_NOT_RECOGNIZED 2
#define BLE_COMMAND_BUSY 3


typedef struct {
    // 0 - RGB
    // 1 - HSV
    // 255 - Unknown
    char colorType; 
    char colorName[11];
    uint16_t first_component;
    uint8_t second_component;
    uint8_t third_component;
} COLOR_DESCRIPTION;

typedef struct {
    uint16_t h;
    uint8_t s;
    uint8_t v;
} COLOR_HSV;

typedef struct {
    uint16_t r;
    uint16_t g;
    uint16_t b;
} COLOR_RGB;

typedef struct {
    uint32_t version;
    COLOR_HSV saved_color;
    COLOR_DESCRIPTION color_palette[AVAILABLE_COLOR_SLOTS]; 
} SETTINGS;

typedef struct 
{
    int* mode_global;
    COLOR_DESCRIPTION* color_palette; 
    COLOR_DESCRIPTION* current_color_description;
    SETTINGS* settings;
    bool * hue_d;
    bool * saturation_d;
    bool * value_d;
} COMMAND_CONTEXT;



#define MAX_PWM_VALUE 1024 
// Memory address for NVRAM storage

#ifndef BOOTLOADER_START_ADDR
#define BOOTLOADER_START_ADDR (NRF_UICR->NRFFW[0]) 
#endif
#define PAGE_SIZE  0x1000
#define NRF_DFU_APP_DATA_PAGES 3

#define APPDATA_START_ADDR  (BOOTLOADER_START_ADDR - (NRF_DFU_APP_DATA_PAGES * PAGE_SIZE))
#define VERSION_SETTING_ADDRESS (APPDATA_START_ADDR)

// USB CDC ACM settings
#define CDC_ACM_COMM_INTERFACE  2
#define CDC_ACM_COMM_EPIN       NRF_DRV_USBD_EPIN3

#define CDC_ACM_DATA_INTERFACE  3
#define CDC_ACM_DATA_EPIN       NRF_DRV_USBD_EPIN4
#define CDC_ACM_DATA_EPOUT      NRF_DRV_USBD_EPOUT4
#define READ_SIZE               1
#define MAX_COMMAND_SIZE        100
#define MAX_BLE_COMMAND_LENGTH  20
#endif // CONFIG_H
