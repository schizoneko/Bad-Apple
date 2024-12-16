#ifndef SH1106_H
#define SH1106_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

// I2C configuration
#define SH1106_I2C_ADDR         0x3C    // Default I2C address for SH1106
#define I2C_MASTER_SDA_IO       21      // GPIO for SDA
#define I2C_MASTER_SCL_IO       22      // GPIO for SCL
#define I2C_MASTER_NUM          I2C_NUM_0
#define I2C_MASTER_FREQ_HZ      100000  // I2C master clock frequency

// Display dimensions
#define SH1106_WIDTH            128
#define SH1106_HEIGHT           64

// Command macros
#define SH1106_CMD_DISPLAY_OFF  0xAE
#define SH1106_CMD_DISPLAY_ON   0xAF
#define SH1106_CMD_SET_PAGE     0xB0

// SH1106 display structure
typedef struct {
    i2c_port_t i2c_num;
    uint8_t address;
    int sda_io;
    int scl_io;
    uint8_t buffer[SH1106_WIDTH * SH1106_HEIGHT / 8];  // Frame buffer
    int width;
    int height;
} sh1106_t;

// Function prototypes
esp_err_t sh1106_init_desc(sh1106_t *oled, i2c_port_t i2c_num, int sda_io, int scl_io);
esp_err_t sh1106_init(sh1106_t *oled);
void sh1106_display_on(sh1106_t *oled);
void sh1106_display_off(sh1106_t *oled);
void sh1106_clear_display(void);
void sh1106_draw_pixel(sh1106_t *oled, int x, int y, uint8_t color);
void sh1106_display_update(void);
void sh1106_clear(sh1106_t *oled);
void sh1106_draw_bitmap(sh1106_t *oled, int x, int y, const uint8_t *bitmap, int w, int h);
void sh1106_update(sh1106_t *oled);

#endif  // SH1106_H
