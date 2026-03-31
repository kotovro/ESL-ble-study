#ifndef COMMAND_UTILS_H
#define COMMAND_UTILS_H

#include "commons.h"
COMMAND_DEFINITION command_definitions[AVAILABLE_COMMANDS_SLOTS];

 
void init_color_palette(COMMAND_CONTEXT* context)
{
    for (int i = 0; i < AVAILABLE_COLOR_SLOTS; i++) 
    {
        context->color_palette[i].colorType = 255; // Unknown
        context->color_palette[i].colorName[0] = '\0';
        context->color_palette[i].first_component = 0;
        context->color_palette[i].second_component = 0;
        context->color_palette[i].third_component = 0;
    }
}

void init_command_definitions()
{
    for (int i = 0; i < AVAILABLE_COMMANDS_SLOTS; i++) 
    {
        command_definitions[i].command_type = CMD_UNKNOWN;
        command_definitions[i].name[0] = '\0';
        command_definitions[i].description[0] = '\0';
        command_definitions[i].executor = NULL;
    }
}

bool try_parse_int_arg(char * carg, int * pos, uint16_t * arg)
{
    uint16_t parsed_arg = 0;
    int max_pos = strlen(carg);
    while((*pos < max_pos) && (carg[*pos] == ' '))
        *pos = *pos + 1;

    int digits = 0;
    while(*pos < max_pos && carg[*pos] != ' ')
    {
        if (carg[*pos] < '0' || carg[*pos] > '9')
            return false; // not a digit
        if (++digits > 3)
            return false; // too big number
        parsed_arg = parsed_arg * 10 + (uint16_t)(carg[*pos] - '0');
        *pos = *pos + 1;
    }

    *arg = parsed_arg;
    return digits > 0;
}

bool try_parse_str_arg(char* carg, int* pos, char* output, int max_len)
{
    int max_pos = strlen(carg);
    while((*pos < max_pos) && (carg[*pos] == ' '))
        *pos = *pos + 1;

    int i = 0;
    while(*pos < max_pos && carg[*pos] != ' ' && i < max_len - 1)
    {
        output[i] = carg[*pos];
        i++;
        *pos = *pos + 1;
    }
    output[i] = '\0';
    return (i > 0) && (carg[*pos] == ' ' || carg[*pos] == '\0');
}



#endif // COMMAND_UTILS_H