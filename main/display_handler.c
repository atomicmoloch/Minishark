#include <string.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "st7789.h"
#include "fontx.h"

static const char *TAG = "display handler";

uint8_t ascii_line[30] = {0};
char input_buf[128];

uint16_t xpos = 0;
uint16_t ypos = 0;
uint16_t text_color = WHITE;
uint16_t background_color = BLACK;
uint16_t margin = 10;
TFT_t dev;

FontxFile fx16G[2];
FontxFile fx24G[2];
FontxFile fx32G[2];
FontxFile fx32L[2];

//Buffered display write, allows strings of any reasonable length to be printed line-by-line
void display_write_line(FontxFile *fx, const char *str)
{
    uint8_t fontWidth;
    uint8_t fontHeight;
    GetFontx(fx, 0, &fontWidth, &fontHeight);
  //  ESP_LOGI(TAG, "Font is %u * %u", fontWidth, fontHeight);
    uint8_t max_chars = CONFIG_WIDTH / fontWidth;
    ascii_line[max_chars+1] = '\0';
    uint16_t slen = strlen(str);

    for (uint16_t i = 0; i < slen; i += max_chars) {
        ypos = ypos + fontHeight + margin;
        if (ypos >= CONFIG_HEIGHT) { //When end of screen is reached, fills screen with black and starts on first line again
            lcdFillScreen(&dev, background_color);
            ypos = fontHeight - 1;
        }
        strncpy((char *)ascii_line, str + i, max_chars);
        lcdDrawString(&dev, fx, xpos, ypos, ascii_line, text_color);
   //     ESP_LOGI(TAG, "New ypos %u", ypos);
    }

    lcdDrawFinish(&dev);
}

//Writes long strings to display, broken by "\n"
void display_write_full(FontxFile *fx, const char *str)
{

    ESP_LOGI(TAG, "Printing %s on screen", str);
    uint16_t slen = strlen(str);

    for (uint16_t i = 0; i < slen; i += 128) {

        strncpy(input_buf, str, 128);

        char *next_line = strtok(input_buf, "\n");

        if (next_line == NULL) {
            display_write_line(fx, input_buf);
        }

        while (next_line != NULL) {
            display_write_line(fx, next_line);
            next_line = strtok(NULL, "\n");
        }
    }
}

void display_write_16pt(const char *str)
{
    display_write_full(fx16G, str);
}

void display_write_24pt(const char *str)
{
    display_write_full(fx24G, str);
}

void display_write_32pt(const char *str)
{
    display_write_full(fx32G, str);
}

void display_set_text_color(uint8_t r, uint8_t g, uint8_t b)
{
    text_color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void display_set_background_color(uint8_t r, uint8_t g, uint8_t b)
{
    background_color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void display_init()
{
    InitFontx(fx16G,"/fonts/ILGH16XB.FNT",""); // 8x16Dot Gothic
    InitFontx(fx24G,"/fonts/ILGH24XB.FNT",""); // 12x24Dot Gothic
    InitFontx(fx32G,"/fonts/ILGH32XB.FNT",""); // 16x32Dot Gothic
    InitFontx(fx32L,"/fonts/LATIN32B.FNT",""); // 16x32Dot Latin

    //Initializes st7789 display driver
    spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO, CONFIG_BL_GPIO);
    lcdInit(&dev, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY);

    lcdFillScreen(&dev, background_color);
    lcdSetFontDirection(&dev, 0);
}

