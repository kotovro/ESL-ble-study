#include "board_utils.h"
#include "nrfx_pwm.h"
#include "led_utils.h"
#include "color_utils.h"

application_context_t* m_application_context;
nrf_pwm_values_individual_t led_seq[FADE_STEPS];

void show_color(COLOR_DESCRIPTION* color) 
{
    if (color->colorType == 0) // RGB
    {
        NRF_LOG_INFO("Showing color: type=RGB, r=%u, g=%u, b=%u",
                    color->rgb.r, color->rgb.g, color->rgb.b);
    }
    else if (color->colorType == 1) // HSV
    {
        NRF_LOG_INFO("Showing color: type=HSV, h=%u, s=%u, v=%u",
                    color->hsv.h, color->hsv.s, color->hsv.v);
    }
    LOG_BACKEND_USB_PROCESS();

    color_rgb_t rgb_color;
    if (color->colorType == 0) // RGB
    {
        rgb_color = color->rgb;
    }
    else if (color->colorType == 1) // HSV
    {
        rgb_color = hsv_to_rgb(color->hsv);
    }
    else
    {
        return; // Unknown color type
    }
    show_rgb_color(rgb_color);
}


void show_rgb_color(color_rgb_t color) 
{
    if (*(m_application_context->mode_global) == SLEEP)
    {
        for (int i = 0; i < FADE_STEPS; ++i) 
        {
            led_seq[i].channel_1 = color.r / 255.f * MAX_PWM_VALUE;
            led_seq[i].channel_2 = color.g / 255.f * MAX_PWM_VALUE;
            led_seq[i].channel_3 = color.b / 255.f * MAX_PWM_VALUE;
        }
    }
    else if (*(m_application_context->mode_global) == PICKING_HUE)
    {
        for (int i = 0; i < FADE_STEPS; ++i) 
        {
            led_seq[i].channel_1 = i % 2 ? 0 : color.r / 255.f * MAX_PWM_VALUE;
            led_seq[i].channel_2 = i % 2 ? 0 : color.g / 255.f * MAX_PWM_VALUE;
            led_seq[i].channel_3 = i % 2 ? 0 : color.b / 255.f * MAX_PWM_VALUE;
        }
    }
    else if (*(m_application_context->mode_global) == PICKING_SATURATION)
    {
        for (int i = 0; i < FADE_STEPS; i++) 
        {
            led_seq[i].channel_1 =
            (i <= FADE_STEPS / 2)
            ? (i * color.r / 255.f * MAX_PWM_VALUE) / (FADE_STEPS / 2)
            : ((FADE_STEPS - i) * color.r / 255.f * MAX_PWM_VALUE) / (FADE_STEPS / 2);
    
            led_seq[i].channel_2 = (i <= FADE_STEPS / 2)
            ? (i * color.g / 255.f * MAX_PWM_VALUE) / (FADE_STEPS / 2)
            : ((FADE_STEPS - i) * color.g / 255.f * MAX_PWM_VALUE) / (FADE_STEPS / 2);
    
            led_seq[i].channel_3 = (i <= FADE_STEPS / 2)
            ? (i * color.b / 255.f * MAX_PWM_VALUE) / (FADE_STEPS / 2)
            : ((FADE_STEPS - i) * color.b / 255.f * MAX_PWM_VALUE) / (FADE_STEPS / 2);

        }
    }
    else if (*(m_application_context->mode_global) == PICKING_VALUE)
    {
        // save_current_color(NULL, m_application_context);
        for (int i = 0; i < (FADE_STEPS - 2); ++i) 
        {
            led_seq[i].channel_1 = color.r / 255.f * MAX_PWM_VALUE;
            led_seq[i].channel_2 = color.g / 255.f * MAX_PWM_VALUE;
            led_seq[i].channel_3 = color.b / 255.f * MAX_PWM_VALUE;
        }

        for (int i = (FADE_STEPS - 2); i < FADE_STEPS; ++i) 
        {
            led_seq[i].channel_1 = 0;
            led_seq[i].channel_2 = 0;
            led_seq[i].channel_3 = 0;
        }
    }
    else if (*(m_application_context->mode_global) == POWER_OFF)
    {
        for (int i = 0; i < FADE_STEPS; ++i) 
        {
            led_seq[i].channel_1 = 0;
            led_seq[i].channel_2 = 0;
            led_seq[i].channel_3 = 0;
        }
    }
}

