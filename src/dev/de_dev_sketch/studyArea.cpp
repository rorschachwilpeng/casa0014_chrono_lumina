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

void StudyEncoderHandler() {
    // 实现你的逻辑...
}

void tickTockStudy() {
    // 实现时间减少逻辑...
    controlStudyLights();
}

void controlStudyLights() {
    // 实现灯光控制逻辑...
}
