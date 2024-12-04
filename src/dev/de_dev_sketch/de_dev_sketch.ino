#include "studyArea.h" // 包含头文件

void setup() {
    Serial.begin(9600);
    remainingStudyTime = 60; // 初始化学习时间
    Serial.print("System initialized. Ready to start...\n");
}

void loop() {
    studyAreaLogic(); // 调用模块化的逻辑
}
