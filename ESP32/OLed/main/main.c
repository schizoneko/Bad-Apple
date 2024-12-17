#include <stdio.h>
#include <string.h>
#include "driver/i2c_master.h"
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

// SH1106 display dimensions
#define SCREEN_WIDTH            128         // SH1106 width
#define SCREEN_HEIGHT           64          // SH1106 height

static const char *TAG = "OLED_Player";

// SH1106 display handle
static sh1106_t oled;

// I2C master bus and device handle
static i2c_master_bus_handle_t bus_handle;
i2c_master_dev_handle_t dev_handle;

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

    if (dev_handle == NULL) {
        ESP_LOGE(TAG, "dev_handle is NULL after i2c_master_bus_add_device()");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "I2C master initialized successfully with dev_handle: %p", dev_handle);
    return ESP_OK;
}

// Display a frame received over serial
void display_frame(uint8_t *buffer, size_t size) {
    if (size != (SCREEN_WIDTH * SCREEN_HEIGHT / 8)) {
        ESP_LOGE(TAG, "Incorrect frame size: expected %d bytes, got %d bytes", SCREEN_WIDTH * SCREEN_HEIGHT / 8, size);
        return;
    }

    // Proceed if the frame size is correct
    sh1106_clear(&oled);
    sh1106_draw_bitmap(&oled, 0, 0, buffer, SCREEN_WIDTH, SCREEN_HEIGHT);
    sh1106_update(&oled);
}

// Function to receive frame data over serial
void receive_frames_task(void *pvParameters) {
    uint8_t buffer[SCREEN_WIDTH * SCREEN_HEIGHT / 8];
    while (1) {
        ESP_LOGI(TAG, "Waiting for frame data...");
        size_t bytes_read = fread(buffer, 1, sizeof(buffer), stdin);
        if (bytes_read == sizeof(buffer)) {
            ESP_LOGI(TAG, "Received frame data, displaying...");
            display_frame(buffer, bytes_read);
        } else {
            ESP_LOGE(TAG, "Failed to receive complete frame data");
        }
        vTaskDelay(pdMS_TO_TICKS(100));  // Delay to maintain 10 fps
    }
}

void app_main(void) {
    // Initialize I2C
    esp_err_t ret = i2c_master_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed");
        return;
    }

    ESP_LOGI(TAG, "I2C initialized successfully");

    // Initialize SH1106 display
    ret = sh1106_init_desc(&oled, I2C_MASTER_PORT, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SH1106 descriptor initialization failed");
        return;
    }

    ret = sh1106_init(&oled);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SH1106 initialization failed");
        return;
    }

    sh1106_clear(&oled);
    sh1106_update(&oled);
    sh1106_display_on(&oled);
    ESP_LOGI(TAG, "OLED display initialized successfully");

    // Start the task to receive and display frames over serial
    xTaskCreate(receive_frames_task, "receive_frames_task", 4096, NULL, 5, NULL);
}
