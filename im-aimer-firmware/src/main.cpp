#include "esp_wifi.h"
#include <Arduino.h>
#include <WiFi.h>
#include "wifi_mac_sniffer.hpp"
#include "util.h"
#include "secrets.h"    // needs to be renamed from secrets.sample.h

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

    Serial.print("Connecting to WiFi... ");
    Serial.flush();
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 3000) {  }
    Serial.print("WiFi connected! IP: ");
    Serial.printf("%s\n", WiFi.localIP().toString().c_str());

    Serial.print("Initializing NTP... ");
    Serial.flush();
    initNTP();
    Serial.println("done.");

    esp_wifi_set_promiscuous_rx_cb(prom_pkt_callback);
    sniffer.start();
}

void loop() {    
    bool running = sniffer.loop(true);
    if (!running) {
        // TODO: send data to MQTT
        delay(10000);
        
        sniffer.start();
    }
}
