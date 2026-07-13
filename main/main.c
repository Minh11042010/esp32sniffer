#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "MONITOR";

// Callback function executed every time a raw 802.11 packet flies through the air
void promiscuous_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
    const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    
    // Print the prefix your Python script is looking for
    printf("DATA:");
    
    // Print the raw payload bytes as a continuous hex string
    for (int i = 0; i < pkt->rx_ctrl.sig_len; i++) {
        printf("%02x", pkt->payload[i]);
    }
    printf("\n");
}

void app_main(void) {
    // 1. Initialize NVS (Required for Wi-Fi stack)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Initialize Network Interface and Wi-Fi Configuration
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // Set storage to RAM so we don't wear out the flash memory
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    
    // Force the chip into Null/Station mode without associating
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // 3. Configure Promiscuous (Monitor) Mode
    // Filter configuration (captures management/data frames, ignores standard beacons if desired)
    wifi_promiscuous_filter_t filter = {
        .filter_mask = WIFI_PROMIS_FILTER_MASK_ALL
    };
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filter));
    
    // Register our custom callback function
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb));
    
    // Turn monitor mode ON
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));

    // 4. Lock onto a specific 2.4GHz Wi-Fi Channel (e.g., Channel 1, 6, or 11)
    uint8_t primary_channel = 1; 
    esp_wifi_set_channel(primary_channel, WIFI_SECOND_CHAN_NONE);
    
    ESP_LOGI(TAG, "Monitor mode initialized on Channel %d. Streaming raw frames...", primary_channel);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}