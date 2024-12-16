#ifndef RESTINGAREA_H
#define RESTINGAREA_H

#include <Arduino.h>

// 全局变量声明
extern bool isStartRest;
extern bool isPausedRest;
extern bool encoderChangeRest;
extern int light_num_rest;
extern bool restTimeFinish;
extern int remainingRestTime;
extern bool lightsActiveRest;

// 函数声明
void restAreaLogic();
void RestEncoderHandler();
void tickTockRest();
void controlRestLights();

#endif
