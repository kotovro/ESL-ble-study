#ifndef BUTTON_EXECUTORS_H
#define BUTTON_EXECUTORS_H

#include "commons.h"

void init_button_executors(application_context_t* application_context_ptr);
void button_press_executor();
void double_click_executor();

#endif