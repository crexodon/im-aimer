#include "esp_wifi.h"
#include <Arduino.h>
#include <WiFi.h>
#include "wifi_mac_sniffer.hpp"
#include <time.h>

#define WIFI_CHANNEL_SWITCH_INTERVAL (500)
#define WIFI_CHANNEL_MAX (13)
uint8_t mac_lib[100][6];
int mac_count = 0;
uint8_t level = 0, channel = 1;
static wifi_country_t wifi_country = {.cc = "DE", .schan = 1, .nchan = 13}; //Most recent esp32 library struct

const char* ntpServer = "de.pool.ntp.org";
const char* TZ_INFO =   "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00"; // Berlin, for others, see: https://remotemonitoringsystems.ca/time-zone-abbreviations.php
tm timeinfo;
time_t now;

bool getNTPtime(int sec) {
    {
        uint32_t start = millis();
        do {
            time(&now);
            localtime_r(&now, &timeinfo);
            delay(10);
        } while (((millis() - start) <= (1000 * sec)) && (timeinfo.tm_year < (2016 - 1900)));
        if (timeinfo.tm_year <= (2016 - 1900))
            return false; // the NTP call was not successful
    }
    return true;
}

char curISOStr[32] = {0};
const char *getISOTimeStr() {
    getNTPtime(0);
    memset(curISOStr, 0, sizeof(curISOStr));
    strftime(curISOStr, sizeof(curISOStr), "%FT%TZ", localtime(&now));  // Formats: http://www.cplusplus.com/reference/ctime/strftime/
    return curISOStr;
}

void initNTP() {
    configTime(0, 0, ntpServer);
    setenv("TZ", TZ_INFO, 1);
    if (getNTPtime(3)) {
    } // wait up to 3sec to sync
    else {
        // DEBUG_println("Time not set");
    }
}

WiFiMACSniffer sniffer;

esp_err_t event_handler(void *ctx, system_event_t *event) { return ESP_OK; }

void prom_pkt_callback(void *buf, wifi_promiscuous_pkt_type_t type) { sniffer.prom_pkt_callback(buf, type); };

void setup() {
    Serial.begin(115200);
    Serial.println("AImer Hello World!");

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
    // delay(WIFI_CHANNEL_SWITCH_INTERVAL);
    // esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    // channel = (channel % WIFI_CHANNEL_MAX) + 1;
    // if (channel == 1) {
    //     Serial.printf("MACs (%d): ", mac_count);
    //     for (int i = 0; i < mac_count; i++) {
    //         Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x, ", mac_lib[i][0], mac_lib[i][1], mac_lib[i][2], mac_lib[i][3], mac_lib[i][4], mac_lib[i][5]);
    //     }
    //     Serial.printf("\n");
        
    //     mac_count = 0;
    // }
    
    sniffer.loop();
}
