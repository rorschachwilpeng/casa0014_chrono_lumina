#include "studyArea.h"      // 引入学习区域的逻辑
#include "mqttHandler.h"    // 引入解耦后的 MQTT 相关逻辑

void setup() {
    Serial.begin(9600);

    // Initialize WiFi and MQTT
    startWifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    // Initialize study area logic
    remainingStudyTime = 60; // 初始化学习时间
    Serial.print("System initialized. Ready to start...\n");
}

void loop() {
    // 保持 WiFi 和 MQTT 的连接
    if (!client.connected()) {
        reconnectMQTT();
    }
    if (WiFi.status() != WL_CONNECTED) {
        startWifi();
    }
    client.loop();

    // 调用学习区域逻辑
    studyAreaLogic();

    delay(100);
}
