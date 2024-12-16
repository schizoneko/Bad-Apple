#include <stdio.h>
#include <string.h>
#include "driver/i2c_master.h"
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "esp_err.h"
#include "esp_log.h"
#include "sh1106.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// I2C configuration
#define I2C_MASTER_SCL_IO       22          // GPIO for SCL
#define I2C_MASTER_SDA_IO       21          // GPIO for SDA
#define I2C_MASTER_FREQ_HZ      100000      // I2C master clock frequency
#define I2C_MASTER_PORT         0           // I2C port number
#define SH1106_I2C_ADDR         0x3C        // SH1106 I2C address

// SD card configuration
#define SD_MOSI_PIN             23          // GPIO for MOSI
#define SD_MISO_PIN             19          // GPIO for MISO
#define SD_CLK_PIN              18          // GPIO for CLK
#define SD_CS_PIN               5           // GPIO for CS

// SH1106 display dimensions
#define SCREEN_WIDTH            128         // SH1106 width
#define SCREEN_HEIGHT           64          // SH1106 height

#define TOTAL_FRAMES            2190        // Total frames to display
#define MOUNT_POINT             "/sdcard"

static const char *TAG = "OLED_Player";

// SH1106 display handle
static sh1106_t oled;
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;

// Initialize I2C master bus and device
esp_err_t i2c_master_init(void) {
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_PORT,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t err = i2c_new_master_bus(&bus_config, &bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C bus: %s", esp_err_to_name(err));
        return err;
    }

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SH1106_I2C_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    err = i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add I2C device: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "I2C master initialized successfully");
    return ESP_OK;
}

// Initialize SD card
esp_err_t sd_card_init(void) {
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS_PIN;
    slot_config.host_id = host.slot;

    esp_err_t ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "SD card mounted successfully");
    return ESP_OK;
}

// Display a frame from a BMP file
void display_frame(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file: %s", filename);
        return;
    }

    uint8_t buffer[SCREEN_WIDTH * SCREEN_HEIGHT / 8];
    size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);
    fclose(file);

    if (bytes_read == sizeof(buffer)) {
        sh1106_clear(&oled);
        sh1106_draw_bitmap(&oled, 0, 0, buffer, SCREEN_WIDTH, SCREEN_HEIGHT);
        sh1106_update(&oled);
    } else {
        ESP_LOGE(TAG, "Error reading file: %s", filename);
    }
}

void app_main(void) {
    // Initialize I2C
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    // Initialize SH1106 display
    ESP_ERROR_CHECK(sh1106_init_desc(&oled, I2C_MASTER_PORT, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO));
    ESP_ERROR_CHECK(sh1106_init(&oled));
    sh1106_clear(&oled);
    sh1106_update(&oled);
    ESP_LOGI(TAG, "OLED display initialized successfully");

    sh1106_display_on(&oled);

    // Initialize SD card
    esp_err_t ret = sd_card_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card");
        return;
    }
    ESP_LOGI(TAG, "SD card mounted successfully");

    // Loop to display all frames
    char filepath[64];
    for (int i = 1; i <= TOTAL_FRAMES; i++) {
        snprintf(filepath, sizeof(filepath), MOUNT_POINT "/converted_frames/frame_%04d.bmp", i);
        ESP_LOGI(TAG, "Displaying frame: %s", filepath);
        display_frame(filepath);
        vTaskDelay(pdMS_TO_TICKS(100)); // 10 fps delay
    }

    ESP_LOGI(TAG, "Animation complete.");

    sh1106_display_off(&oled);
}
