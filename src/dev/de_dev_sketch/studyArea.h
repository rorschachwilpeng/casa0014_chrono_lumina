#ifndef STUDY_AREA_H
#define STUDY_AREA_H

#include <Arduino.h>

// 全局变量声明
extern bool isStartStudy;
extern bool isPausedStudy;
extern bool encoderChangeStudy;
extern int light_num_study;
extern bool studyTimeFinish;
extern int remainingStudyTime;
extern bool lightsActive;

// 函数声明
void studyAreaLogic();
void StudyEncoderHandler();
void tickTockStudy();
void controlStudyLights();

#endif
