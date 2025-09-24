#ifndef DISPLAY_HANDLER_H
#define DISPLAY_HANDLER_H

#include <stdint.h>

// Display handler API

// Initialize fonts and display. Assumes a spiffs partition containing the relevant files has already been mounted.
void display_init(void);

// Writes string at different font sizes
void display_write_16pt(const char *str);
void display_write_24pt(const char *str);
void display_write_32pt(const char *str);

// Set RGB888 colors
void display_set_text_color(uint8_t r, uint8_t g, uint8_t b);
void display_set_background_color(uint8_t r, uint8_t g, uint8_t b);

#endif
