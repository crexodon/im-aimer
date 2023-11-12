#include "esp_wifi.h"
#include <Arduino.h>
#include <WiFi.h>
#include "wifi_mac_sniffer.hpp"
#include "util.h"
#include "secrets.h"    // needs to be renamed from secrets.sample.h
#include "ota.hpp"
#include "mqtt.hpp"

const uint doWiFiThingsTime = 5000;

char deviceID[32] = {0};

uint32_t lastWifiTime = 0;

WiFiMACSniffer sniffer;

esp_err_t event_handler(void *ctx, system_event_t *event) { return ESP_OK; }

void prom_pkt_callback(void *buf, wifi_promiscuous_pkt_type_t type) { sniffer.prom_pkt_callback(buf, type); };

void setup() {
    uint64_t mac = ESP.getEfuseMac();
    snprintf(deviceID, sizeof(deviceID), "esp-%02x%02x%02x%02x%02x%02x", MAC2STR((uint8_t *)&mac));
    
    Serial.begin(115200);
    Serial.println("AImer Hello World!");
    Serial.println(deviceID);

    Serial.print("Connecting to WiFi... ");
    Serial.flush();
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(deviceID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 3000) {  }
    
    Serial.print("WiFi connected! IP: ");
    Serial.printf("%s\n", WiFi.localIP().toString().c_str());

    Serial.print("Initializing NTP... ");
    Serial.flush();
    initNTP();
    Serial.println("done.");

    otaInit();
    mqttInit();

    esp_wifi_set_promiscuous_rx_cb(prom_pkt_callback);
    // sniffer.start();
}

void loop() {    
    // bool running = sniffer.loop(true);
    // if (!running) {
    //     if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    //         Serial.print("Sending MQTT values... "); 
    //         Serial.flush();
    //         mqttLoop(); // reconnect
    //         mqttPublish("online", "1");
    //         mqttPublish("uptime", String( millis() ).c_str());
    //         mqttPublish("wifi-probe-devices", String( sniffer.getCount() ).c_str());
    //         Serial.println("done.");

    //         uint32_t start = millis();
    //         while (millis() - start < doWiFiThingsTime) {
    //             otaLoop();
    //             mqttLoop();
    //         }
    //     }
        
    //     sniffer.start();
    // }
    
    mqttLoop();
    if (millis()%5000 == 0) {
        if(!client.publish("outTopic", "hello world 2")) {
            Serial.println("Publish failed.");
        }
        else {
            Serial.println("Publish success!");
        }
    }

}
