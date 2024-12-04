/********* ---------------------------------------------------------------------------- Header --------------------------------------------------------------------- ********/
//#include <Wire.h>
//#include <Adafruit_GFX.h>
#include <WiFiNINA.h>   
#include <PubSubClient.h>
#include <utility/wifi_drv.h>   // library to drive to RGB LED on the MKR1010
#include "arduino_secrets.h" 
#include <math.h>  // 用于正弦函数
/********* ---------------------------------------------------------------------------- Header --------------------------------------------------------------------- ********/


/********* ---------------------------------------------------------------------------- Variables --------------------------------------------------------------------- ********/
// 全局变量，用于呼吸灯的计算
unsigned long lastUpdate = 0; // 上次更新灯光的时间
float brightness = 0.0;      // 当前亮度
float increment = 0.1;       // 增量，用于控制变化速度
float maxBrightness = 255.0; // 最大亮度
float minBrightness = 50.0;  // 最小亮度
float angle = 0.0;            // 全局角度，用于控制呼吸节奏
bool AllLightsOff=false;

// Number of light you wanna control
const int LightsNum =52;
const int StudyAreaNum=25;
const int DividingNum=5;
const int RestingAreaNum=22;

// hashmap for mapping light. 
// Key: light's number; 
// Value: 0/1, light is on/off
int DividingLineArea[StudyAreaNum];
int StudyArea[DividingNum];
int RestingArea[RestingAreaNum];

const char* ssid          = SECRET_SSID;
const char* password      = SECRET_PASS;
const char* mqtt_username = SECRET_MQTTUSER;
const char* mqtt_password = SECRET_MQTTPASS;
const char* mqtt_server = "mqtt.cetools.org";
const int mqtt_port = 1884;
int status = WL_IDLE_STATUS;     // the Wifi radio's status


WiFiServer server(80);
WiFiClient wificlient;

WiFiClient mkrClient;
PubSubClient client(mkrClient);

// 定义 MQTT 主题模板，用于控制不同灯光
char mqtt_topic_demo[500];
/********* ---------------------------------------------------------------------------- Variables --------------------------------------------------------------------- ********/


bool lightsActive = true;
/********* -----------------------------------------------SET UP & LOOP---------------------------------------------------------------------------------------- ********/
void setup() {
  // Start the serial monitor to show output
  Serial.begin(115200);
  delay(1000);

  //WiFi.setHostname("Lumina ucjtdjw");
  WiFi.setHostname("Lumina ucfnyyp");
  startWifi();
  client.setServer(mqtt_server, mqtt_port);
  Serial.println("setup complete");
  client.setBufferSize(512);

}

void loop(){

  if (touchSensorA){//如果走学习区的逻辑，休息区的时间停止
    StudyArea();
  }else if(touchSensorB){//如果走休息区的逻辑，休息区的时间停止
    RestingArea();
  } else if(touchSensorA && touchSensorB){//如果同时按下TA和TB --> 则清空逻辑
    ResetAreas();
  }
  
}


void loop() {
  // 保持WiFi和MQTT的连接
  if (!client.connected()) {
    reconnectMQTT();
  }

  if (WiFi.status() != WL_CONNECTED) {
    startWifi();
  }
  client.loop();
  //Study Area
  // int studyAreaLights[] = {23, 24};
  // controlStudyArea(studyAreaLights,2,true);

  //
  int studyAreaLights[] = {1,49};
  controlStudyAreaWithTimer(studyAreaLights,2,true,20);

  // Dividing Area
  // int dividingLights[] = {23, 24};

  // controlDividingLine(dividingLights,2);
  

  delay(100);

}
/********* -----------------------------------------------SET UP & LOOP---------------------------------------------------------------------------------------- ********/


/********* ----------------------------------------------- General Functions ---------------------------------------------------------------------------------------- ********/
// TODO:关闭所有灯的函数
void turnOffLights(int lights[], int num_lights) {
  for (int i = 0; i < num_lights; i++) {
    // 更新 MQTT 主题为当前灯光的编号
    sprintf(mqtt_topic_demo, "student/CASA0014/light/%d/pixel/", lights[i]);

    // 构造 JSON 消息，将 RGB 和亮度都设为 0（关灯）
    char mqtt_message[100];
    sprintf(mqtt_message, "{\"pixelid\": %d, \"R\": %d, \"G\": %d, \"B\": %d, \"W\": %d}", 0, 0, 0, 0, 0);

    // 发布消息
    if (client.publish(mqtt_topic_demo, mqtt_message)) {
      Serial.print("Light turned off: ");
      Serial.println(lights[i]);
    } else {
      Serial.print("Failed to turn off light: ");
      Serial.println(lights[i]);
    }
  }
  AllLightsOff=true;
}


