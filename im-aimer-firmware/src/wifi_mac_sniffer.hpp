#pragma once

#include "esp_wifi.h"
#include <Arduino.h>
#include <map>
#include <time.h>
#include "util.h"
#include "secrets.h"

class WiFiMACSniffer {
  public:

    void start() {
        _prevWiFiMode = WiFi.getMode();
        WiFi.disconnect();
        esp_wifi_set_promiscuous(true);
        _lastChanSwitch = millis();
        _clients.clear();
        _channel = 1;
        esp_wifi_set_channel(_channel, WIFI_SECOND_CHAN_NONE);
    }

    void stop() {
        esp_wifi_set_promiscuous(false);
        // esp_wifi_stop();
        WiFi.mode(_prevWiFiMode);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
    }

    int getCount() {
        bool promEnabled;
        esp_wifi_get_promiscuous(&promEnabled);
        if (promEnabled) // still running
            return -1;
        else
            return _clients.size();
    }

    // Returns true when running
    bool loop(bool stopAfter1Iteration) {
        bool promEnabled;
        esp_wifi_get_promiscuous(&promEnabled);

        if (!promEnabled)
            return false;

        if (millis() - _lastChanSwitch >= WIFI_CHANNEL_SWITCH_INTERVAL) {
            if (promEnabled) {
                
                _lastChanSwitch = millis();
                esp_wifi_set_channel(_channel, WIFI_SECOND_CHAN_NONE);
                _channel = (_channel % WIFI_CHANNEL_MAX) + 1;

                if (_channel == 1) {
                    // TODO: do data parsing?
                    Serial.printf("%lld  Number of clients: %2d\n", getTimestamp(), _clients.size());
                    for (auto const &client : _clients) {
                        uint64_t mac = client.first;
                        client_metadata_t data = _clients[mac];

                        Serial.printf("[" MACSTR "] Chan: %2d, RSSI: %3d, Type: %d, Count: %4d, lastSeen: %6lld\n", MAC2STR((uint8_t*)&mac),
                            data.channel, data.rssi, data.packetType, data.packetCount, data.lastSeen);
                    }

                    if (stopAfter1Iteration) {
                        stop();
                    }

                    return false;   // done
                }
            }
        }
        return true;
    }

    void prom_pkt_callback(void *buf, wifi_promiscuous_pkt_type_t type) {
        if (type != WIFI_PKT_MGMT)
            return;

        const wifi_promiscuous_pkt_t *promPkt = (wifi_promiscuous_pkt_t *)buf;
        const wifi_ieee80211_mac_frame_t *macFrame = (wifi_ieee80211_mac_frame_t *)promPkt->payload;
        // const wifi_ieee80211_mac_frame_hdr_t *macFrameHdr = &macFrame->hdr;

        uint64_t clientMacAddr;

        if ((int)macFrame->hdr.frame_ctrl.subtype != 8) {
            Serial.printf(MACSTR " / " MACSTR " / " MACSTR " / Chan: %2d, RSSI: %3d, Subtype: %2d\n", 
                MAC2STR(macFrame->hdr.addr1),
                MAC2STR(macFrame->hdr.addr2), 
                MAC2STR(macFrame->hdr.addr3), 
                promPkt->rx_ctrl.channel, 
                promPkt->rx_ctrl.rssi,
                macFrame->hdr.frame_ctrl.subtype
            );
        }

        switch (macFrame->hdr.frame_ctrl.subtype) {
            case frame_ctrl_subtype::ProbeRequest:
                memcpy(&clientMacAddr, macFrame->hdr.addr2, sizeof(macFrame->hdr.addr2));
                for (int i = 0; i < promPkt->rx_ctrl.sig_len; i++) {
                    Serial.printf("%02X ", promPkt->payload[i]);
                }
                Serial.print("   ");
                for (int i = 0; i < promPkt->rx_ctrl.sig_len; i++) {
                    uint8_t c = promPkt->payload[i];
                    Serial.write((c >= 0x20 && c <= 0x7E) ? c : '.');
                }
                Serial.println();
                
                break;
            case frame_ctrl_subtype::ProbeResponse:
                memcpy(&clientMacAddr, macFrame->hdr.addr1, sizeof(macFrame->hdr.addr1));
                break;
            default:
                // other packets are irrelevant
                return;
        }

        client_metadata_t toSave = {
            .channel = promPkt->rx_ctrl.channel,
            .rssi = promPkt->rx_ctrl.rssi,
            .packetType = (unsigned)macFrame->hdr.frame_ctrl.subtype,
            .lastSeen = getTimestamp(),   
        };
        try {
            client_metadata_t prevStats = _clients.at(clientMacAddr);
            toSave.packetCount = prevStats.packetCount + 1;
            _clients[clientMacAddr] = toSave;
        } catch (std::out_of_range e) { // client mac not yet in buffer
            toSave.packetCount = 0;
            _clients[clientMacAddr] = toSave;
        }
    }

#pragma pack(push, 1)
    typedef struct {
        uint8_t channel;
        int8_t rssi;
        uint32_t packetCount;
        uint8_t packetType;
        uint64_t lastSeen;
    } client_metadata_t;
#pragma pack(pop)

  private:
    const unsigned WIFI_CHANNEL_SWITCH_INTERVAL = 500;
    const unsigned WIFI_CHANNEL_MAX = 13;

    uint32_t _lastChanSwitch = 0;
    uint8_t _channel = 1;
    wifi_mode_t _prevWiFiMode = WIFI_MODE_STA;

    std::map<uint64_t, client_metadata_t> _clients;

#pragma pack(push, 1)
    enum class frame_ctrl_subtype : uint8_t {
        AssociationRequest,
        AssociationResponse,
        ReassociationRequest,
        ReassociationResponse,
        ProbeRequest,
        ProbeResponse,
        TimingAdvertisement,
        Beacon = 8,
        ATIM,
        Disassociation,
        Authentication,
        Deauthentication,
        Action,
        ActionNACK,
    };

    // frame_ctrl field in MAC frame header
    typedef union {
        struct {
            uint16_t protocol_version : 2;
            uint16_t type : 2;
            frame_ctrl_subtype subtype : 4;
            uint16_t to_DS : 1;
            uint16_t from_DS : 1;
            uint16_t more_fragments : 1;
            uint16_t retry : 1;
            uint16_t pwr_mgmt : 1;
            uint16_t more_data : 1;
            uint16_t protected_frame : 1;
            uint16_t htc_order : 1;
        };
        uint16_t raw;
    } mac_frame_ctrl;

    // MAC frame header
    typedef struct {
        mac_frame_ctrl frame_ctrl;
        unsigned duration_id : 16;
        uint8_t addr1[6]; /* receiver address */
        uint8_t addr2[6]; /* sender address */
        uint8_t addr3[6]; /* final receiver address */
        unsigned sequence_ctrl : 16;
        uint8_t addr4[6]; /* optional */
    } wifi_ieee80211_mac_frame_hdr_t;

    // MAC frame
    typedef struct {
        wifi_ieee80211_mac_frame_hdr_t hdr;
        uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
    } wifi_ieee80211_mac_frame_t;

#pragma pack(pop)

    const char *wifi_promiscuous_pkt_type_str[4] = {
        "MGMT",
        "CTRL",
        "DATA",
        "MISC",
    };
};