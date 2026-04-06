#include "button_executors.h"
#include "led_utils.h"

extern void save_current_color(char* args, COMMAND_CONTEXT* context);
static COMMAND_CONTEXT* m_application_context;

void init_button_executors(COMMAND_CONTEXT* application_context_ptr)
{
    m_application_context = application_context_ptr;
}

void button_press_executor() 
{
   if (m_application_context->current_color_description->colorType != 1) 
    {
        return; // Not in HSV mode
    }

    if (*(m_application_context->mode_global) == PICKING_HUE) {
        if (*(m_application_context->hue_d) == INCREASE) {
            m_application_context->current_color_description->first_component += STEP_OF_COLOR_CHANGE;
            if (m_application_context->current_color_description->first_component >= 360) {
                m_application_context->current_color_description->first_component = 360;
                *(m_application_context->hue_d) = DECREASE;
            }
        } else {
            m_application_context->current_color_description->first_component -= STEP_OF_COLOR_CHANGE;
            if ((int16_t)m_application_context->current_color_description->first_component <= 0) {
                m_application_context->current_color_description->first_component = 0;
                *(m_application_context->hue_d) = INCREASE;
            }
        }
    } 
    else if (*(m_application_context->mode_global) == PICKING_SATURATION) {
        if (*(m_application_context->saturation_d) == INCREASE) {
            m_application_context->current_color_description->second_component += STEP_OF_COLOR_CHANGE;
            if (m_application_context->current_color_description->second_component >= 100) {
                m_application_context->current_color_description->second_component = 100;
                *(m_application_context->saturation_d) = DECREASE;
            }
        } else {
            m_application_context->current_color_description->second_component -= STEP_OF_COLOR_CHANGE;
            if ((int8_t)m_application_context->current_color_description->second_component <= 0) {
                m_application_context->current_color_description->second_component = 0;
                *(m_application_context->saturation_d) = INCREASE;
            }
        }
    } 
    else if (*(m_application_context->mode_global) == PICKING_VALUE) {
        if (*(m_application_context->value_d) == INCREASE) {
            m_application_context->current_color_description->third_component += STEP_OF_COLOR_CHANGE;
            if (m_application_context->current_color_description->third_component >= 100) {
                m_application_context->current_color_description->third_component = 100;
                *(m_application_context->value_d) = DECREASE;
            }
        } else {
            m_application_context->current_color_description->third_component -= STEP_OF_COLOR_CHANGE;
            if ((int8_t)m_application_context->current_color_description->third_component <= 0) {
                m_application_context->current_color_description->third_component = 0;
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
        // save_current_color(NULL, m_application_context);
    }
    show_color(m_application_context->current_color_description);
}