// -------------------- PWM LED control --------------------
static const uint32_t m_led_pins[LED_COUNT] = {
    NRF_GPIO_PIN_MAP(0,6),
    NRF_GPIO_PIN_MAP(0,8),
    NRF_GPIO_PIN_MAP(1,9),
    NRF_GPIO_PIN_MAP(0,12) 
};

nrfx_pwm_config_t config_pwm0 =
{
    .output_pins =
    {
        m_led_pins[0] | NRFX_PWM_PIN_INVERTED,
        m_led_pins[1] | NRFX_PWM_PIN_INVERTED,
        m_led_pins[2] | NRFX_PWM_PIN_INVERTED,
        m_led_pins[3] | NRFX_PWM_PIN_INVERTED
    },
    .irq_priority = NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY,
    .base_clock   = NRF_PWM_CLK_1MHz,
    .count_mode   = NRF_PWM_MODE_UP,
    .top_value    = MAX_PWM_VALUE,             
    .load_mode    = NRF_PWM_LOAD_INDIVIDUAL,
    .step_mode    = NRF_PWM_STEP_AUTO
};

#if defined(BOARD_PCA10059)
/** 
 * This part was taken form boards file
 * Function for configuring UICR_REGOUT0 register
 * to set GPIO output voltage to 3.0V.
 */
static void gpio_output_voltage_setup(void)
{
    // Configure UICR_REGOUT0 register only if it is set to default value.
    if ((NRF_UICR->REGOUT0 & UICR_REGOUT0_VOUT_Msk) ==
        (UICR_REGOUT0_VOUT_DEFAULT << UICR_REGOUT0_VOUT_Pos))
    {
        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}

        NRF_UICR->REGOUT0 = (NRF_UICR->REGOUT0 & ~((uint32_t)UICR_REGOUT0_VOUT_Msk)) |
                            (UICR_REGOUT0_VOUT_3V0 << UICR_REGOUT0_VOUT_Pos);

        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}

        // System reset is needed to update UICR registers.
        NVIC_SystemReset();
    }
}
#endif

void init_leds_init(application_context_t* context)
{
    #if defined(BOARD_PCA10059)
    // If nRF52 USB Dongle is powered from USB (high voltage mode),
    // GPIO output voltage is set to 1.8 V by default, which is not
    // enough to turn on green and blue LEDs. Therefore, GPIO voltage
    // needs to be increased to 3.0 V by configuring the UICR register.
    if (NRF_POWER->MAINREGSTATUS &
       (POWER_MAINREGSTATUS_MAINREGSTATUS_High << POWER_MAINREGSTATUS_MAINREGSTATUS_Pos))
    {
        gpio_output_voltage_setup();
    }
    #endif
    m_application_context = context;
}

static nrfx_pwm_t m_pwn_status_led = NRFX_PWM_INSTANCE(2);


nrf_pwm_sequence_t seq_smooth = {
    .values.p_individual = led_seq,
    .length = 4 * FADE_STEPS,
    .repeats = 50,
    .end_delay = 0
};

void pattern_off(void) {
    for (int i = 0; i < FADE_STEPS; i++) {
        led_seq[i].channel_0 = 0;
    }
}

void pattern_slow_blinking(void) {
    for (int i = 0; i < FADE_STEPS; i++) {
        led_seq[i].channel_0 =
        (i <= FADE_STEPS / 2)
        ? (i * MAX_PWM_VALUE) / (FADE_STEPS / 2)
        : ((FADE_STEPS - i) * MAX_PWM_VALUE) / (FADE_STEPS / 2);
    }
}

void pattern_rapid_blinking(void) {
    for (int i = 0; i < FADE_STEPS; i++) {
        led_seq[i].channel_0 = i % 2 ? 0 : MAX_PWM_VALUE;
    }      
}

void pattern_on(void) {
    for (int i = 0; i < FADE_STEPS; i++)
        led_seq[i].channel_0 = MAX_PWM_VALUE;
}



// -------------------- Init PWM --------------------
void init_pwm_leds(COLOR_DESCRIPTION* color) {
    nrfx_pwm_init(&m_pwn_status_led, &config_pwm0, NULL);
    show_color(color);
    nrfx_pwm_simple_playback(&m_pwn_status_led, &seq_smooth, 1, NRFX_PWM_FLAG_LOOP);
}