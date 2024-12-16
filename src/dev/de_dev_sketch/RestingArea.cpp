#include "RestingArea.h"

// 全局变量定义
bool isStartRest = true;
bool isPausedRest = false;
bool encoderChangeRest = false;
int light_num_rest = 0;
bool restTimeFinish = false;
int remainingRestTime = 0;
bool lightsActiveRest = true;

// Resting Area 主逻辑
void restAreaLogic() {
    // 根据 Encoder 的输入情况来决定逻辑走向
    RestEncoderHandler();

    if (!isStartRest) {
        Serial.print("Resting Area is waiting to resume...\n");
        return;
    }

    tickTockRest();

    if (restTimeFinish) {
        Serial.print("Resting time finished. Exiting...\n");
        isStartRest = false;
    }
}

// 监测走休息逻辑时的 Encoder 变化
void RestEncoderHandler() {
    // 休息状态默认是开始的（结束学习逻辑后直接走休息逻辑），直到用户第一次按下暂停按钮
    if (UserPressedButton) {
        isStartRest = false;
    }

    // 开始休息，但是用户按下按钮，走暂停逻辑
    if (isStartRest && UserPressedButton) {
        isPausedRest = true;
        isStartRest = false;
        // TODO:这里加入停止时间的功能代码
    }

    // 编码器变化逻辑（仅在休息未开始时生效）
    if (!isStartRest && encoderChangeRest) {
        // 休息未开始时，允许调整灯光
        if (encoderChangeRest) {
            if (/* 检查编码器减少 */) {
                light_num_rest--;
                Serial.print("Resting Area Light decreased. Current light_num: ");
                Serial.println(light_num_rest);
            } else if (/* 检查编码器增加 */) {
                light_num_rest++;
                Serial.print("Resting Area Light increased. Current light_num: ");
                Serial.println(light_num_rest);
            }
            encoderChangeRest = false; // 处理完变化后重置标志
        }
    }
}

// 模拟休息的计时逻辑
void tickTockRest() {
    static unsigned long lastUpdateTime = millis(); // 上一次更新时间
    unsigned long currentTime = millis();

    // 如果时间间隔超过1秒
    if (currentTime - lastUpdateTime >= 1000) {
        lastUpdateTime = currentTime;

        // 减少剩余休息时间
        if (remainingRestTime > 0) {
            remainingRestTime--;
            Serial.print("Remaining resting time: ");
            Serial.println(remainingRestTime);
        }

        // 检查休息时间是否结束
        if (remainingRestTime <= 0) {
            Serial.print("Resting time finished. Exiting...\n");
            restTimeFinish = true;  // 设置休息完成状态
            lightsActiveRest = false; // 关闭灯光
            return;
        }
    }

    // 调用灯光控制逻辑
    controlRestLights();
}

// 控制休息区的灯光
void controlRestLights() {
    static float angle = 0.0;        // 用于动态亮度控制的角度
    const int minBrightness = 50;   // 最小亮度
    const int maxBrightness = 255;  // 最大亮度

    int lights[] = {5, 6, 7, 8};    // 示例灯光编号
    int numLights = sizeof(lights) / sizeof(lights[0]);
    int totalTime = 60;             // 休息总时间（示例：60秒）
    bool dynamicEffect = true;      // 是否启用动态效果

    int timeBlock = totalTime / numLights; // 每个灯光的时间块
    static int currentLightIndex = 0;      // 当前亮灯索引

    // 计算当前时间块对应的灯索引
    int lightIndex = (totalTime - remainingRestTime) / timeBlock;

    if (lightIndex != currentLightIndex) {
        currentLightIndex = lightIndex; // 更新当前亮灯索引
    }

    // 遍历所有灯
    for (int i = 0; i < numLights; i++) {
        for (int pixel_id = 0; pixel_id < 12; pixel_id++) {
            char mqtt_topic_demo[100];
            sprintf(mqtt_topic_demo, "student/CASA0014/restLight/%d/pixel/", lights[i]);

            int R, G, B, W;

            if (i == currentLightIndex) {
                // 当前灯发光
                if (dynamicEffect) {
                    // 动态效果：基于正弦波变化亮度
                    angle += 0.1;
                    if (angle > 2 * PI) {
                        angle -= 2 * PI;
                    }
                    float brightness = minBrightness + (maxBrightness - minBrightness) * (0.5 * (1 + sin(angle)));
                    R = (int)(brightness);
                    G = (int)(brightness * 0.5);
                    B = (int)(brightness * 0.2);
                    W = 0;
                } else {
                    // 静态效果：固定颜色
                    R = 0;
                    G = 255;
                    B = 0;
                    W = 0;
                }
            } else {
                // 非当前灯熄灭
                R = 0;
                G = 0;
                B = 0;
                W = 0;
            }

            // 构造并发布 MQTT 消息
            char mqtt_message[100];
            sprintf(mqtt_message, "{\"pixelid\": %d, \"R\": %d, \"G\": %d, \"B\": %d, \"W\": %d}", pixel_id, R, G, B, W);
            if (client.publish(mqtt_topic_demo, mqtt_message)) {
                Serial.print("Set pixel ");
                Serial.print(pixel_id);
                Serial.print(" of light ");
                Serial.print(lights[i]);
                Serial.println(" successfully.");
            } else {
                Serial.print("Failed to set pixel ");
                Serial.print(pixel_id);
                Serial.print(" of light ");
                Serial.println(lights[i]);
            }
        }
    }

    // 控制灯光刷新频率
    delay(100);
}
