#ifndef NVRAM_UTILS
#define NVRAM_UTILS
#include "commons.h"

void nvram_init(void);
void update_version(uint32_t version);
bool is_version_changed(uint32_t version);
void nvram_save_settings(uint32_t* settings, size_t settings_size);
void nvram_load_settings(uint32_t* settings, size_t settings_size);
#endif