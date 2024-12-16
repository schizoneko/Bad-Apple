#include "sh1106.h"
#include "esp_log.h"
#include <string.h>
#include "driver/i2c_master.h"   // New I2C API
#include "freertos/FreeRTOS.h"   // For pdMS_TO_TICKS
#include "freertos/task.h"       // For FreeRTOS tasks

// Logging tag
static const char *TAG = "SH1106";

// Frame buffer for the display (128x64 pixels, 1 bit per pixel)
//static uint8_t sh1106_buffer[SH1106_WIDTH * SH1106_HEIGHT / 8];

// I2C bus handle and device handle
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;

// Initialize SH1106 display descriptor
esp_err_t sh1106_init_desc(sh1106_t *oled, i2c_port_t i2c_num, int sda_io, int scl_io) {
    if (bus_handle != NULL) {
        ESP_LOGW(TAG, "I2C bus already initialized. Releasing it first.");
        i2c_master_bus_rm_device(dev_handle);
        i2c_del_master_bus(bus_handle);
        bus_handle = NULL;
    }

    i2c_master_bus_config_t bus_conf = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = i2c_num,
        .scl_io_num = scl_io,
        .sda_io_num = sda_io,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t err = i2c_new_master_bus(&bus_conf, &bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus initialization failed: %s", esp_err_to_name(err));
        return err;
    }

    i2c_device_config_t dev_conf = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SH1106_I2C_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    err = i2c_master_bus_add_device(bus_handle, &dev_conf, &dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C device addition failed: %s", esp_err_to_name(err));
        return err;
    }

    oled->i2c_num = i2c_num;
    oled->address = SH1106_I2C_ADDR;
    oled->sda_io = sda_io;
    oled->scl_io = scl_io;
    oled->width = SH1106_WIDTH;
    oled->height = SH1106_HEIGHT;
    memset(oled->buffer, 0, sizeof(oled->buffer));

    return ESP_OK;
}

// Send a command to the display
esp_err_t sh1106_send_command(sh1106_t *oled, uint8_t command) {
    uint8_t data[] = {0x00, command};  // Command mode
    esp_err_t ret = i2c_master_transmit(dev_handle, data, sizeof(data), pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C command send failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

// SH1106 initialization
esp_err_t sh1106_init(sh1106_t *oled) {
    oled->width = SH1106_WIDTH;
    oled->height = SH1106_HEIGHT;
    memset(oled->buffer, 0, sizeof(oled->buffer));  // Clear the buffer

    // Initialization commands
    uint8_t init_cmds[] = {
        SH1106_CMD_DISPLAY_OFF,
        0x20, 0x00,  // Set Memory Addressing Mode
        0xB0,        // Set Page Start Address for Page Addressing Mode
        0xC8,        // Set COM Output Scan Direction
        0x00,        // Set low column address
        0x10,        // Set high column address
        0x40,        // Set start line address
        0x81, 0xFF,  // Set contrast control register
        0xA1,        // Set Segment Re-map
        0xA6,        // Set Normal Display
        0xA8, 0x3F,  // Set Multiplex Ratio
        0xD3, 0x00,  // Set display offset
        0xD5, 0x80,  // Set display clock divide ratio/oscillator frequency
        0xD9, 0xF1,  // Set pre-charge period
        0xDA, 0x12,  // Set com pins hardware configuration
        0xDB, 0x40,  // Set vcomh
        0x8D, 0x14,  // Enable charge pump regulator
        SH1106_CMD_DISPLAY_ON
    };

    for (size_t i = 0; i < sizeof(init_cmds); i++) {
        esp_err_t ret = sh1106_send_command(oled, init_cmds[i]);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error initializing SH1106: %s", esp_err_to_name(ret));
            return ret;
        }
    }

    ESP_LOGI(TAG, "SH1106 initialized successfully");
    sh1106_clear(oled);
    sh1106_update(oled);
    return ESP_OK;
}


// Turn on the display
void sh1106_display_on(sh1106_t *oled) {
    sh1106_send_command(oled, SH1106_CMD_DISPLAY_ON);
}

// Turn off the display
void sh1106_display_off(sh1106_t *oled) {
    sh1106_send_command(oled, SH1106_CMD_DISPLAY_OFF);
}

// Clear the display buffer
void sh1106_clear(sh1106_t *oled) {
    memset(oled->buffer, 0, sizeof(oled->buffer));
}

// Update the display with the current buffer content
void sh1106_update(sh1106_t *oled) {
    for (uint8_t page = 0; page < (oled->height / 8); page++) {
        sh1106_send_command(oled, SH1106_CMD_SET_PAGE | page);
        sh1106_send_command(oled, 0x02);  // Lower column address
        sh1106_send_command(oled, 0x10);  // Higher column address

        uint8_t data[SH1106_WIDTH + 1];
        data[0] = 0x40;  // Data mode
        memcpy(&data[1], &oled->buffer[page * oled->width], oled->width);

        esp_err_t ret = i2c_master_transmit(dev_handle, data, sizeof(data), pdMS_TO_TICKS(1000));
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error updating display: %s", esp_err_to_name(ret));
        }
    }
}


void sh1106_draw_pixel(sh1106_t *oled, int x, int y, uint8_t color) {
    // Example implementation
    if (x < 0 || x >= oled->width || y < 0 || y >= oled->height) {
        return;
    }
    oled->buffer[x + (y / 8) * oled->width] |= (1 << (y % 8));
}

void sh1106_draw_bitmap(sh1106_t *oled, int x, int y, const uint8_t *bitmap, int w, int h) {
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            int bit = (bitmap[j * (w / 8) + i / 8] >> (7 - (i % 8))) & 1;
            sh1106_draw_pixel(oled, x + i, y + j, bit);
        }
    }
}