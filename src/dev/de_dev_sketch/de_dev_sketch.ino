#include "studyArea.h"
#include "mqttHandler.h"
#include "restingArea.h"

// 新增全局变量用于状态管理
enum SystemState { STUDYING, RESTING, IDLE };
SystemState currentState = IDLE;  // 系统当前状态

void setup() {
    Serial.begin(9600);

    // Initialize WiFi and MQTT
    startWifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    // Initialize study area logic
    remainingStudyTime = 60; // 初始化学习时间
    restingTime = 30;        // 初始化休息时间
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

    // 判断用户操作
    if (TATouched) { // 用户触摸 TA，进入学习区域
        currentState = STUDYING;  // 切换到学习状态
        studyAreaLogic();         // 调用学习逻辑
    } else if (TBTouched) { // 用户触摸 TB，进入休息区域
        currentState = RESTING;   // 切换到休息状态
        restAreaLogic();          // 调用休息逻辑
    } else if (TATouched && TBTouched) { // 用户同时触摸 TA 和 TB，重置灯光
        resetLogic();             // 重置逻辑
        currentState = IDLE;      // 切换到空闲状态
    }

    // 自动切换逻辑
    if (currentState == STUDYING && studyTimeFinish && restingTime > 0) {
        Serial.print("Study time finished. Switching to rest area logic...\n");
        currentState = RESTING; // 切换到休息状态
    } else if (currentState == RESTING && restTimeFinish) {
        Serial.print("Resting time finished. Returning to idle state...\n");
        currentState = IDLE; // 切换到空闲状态
    }

    delay(100);
}
