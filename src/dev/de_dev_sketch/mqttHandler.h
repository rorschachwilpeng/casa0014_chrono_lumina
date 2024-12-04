#ifndef MQTTHANDLER_H
#define MQTTHANDLER_H

#include <WiFiNINA.h>
#include <PubSubClient.h>

// WiFi and MQTT credentials (replace with your own)
extern const char* ssid;          // WiFi SSID
extern const char* password;      // WiFi Password
extern const char* mqtt_server;   // MQTT broker address
extern const char* mqtt_username; // MQTT username
extern const char* mqtt_password; // MQTT password

// Declare global variables
extern WiFiClient espClient;
extern PubSubClient client;

// Function declarations
void startWifi();
void reconnectMQTT();
void callback(char* topic, byte* payload, int length);

#endif
