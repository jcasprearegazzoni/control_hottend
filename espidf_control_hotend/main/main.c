#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "app";

void app_main(void)
{
    while (1) {
        ESP_LOGI(TAG, "Hola ESP-IDF desde VS Code");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
