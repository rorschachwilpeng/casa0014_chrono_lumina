#include "mqttHandler.h"

// WiFi and MQTT credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqtt_server = "YOUR_MQTT_SERVER_ADDRESS";
const char* mqtt_username = "YOUR_MQTT_USERNAME";
const char* mqtt_password = "YOUR_MQTT_PASSWORD";

// Global variables
WiFiClient espClient;
PubSubClient client(espClient);

// Start WiFi connection
void startWifi() {
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("Communication with WiFi module failed!");
        while (true);
    }
    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
        Serial.println("Please upgrade the firmware");
    }
    int n = WiFi.scanNetworks();
    Serial.println("Scan done");
    if (n == 0) {
        Serial.println("No networks found");
    } else {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i) {
            String availablessid = WiFi.SSID(i);
            if (availablessid.equals(ssid)) {
                Serial.print("Connecting to ");
                Serial.println(ssid);
                WiFi.begin(ssid, password);
                while (WiFi.status() != WL_CONNECTED) {
                    delay(600);
                    Serial.print(".");
                }
                if (WiFi.status() == WL_CONNECTED) {
                    Serial.println("Connected to " + String(ssid));
                    break;
                } else {
                    Serial.println("Failed to connect to " + String(ssid));
                }
            } else {
                Serial.print(availablessid);
                Serial.println(" - this network is not in my list");
            }
        }
    }
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

// Reconnect to MQTT broker
void reconnectMQTT() {
    if (WiFi.status() != WL_CONNECTED) {
        startWifi();
    }

    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        String clientId = "LuminaSelector";
        clientId += String(random(0xffff), HEX);
        if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("MQTT connected");
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

// Callback function for handling incoming MQTT messages
void callback(char* topic, byte* payload, int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("]: ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}
