file(GLOB_RECURSE APP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
idf_component_register(
    SRCS ${APP_SOURCES}
    INCLUDE_DIRS "." "ui" "model"
    REQUIRES nvs_flash esp_wifi esp_event esp_netif esp_timer app_update lvgl bsp bus i2c_devices esp_lcd driver heap)
