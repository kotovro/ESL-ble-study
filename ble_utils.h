#ifndef BlE_UTILS_H__
#define BLE_UTILS_H__

#include <stdint.h>

uint32_t send_notitification(uint16_t conn_handle, uint16_t char_value_handle, const uint8_t *data);
uint32_t send_indication(uint16_t conn_handle, uint16_t char_value_handle, const uint8_t *data);

#endif /* BlE_UTILS_H__ */