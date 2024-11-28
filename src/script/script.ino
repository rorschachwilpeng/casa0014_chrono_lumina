/********* ---------------------------------------------------------------------------- Header --------------------------------------------------------------------- ********/
//#include <Wire.h>
//#include <Adafruit_GFX.h>
#include <WiFiNINA.h>   
#include <PubSubClient.h>
#include <utility/wifi_drv.h>   // library to drive to RGB LED on the MKR1010
#include "arduino_secrets.h" 
#include <math.h>  // 用于正弦函数
/********* ---------------------------------------------------------------------------- Header --------------------------------------------------------------------- ********/


/*
**** please enter your sensitive data in the Secret tab/arduino_secrets.h
**** using format below

#define SECRET_SSID "ssid name"
#define SECRET_PASS "ssid password"
#define SECRET_MQTTUSER "user name - eg student"
#define SECRET_MQTTPASS "password";
 */

// 全局变量，用于呼吸灯的计算
unsigned long lastUpdate = 0; // 上次更新灯光的时间
float brightness = 0.0;      // 当前亮度
float increment = 0.1;       // 增量，用于控制变化速度
float maxBrightness = 255.0; // 最大亮度
float minBrightness = 50.0;  // 最小亮度
float angle = 0.0;            // 全局角度，用于控制呼吸节奏


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

// edit this for the light you are connecting to
char mqtt_topic_demo[] = "student/CASA0014/light/22/pixel/";


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
}

// void loop() {

//   // we need to make sure the arduino is still connected to the MQTT broker
//   // otherwise we will not receive any messages
//   if (!client.connected()) {
//     reconnectMQTT();
//   }

//   // we also need to make sure we are connected to the wifi
//   // otherwise it will be impossible to connect to MQTT!
//   if (WiFi.status() != WL_CONNECTED){
//     startWifi();
//   }

//   // check for messages from the broker and ensuring that any outgoing messages are sent.
//   client.loop();

//   sendmqtt();




//   Serial.println("sent a message");
//   delay(10000);
  
// }
void loop() {
  // 保持WiFi和MQTT的连接
  if (!client.connected()) {
    reconnectMQTT();
  }

  if (WiFi.status() != WL_CONNECTED) {
    startWifi();
  }

  client.loop();

  // 每50ms更新一次灯光
  if (millis() - lastUpdate > 50) {
    lastUpdate = millis();
    sendmqtt(); // 发送呼吸灯指令
  }
}
/********* -----------------------------------------------SET UP & LOOP---------------------------------------------------------------------------------------- ********/


/********* ---------------------------------------------MQTT----------------------------------------------------------------------------------------- ********/
// 灯光控制信息的发步
// void sendmqtt(){

//   // send a message to update the light
//   char mqtt_message[100];
//   sprintf(mqtt_message, "{\"pixelid\": %d, \"R\": 0, \"G\": 255, \"B\": 128, \"W\": 200}", 2);
//   Serial.println(mqtt_topic_demo);
//   Serial.println(mqtt_message);
  

//   if (client.publish(mqtt_topic_demo, mqtt_message)) {
//     Serial.println("Message published");
//   } else {
//     Serial.println("Failed to publish message");
//   }

// }
// void sendmqtt() {
//   // 计算当前亮度值（模拟呼吸灯效果）
//   static float angle = 0.0; // 角度，用于计算正弦波
//   angle += increment;
//   if (angle > 2 * PI) {
//     angle -= 2 * PI; // 防止角度无限增长
//   }
//   brightness = minBrightness + (maxBrightness - minBrightness) * (0.5 * (1 + sin(angle)));

//   // 设置灯光颜色，使用动态的亮度值
//   int R = (int)(brightness); // 红色分量
//   int G = (int)(brightness * 0.5); // 绿色分量
//   int B = (int)(brightness * 0.2); // 蓝色分量
//   int W = 0;                      // 白光分量

//   // 构造JSON消息
//   char mqtt_message[100];
//   sprintf(mqtt_message, "{\"pixelid\": %d, \"R\": %d, \"G\": %d, \"B\": %d, \"W\": %d}", 2, R, G, B, W);
  
//   // 打印消息和主题
//   Serial.println(mqtt_topic_demo);
//   Serial.println(mqtt_message);

//   // 发布消息
//   if (client.publish(mqtt_topic_demo, mqtt_message)) {
//     Serial.println("Message published");
//   } else {
//     Serial.println("Failed to publish message");
//   }
// }

void sendmqtt() {
  // 静态变量，记录当前点亮的像素 ID 和全局亮度变化角度
  static int current_pixel = 0;  // 当前点亮的像素
  static float angle = 0.0;      // 角度，用于计算正弦波（控制亮度）

  // 更新亮度（呼吸灯效果）
  angle += increment;  // 每次调用，角度增加
  if (angle > 2 * PI) {
    angle -= 2 * PI;  // 防止角度无限增长
  }
  float brightness = minBrightness + (maxBrightness - minBrightness) * (0.5 * (1 + sin(angle)));

  // 计算 RGB 值
  int R = (int)(brightness);       // 红色分量
  int G = (int)(brightness * 0.5); // 绿色分量
  int B = (int)(brightness * 0.2); // 蓝色分量
  int W = 0;                       // 白光分量

  // 构造 JSON 消息，只更新当前像素
  char mqtt_message[100];
  sprintf(mqtt_message, "{\"pixelid\": %d, \"R\": %d, \"G\": %d, \"B\": %d, \"W\": %d}", current_pixel, R, G, B, W);

  // 打印消息和主题
  Serial.println(mqtt_topic_demo);
  Serial.println(mqtt_message);

  // 发布消息
  if (client.publish(mqtt_topic_demo, mqtt_message)) {
    Serial.println("Message published");
  } else {
    Serial.println("Failed to publish message");
  }

  // 更新到下一个像素
  current_pixel++;
  if (current_pixel >= 12) {  // 如果已到最后一个像素，则循环到第一个像素
    current_pixel = 0;
  }
}





//   Serial.println("arrive here3");

//   // 结束 JSON 数组
//   strcat(mqtt_message, "]");

//   // 发布消息
//   Serial.println(mqtt_message);  // 打印消息用于调试
//   if (client.publish(mqtt_topic_demo, mqtt_message)) {
//     Serial.println("Message published");
//   } else {
//     Serial.println("Failed to publish message");
//   }

//   // 更新角度
//   angle += increment;
//   if (angle > 2 * PI) {
//     angle -= 2 * PI;
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
/********* ---------------------------------------------MQTT----------------------------------------------------------------------------------------- ********/