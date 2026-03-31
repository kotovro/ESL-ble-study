#include <math.h>
#include "commons.h"


COLOR_HSV rgb_to_hsv(COLOR_RGB rgb) 
{
    float r = rgb.r / 255.0f;
    float g = rgb.g / 255.0f;
    float b = rgb.b / 255.0f;

    float max = fmaxf(r, fmaxf(g, b));
    float min = fminf(r, fminf(g, b));
    float delta = max - min;

    COLOR_HSV hsv;
    // Hue calculation
    if (delta < 0.00001f) {
        hsv.h = 0;
    } else if (max == r) {
        hsv.h = 60.0f * fmodf(((g - b) / delta), 6.0f);
    } else if (max == g) {
        hsv.h = 60.0f * (((b - r) / delta) + 2.0f);
    } else {
        hsv.h = 60.0f * (((r - g) / delta) + 4.0f);
    }
    if (hsv.h < 0) {
        hsv.h += 360.0f;
    }

    // Saturation calculation
    hsv.s = (max <= 0.0f) ? 0 : (delta / max) * 100.0f;

    // Value calculation
    hsv.v = max * 100.0f;

    return hsv;
}

COLOR_RGB hsv_to_rgb(COLOR_HSV hsv) 
{
    float r, g, b;
    float hf = hsv.h / 60.0f;
    float sf = hsv.s / 100.0f;
    float vf = hsv.v / 100.0f;
    int i = (int)hf % 6;
    float f = hf - i;
    float p = vf * (1 - sf);
    float q = vf * (1 - f * sf);
    float t = vf * (1 - (1 - f) * sf);

    switch (i) {
        case 0: r = vf; g = t; b = p; break;
        case 1: r = q; g = vf; b = p; break;
        case 2: r = p; g = vf; b = t; break;
        case 3: r = p; g = q; b = vf; break;
        case 4: r = t; g = p; b = vf; break;
        case 5: r = vf; g = p; b = q; break;
        default: r = g = b = 0; break;
    }
    COLOR_RGB result_COLOR_RGB;
    result_COLOR_RGB.r = (uint16_t)(r * 255);
    result_COLOR_RGB.g = (uint16_t)(g * 255);
    result_COLOR_RGB.b = (uint16_t)(b * 255);
    return result_COLOR_RGB;
}
