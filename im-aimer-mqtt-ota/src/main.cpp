#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include "credentials.h"

long lastReconnectAttempt = 0;

char deviceName[20];

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

boolean reconnect()
{
  if (client.connect("ESP8266Client", user, pw))
  {
    // Once connected, publish an announcement...
    client.publish("outTopic", "hello world");
    // ... and resubscribe
    client.subscribe("inTopic");
  }
  return client.connected();
}

void setup()
{

  uint64_t ID = ESP.getEfuseMac();
  sprintf(deviceName, "esp-%02x%02x%02x%02x%02x%02x", MAC2STR((uint8_t *)&ID));

  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(deviceName);
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // No authentication by default
  ArduinoOTA.setPassword("im-ainer");

  ArduinoOTA
      .onStart([]()
               {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type); })
      .onEnd([]()
             { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed"); });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_server_port);
  client.setCallback(callback);
}

int test = 0;

void loop()
{
  if (!client.connected())
  {
    long now = millis();
    if (now - lastReconnectAttempt > 5000)
    {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect())
      {
        lastReconnectAttempt = 0;
      }
    }
  }
  else
  {

    client.loop();

    long now = millis();
    if (now - lastMsg > 5000)
    {
      lastMsg = now;

      // hier anderen code einfügen

      char topicBuff[100];
      sprintf(topicBuff, "devices/%s/fill-level", deviceName);

      char buff[5];
      sprintf(buff, "%u", random(0, 100));

      client.publish(topicBuff, buff);
      Serial.println("Publish");
    }
  }

  ArduinoOTA.handle();
}