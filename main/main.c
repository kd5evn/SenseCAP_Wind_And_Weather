#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_ota_ops.h"
#include "nvs_flash.h"
#include "bsp_board.h"
#include "lv_port.h"
#include "config.h"
#include "wind_model.h"
#include "wind_page.h"

static const char *TAG = "app";
ESP_EVENT_DEFINE_BASE(VIEW_EVENT_BASE);

void app_main(void)
{
    ESP_LOGI(TAG, "SenseCAP wind display, milestone M0 (%s %s)", __DATE__, __TIME__);

    esp_err_t r = nvs_flash_init();
    if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_ERROR_CHECK(bsp_board_init());
    lv_port_init();
    wind_model_init();

    lv_port_sem_take();
    wind_page_init();
    lv_port_sem_give();

    /* Mark the image valid once the UI is up so OTA rollback does not revert it. */
    esp_ota_mark_app_valid_cancel_rollback();

    while (1) {
        ESP_LOGI(TAG, "heap internal %u, psram %u",
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
