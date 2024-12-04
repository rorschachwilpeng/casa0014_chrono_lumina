#include "studyArea.h"

// 全局变量定义
bool isStartStudy = false;
bool isPausedStudy = false;
bool encoderChangeStudy = false;
int light_num_study = 0;
bool studyTimeFinish = false;
int remainingStudyTime = 0;
bool lightsActive = true;

// 函数实现
void studyAreaLogic() {
    // 根据 Encoder 的输入情况来决定逻辑走向
    StudyEncoderHandler();

    if (!isStartStudy) {
        Serial.print("System is waiting to resume...\n");
        return;
    }

    tickTockStudy();

    if (studyTimeFinish) {
        Serial.print("Study time finished. Exiting...\n");
        isStartStudy = false;
    }
}

// 监测走学习逻辑时候的Encoder变化
void StudyEncoderHandler(){
  //学习状态默认是不开始的，直到用户第一次按下按钮
  if(!isStartStudy && UserPressedButton){
    isStartStudy=true;
  }

  //开始学习，但是用户按下按钮，走暂停逻辑
  if(isStartStudy && UserPressedButton){
      isPausedStudy = true;
      isStartStudy=false;
      //TODO:这里加入停止时间的功能代码
  }

  // 编码器变化逻辑（仅在学习未开始时生效）
  if (!isStartStudy && encoderChangeStudy) {
      // 学习未开始时，允许调整灯光
      if (encoderChangeStudy) {
          // 假设外部函数更新 encoderChangeStudy，这里处理灯光变化
          if (/* 检查编码器减少 */) {
              light_num_study--;
              Serial.print("Light decreased. Current light_num: ");
              Serial.println(light_num_study);
          } else if (/* 检查编码器增加 */) {
              light_num_study++;
              Serial.print("Light increased. Current light_num: ");
              Serial.println(light_num_study);
          }
          encoderChangeStudy = false; // 处理完变化后重置标志
      }
  }
}

void tickTockStudy() {
    static unsigned long lastUpdateTime = millis(); // 上一次更新时间
    unsigned long currentTime = millis();

    // 如果时间间隔超过1秒
    if (currentTime - lastUpdateTime >= 1000) {
        lastUpdateTime = currentTime;

        // 减少剩余学习时间
        if (remainingStudyTime > 0) {
            remainingStudyTime--;
            Serial.print("Remaining study time: ");
            Serial.println(remainingStudyTime);
        }

        // 检查学习时间是否结束
        if (remainingStudyTime <= 0) {
            Serial.print("Study time finished. Exiting...\n");
            studyTimeFinish = true; // 设置学习完成状态
            lightsActive = false;  // 关闭灯光
            return;
        }
    }

    // 调用灯光控制逻辑
    controlStudyLights();
}

void controlStudyLights() {
    static float angle = 0.0;        // 用于动态亮度控制的角度
    const int minBrightness = 50;   // 最小亮度
    const int maxBrightness = 255;  // 最大亮度

    int lights[] = {1, 2, 3, 4};    // 示例灯光编号
    int numLights = sizeof(lights) / sizeof(lights[0]);
    int totalTime = 60;             // 学习总时间（示例：60秒）
    bool dynamicEffect = true;      // 是否启用动态效果

    int timeBlock = totalTime / numLights; // 每个灯光的时间块
    static int currentLightIndex = 0;      // 当前亮灯索引

    // 计算当前时间块对应的灯索引
    int lightIndex = (totalTime - remainingStudyTime) / timeBlock;

    if (lightIndex != currentLightIndex) {
        currentLightIndex = lightIndex; // 更新当前亮灯索引
    }

    // 遍历所有灯
    for (int i = 0; i < numLights; i++) {
        for (int pixel_id = 0; pixel_id < 12; pixel_id++) {
            char mqtt_topic_demo[100];
            sprintf(mqtt_topic_demo, "student/CASA0014/light/%d/pixel/", lights[i]);

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
                    R = 255;
                    G = 0;
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