//TODO:将Encoder的角度映射到灯的数量上 -->最多只能亮6盏灯，每盏灯代表5分钟的time block
void RotationMapping(){

}
/********* ----------------------------------------------- General Functions ---------------------------------------------------------------------------------------- ********/


/********* --------------------------------------------- Study Area Logic----------------------------------------------------------------------------------------- ********/

#include <iostream>

// TODO:加入Encoder和Sensor的控件进行测试

// 全局状态变量
bool isStartStudy = false;    // 学习是否开始
bool isPausedStudy = false;   // 是否处于暂停状态
bool encoderChangeStudy = false; // 编码器是否有变化
int light_num_study = 0;      // 学习区域灯光计数
bool studyTimeFinish = false; // 学习时间是否完成

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

void studyAreaLogic() {
    //根据Encoder的输入情况来决定逻辑走向
    StudyEncoderHandler();

    // 如果未开始学习，直接返回
    if (!isStartStudy) {
        Serial.print("System is waiting to resume...\n");
        return;
    }

    // 学习逻辑正在进行
    tickTockStudy(); // 模拟学习中的计时逻辑

    // 检查学习时间是否完成
    if (studyTimeFinish) {
        Serial.print("Study time finished. Exiting...\n");
        isStartStudy = false; // 重置状态
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







void controlStudyArea(int lights[], int numLights, bool dynamicEffect) {
    static float angle = 0.0; // 角度，用于正弦波亮度控制
    static int current_pixel = 0; // 当前点亮的像素
    float brightness;
    int R, G, B, W;

    for (int i = 0; i < numLights; i++) {
        for (int pixel_id = 0; pixel_id < 12; pixel_id++) {
            // 设置 MQTT 主题为指定灯光编号
            sprintf(mqtt_topic_demo, "student/CASA0014/light/%d/pixel/", lights[i]);

            if (dynamicEffect) {
                // 动态效果：计算当前像素的亮度
                angle += 0.1; // 控制亮度变化的速度
                if (angle > 2 * PI) {
                    angle -= 2 * PI; // 防止角度无限增长
                }
                brightness = minBrightness + (maxBrightness - minBrightness) * (0.5 * (1 + sin(angle)));
                R = (int)(brightness);  // 红色分量
                G = (int)(brightness*0.5);        // 绿色分量
                B = (int)(brightness * 0.2);  // 蓝色分量
                W = 0;                        // 白光分量
            } else {
                // 静态效果：设置为固定颜色
                R = 255;
                G = 0;
                B = 0;
                W = 0;
            }

            // 构造 JSON 消息，将像素点设置为指定颜色
            char mqtt_message[100];
            sprintf(mqtt_message, "{\"pixelid\": %d, \"R\": %d, \"G\": %d, \"B\": %d, \"W\": %d}", pixel_id, R, G, B, W);

            // 发布消息
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

        // 更新到下一个像素（仅动态模式下）
        if (dynamicEffect) {
            current_pixel++;
            if (current_pixel >= 12) { // 循环到第一个像素
                current_pixel = 0;
            }
        }
    }
}

void controlStudyAreaWithTimer(int lights[], int numLights, bool dynamicEffect, int totalTime) {
    static float angle = 0.0; // 角度，用于正弦波亮度控制
    float brightness;
    int R, G, B, W;

    int timeBlock = totalTime / numLights; // 每个灯的时间块长度

    // 如果灯光已经关闭，直接退出函数
    if (!lightsActive) {
        return;
    }

    static unsigned long globalStartTime = 0; // 静态全局开始时间
    static int currentLightIndex = 0;         // 当前发光灯的索引
    static bool timerInitialized = false;     // 标记计时器是否已经初始化

    if (!timerInitialized) { // 初始化计时器
        globalStartTime = millis();
        currentLightIndex = 0;
        timerInitialized = true;
    }

    unsigned long elapsedTime = millis() - globalStartTime;

    // 如果计时结束
    if (elapsedTime > (unsigned long)(totalTime * 1000)) {
        lightsActive = false; // 关闭灯光
        timerInitialized = false; // 重置计时器
        // 确保所有灯熄灭
        for (int i = 0; i < numLights; i++) {
            for (int pixel_id = 0; pixel_id < 12; pixel_id++) {
                sprintf(mqtt_topic_demo, "student/CASA0014/light/%d/pixel/", lights[i]);
                char mqtt_message[100];
                sprintf(mqtt_message, "{\"pixelid\": %d, \"R\": 0, \"G\": 0, \"B\": 0, \"W\": 0}", pixel_id);
                client.publish(mqtt_topic_demo, mqtt_message);
            }
        }
        return; // 结束函数
    }

    // 判断当前时间块的灯索引
    int lightIndex = elapsedTime / (timeBlock * 1000);
    if (lightIndex != currentLightIndex) {
        currentLightIndex = lightIndex; // 更新当前发光灯的索引
    }

    // 遍历所有灯
    for (int i = 0; i < numLights; i++) {
        for (int pixel_id = 0; pixel_id < 12; pixel_id++) {
            // 设置 MQTT 主题为指定灯光编号
            sprintf(mqtt_topic_demo, "student/CASA0014/light/%d/pixel/", lights[i]);

            if (i == currentLightIndex) { 
                // 当前灯发光
                if (dynamicEffect) {
                    // 动态效果：计算当前像素的亮度
                    angle += 0.1; // 控制亮度变化的速度
                    if (angle > 2 * PI) {
                        angle -= 2 * PI; // 防止角度无限增长
                    }
                    brightness = minBrightness + (maxBrightness - minBrightness) * (0.5 * (1 + sin(angle)));
                    R = (int)(brightness);    // 红色分量
                    G = (int)(brightness * 0.5); // 绿色分量
                    B = (int)(brightness * 0.2); // 蓝色分量
                    W = 0;                       // 白光分量
                } else {
                    // 静态效果：设置为固定颜色
                    R = 255;
                    G = 0;
                    B = 0;
                    W = 0;
                }
            } else {
                // 其他灯熄灭
                R = 0;
                G = 0;
                B = 0;
                W = 0;
            }

            // 构造 JSON 消息，将像素点设置为指定颜色
            char mqtt_message[100];
            sprintf(mqtt_message, "{\"pixelid\": %d, \"R\": %d, \"G\": %d, \"B\": %d, \"W\": %d}", pixel_id, R, G, B, W);

            // 发布消息
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

    // 延迟用于控制灯光刷新频率（可调整）
    delay(100);
}


//学习区域
// 模拟计时器逻辑
// void countdown() {
//     if (timeLeft > 0) {
//         timeLeft--;
//         cout << "Counting down... Time left: " << timeLeft << endl;
//     }
// }

// 暂停逻辑
// void handlePause() {
//     cout << "Paused. Waiting for resume..." << endl;
//     // 这里可以插入暂停的具体操作逻辑
// }

// 修改灯光数量
// void modifyLight(bool increase) {
//     lightNum += (increase ? 1 : -1);
//     cout << "Light number updated: " << lightNum << endl;
// }


// Study Area主逻辑框架
// void studyAreaLogic() {
//     if (studyStarted) {
//         countdown();

//         if (paused) {
//             handlePause();
//             return; // 暂停时退出当前逻辑
//         } else {
//             // 未暂停时
//             if (timeLeft > 0) {
//                 // 继续计时
//                 return;
//             } else {
//                 // 时间结束
//                 studyFinished = true;
//                 cout << "Study session finished." << endl;
//             }
//         }
//     } else {
//         // 学习未开始的分支
//         if (encoderChanged) {
//             modifyLight(true); // 假设为增加灯光
//         } else {
//             cout << "Waiting for start or encoder input..." << endl;
//         }
//     }
// }

/********* --------------------------------------------- Study Area Logic----------------------------------------------------------------------------------------- ********/


/********* --------------------------------------------- Dividing Line Logic----------------------------------------------------------------------------------------- ********/
void controlDividingLine(int lights[], int numLights) {
  for (int i = 0; i < numLights; i++) {
    for (int pixel_id = 0; pixel_id < 12; pixel_id++) {
        // 设置MQTT主题为第24个灯
        sprintf(mqtt_topic_demo, "student/CASA0014/light/%d/pixel/", lights[i]);

        // 构造JSON消息，将像素设置为红色
        char mqtt_message[100];
        sprintf(mqtt_message, "{\"pixelid\": %d, \"R\": %d, \"G\": %d, \"B\": %d, \"W\": %d}", pixel_id, 255, 0, 0, 0);

        // 发布消息
        if (client.publish(mqtt_topic_demo, mqtt_message)) {
          Serial.print("Set pixel ");
          Serial.print(pixel_id);
          Serial.println(" to red.");
        } else {
          Serial.print("Failed to set pixel ");
          Serial.println(pixel_id);
        }
      }
  }
}
/********* --------------------------------------------- Dividing Line Logic----------------------------------------------------------------------------------------- ********/


/********* --------------------------------------------- Chill Area Logic----------------------------------------------------------------------------------------- ********/
//休息区域
// void restAreaLogic(int[] rest_arr){
//   return;
// }
/********* --------------------------------------------- Chill Area Logic----------------------------------------------------------------------------------------- ********/


/********* ---------------------------------------------MQTT----------------------------------------------------------------------------------------- ********/
// 将信息传给MQTT
// void sendmqtt(int num_lights) {
//   // 静态变量，记录全局亮度变化角度
//   static float angle = 0.0; // 角度，用于正弦波亮度控制
//   static int current_pixel = 0;  // 当前点亮的像素
//   int lights_to_control[num_lights];
  
//   //填充数组
//   for(int i=0;i<num_lights;i++){
//     lights_to_control[i]=i+1;
//   }

//   // 关闭所有灯
//   if(!AllLightsOff){
//      turnOffLights(lights_to_control, num_lights);
//      return;
//   }
 

//   // 更新亮度（模拟呼吸灯效果）
//   angle += increment; // 每次调用，角度增加
//   if (angle > 2 * PI) {
//     angle -= 2 * PI; // 防止角度无限增长
//   }
//   float brightness = minBrightness + (maxBrightness - minBrightness) * (0.5 * (1 + sin(angle)));

//   // 计算 RGB 值
//   int R = (int)(brightness*0.5);       // 红色分量
//   int G = (int)(brightness); // 绿色分量
//   int B = (int)(brightness * 0.2); // 蓝色分量
//   int W = 0;                       // 白光分量

//   // 定义需要控制的灯光编号
//   //int lights_to_control[] = {1, 4, 19, 20, 21, 22, 23, 24, 25};
//   //int num_lights = 9; // 灯光数量

//   // 遍历每个灯光，分别发送控制指令
//   for (int i = 0; i < num_lights; i++) {
//     // 更新 MQTT 主题为当前灯光的编号
//     sprintf(mqtt_topic_demo, "student/CASA0014/light/%d/pixel/", lights_to_control[i]);

//     // 构造 JSON 消息
//     char mqtt_message[100];
//     sprintf(mqtt_message, "{\"pixelid\": %d, \"R\": %d, \"G\": %d, \"B\": %d, \"W\": %d}", current_pixel, R, G, B, W);

//     // 打印消息和主题
//     // Serial.print("Publishing to topic: ");
//     // Serial.println(mqtt_topic_demo);
//     // Serial.print("Message: ");
//     // Serial.println(mqtt_message);

//     // 发布消息
//     if (client.publish(mqtt_topic_demo, mqtt_message)) {
//       Serial.print("Message published for light ");
//       Serial.println(lights_to_control[i]);
      
//     } else {
//       Serial.print("Failed to publish message for light ");
//       Serial.println(lights_to_control[i]);
//     }
//     // 更新到下一个像素
//     current_pixel++;
//     if (current_pixel >= 12) {  // 如果已到最后一个像素，则循环到第一个像素
//       current_pixel = 0;
//     }
    
//   }
// }

// 负责管理MKR1010的WiFi连接
void startWifi(){
    
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // Function for connecting to a WiFi network
  // is looking for UCL_IoT and a back up network (usually a home one!)
  int n = WiFi.scanNetworks();
  Serial.println("Scan done");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    // loop through all the networks and if you find UCL_IoT or the backup - ssid1
    // then connect to wifi
    Serial.print("Trying to connect to: ");
    Serial.println(ssid);
    for (int i = 0; i < n; ++i){
      String availablessid = WiFi.SSID(i);
      // Primary network
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
          break; // Exit the loop if connected
        } else {
          Serial.println("Failed to connect to " + String(ssid));
        }
      } else {
        Serial.print(availablessid);
        Serial.println(" - this network is not in my list");
      }

    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

}

// 没连接上的话，接着连接
void reconnectMQTT() {
  if (WiFi.status() != WL_CONNECTED){
    startWifi();
  } else {
    //Serial.println(WiFi.localIP());
  }

  // Loop until we're reconnected
  while (!client.connected()) {    // while not (!) connected....
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "LuminaSelector";
    clientId += String(random(0xffff), HEX);
    
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("connected");
      // ... and subscribe to messages on broker
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// 打印接收到的主题和消息内容
void callback(char* topic, byte* payload, int length) {
  // Handle incoming messages
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

}
/********* ----******* ---------------------------------------------MQTT----------------------------------------------------------------------------------------- ********/