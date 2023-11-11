#include "esp_wifi.h"
#include <Arduino.h>
#include <WiFi.h>
#include "wifi_mac_sniffer.hpp"
#include "util.h"

#define WIFI_CHANNEL_SWITCH_INTERVAL (500)
#define WIFI_CHANNEL_MAX (13)
uint8_t mac_lib[100][6];
int mac_count = 0;
uint8_t level = 0, channel = 1;
static wifi_country_t wifi_country = {.cc = "DE", .schan = 1, .nchan = 13}; //Most recent esp32 library struct


WiFiMACSniffer sniffer;

esp_err_t event_handler(void *ctx, system_event_t *event) { return ESP_OK; }

void prom_pkt_callback(void *buf, wifi_promiscuous_pkt_type_t type) { sniffer.prom_pkt_callback(buf, type); };

void setup() {
    Serial.begin(115200);
    Serial.println("AImer Hello World!");

    // probably need WiFi first
    initNTP();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_country(&wifi_country)); /* set country for channel range [1, 13] */
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(prom_pkt_callback);
}

void loop() {    
    sniffer.loop();
}
