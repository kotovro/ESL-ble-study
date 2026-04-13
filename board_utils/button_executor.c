#include "button_executors.h"
#include "led_utils.h"

extern void save_settings(char* args, application_context_t* context, uint8_t* data, uint8_t data_len);
static application_context_t* m_application_context;

void init_button_executors(application_context_t* application_context_ptr)
{
    m_application_context = application_context_ptr;
}

void button_press_executor() 
{
   if (m_application_context->current_color_description->colorType != 1) 
    {
        return; // Not in HSV mode
    }
    if (*(m_application_context->mode_global) == POWER_OFF)
    {
        *(m_application_context->mode_global) = SLEEP;
    } 

    color_hsv_t *hsv = &m_application_context->current_color_description->hsv;

    if (*(m_application_context->mode_global) == PICKING_HUE) {
        if (*(m_application_context->hue_d) == INCREASE) {
            hsv->h += STEP_OF_COLOR_CHANGE;
            if (hsv->h >= 360) {
                hsv->h = 360;
                *(m_application_context->hue_d) = DECREASE;
            }
        } else {
            hsv->h -= STEP_OF_COLOR_CHANGE;
            if ((int16_t)hsv->h <= 0) {
                hsv->h = 0;
                *(m_application_context->hue_d) = INCREASE;
            }
        }
    } 
    else if (*(m_application_context->mode_global) == PICKING_SATURATION) {
        if (*(m_application_context->saturation_d) == INCREASE) {
            hsv->s += STEP_OF_COLOR_CHANGE;
            if (hsv->s >= 100) {
                hsv->s = 100;
                *(m_application_context->saturation_d) = DECREASE;
            }
        } else {
            hsv->s -= STEP_OF_COLOR_CHANGE;
            if ((int8_t)hsv->s <= 0) {
                hsv->s = 0;
                *(m_application_context->saturation_d) = INCREASE;
            }
        }
    } 
    else if (*(m_application_context->mode_global) == PICKING_VALUE) {
        if (*(m_application_context->value_d) == INCREASE) {
            hsv->v += STEP_OF_COLOR_CHANGE;
            if (hsv->v >= 100) {
                hsv->v = 100;
                *(m_application_context->value_d) = DECREASE;
            }
        } else {
            hsv->v -= STEP_OF_COLOR_CHANGE;
            if ((int8_t)hsv->v <= 0) {
                hsv->v = 0;
                *(m_application_context->value_d) = INCREASE;
            }
        }
    }
    show_color(m_application_context->current_color_description);
}

void double_click_executor()
{   
    if (*(m_application_context->mode_global) == SLEEP)
    {
        *(m_application_context->mode_global) = PICKING_HUE;
    }
    else if (*(m_application_context->mode_global) == PICKING_HUE)
    {
        *(m_application_context->mode_global) = PICKING_SATURATION;
    }
    else if (*(m_application_context->mode_global) == PICKING_SATURATION)
    {
        *(m_application_context->mode_global) = PICKING_VALUE;
    }
    else if (*(m_application_context->mode_global) == PICKING_VALUE)
    {
        *(m_application_context->mode_global) = SLEEP;
        save_settings(NULL, m_application_context, NULL, 0);
    }
    show_color(m_application_context->current_color_description);
}
