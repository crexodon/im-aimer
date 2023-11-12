#pragma once
// File just simply gets included once, so I can define functions here (tm)

#include "secrets.h"
#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

extern char deviceID[32];

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
uint32_t lastReconnectAttempt = 0;

String mqttGetFullTopic(const char *subTopic) {
    return String("devices/") + String(deviceID) + String("/") + String(subTopic);
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

boolean mqttReconnect() {
    const char *lastWillTopic = mqttGetFullTopic("online").c_str();
    if (client.connect("foobar", MQTT_USER, MQTT_PASS, lastWillTopic, 1, true, "0")) {
        Serial.println("MQTT connected!");
        // Once connected, publish an announcement...
        client.publish("outTopic", "hello world");
        // ... and resubscribe
        // client.subscribe("inTopic");
    }
    else {
        Serial.println("MQTT connect failed! " + String(client.state()));
    }
    return client.connected();
}

bool mqttPublish(const char* subTopic, const uint8_t *payload, size_t payloadLen) {
    char topicBuf[128];
    snprintf(topicBuf, sizeof(topicBuf), "devices/%s/%s", deviceID, subTopic);
    return client.publish(topicBuf, payload, payloadLen);
}

bool mqttPublish(const char* subTopic, const char* payload) {
    return mqttPublish(subTopic, (const uint8_t*)payload, payload ? strlen(payload) : 0);
}

void mqttInit() {
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(mqttCallback);
}

void mqttLoop() {
    if (!client.connected()) {
        uint32_t now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            // Attempt to reconnect
            if (mqttReconnect()) {
                lastReconnectAttempt = 0;
            }
        }
    }
    else {  // client connected
        client.loop();
    }
}