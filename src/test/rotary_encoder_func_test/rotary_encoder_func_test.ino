
/********************************************  Header  **************************************************/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "arduino_secrets.h" 
#include <Ticker.h>
/********************************************  Header  **************************************************/

/********************************************  Variables  **************************************************/
#define CLK_PIN 14  // CLK connected to D5 (GPIO14)
#define DT_PIN 12   // DT connected to D6 (GPIO12)
#define SW_PIN 13   // SW connected to D7 (GPIO13)
// Ticker instances for non-blocking tasks -- avoid any potential issue to the MQTT heartbeat mechnisim
Ticker REcoderTicker;
Ticker sendMqttTicker;

const int maxLights = 4;                  // 最大灯数量
int currentLightCount = 1;                // 当前亮灯主题数量，初始为 1
const char* lightTopics[maxLights] = {    // 每盏灯对应的 MQTT 主题
    "student/CASA0014/light/1/pixel/",
    "student/CASA0014/light/6/pixel/",
    "student/CASA0014/light/14/pixel/",
    "student/CASA0014/light/22/pixel/"
};

// 全局变量，用于呼吸灯的计算
static int current_pixel = 0;  // 当前点亮的 pixel ID
float brightness = 0.0;      // 当前亮度
float increment = 0.1;       // 增量，用于控制变化速度
float maxBrightness = 255.0;  // 最大亮度
float minBrightness = 0.0;    // 最小亮度（未选中灯的亮度）
float angle = 0.0;            // 全局角度，用于控制呼吸节奏

int currentLightIndex = 0; // 当前灯的索引，用于逐步切换灯光

// WiFi and MQTT credentials
const char* ssid          = SECRET_SSID;
const char* password      = SECRET_PASS;
const char* mqtt_username = SECRET_MQTTUSER;
const char* mqtt_password = SECRET_MQTTPASS;
const char* mqtt_server = "mqtt.cetools.org";
const int mqtt_port = 1884;

WiFiClient espClient;
PubSubClient client(espClient);
/********************************************  Variables  **************************************************/

void setup() {
  Serial.begin(115200);
  delay(10);

  // Rotary Encoder
  pinMode(CLK_PIN, INPUT);
  pinMode(DT_PIN, INPUT);
  pinMode(SW_PIN, INPUT_PULLUP); // Internal pull-up resistor

  // Connect to WiFi
  startWifi();

  // Connect to the MQTT broker
  client.setServer(mqtt_server, mqtt_port);
  client.setBufferSize(2000);
  client.setCallback(callback);

  Serial.println("Set-up complete");

  // Schedule tasks with tickers
  REcoderTicker.attach_ms(50,checkRotaryEncoder);//Check rotary encoder with Ticker
  //sendMqttTicker.attach_ms(500, sendmqtt); 
  sendMqttTicker.attach(2, sendNextLight);    // Periodically send MQTT messages for the next light
}

void loop() {
  // Reconnect to MQTT if necessary
  if (!client.connected()) {
    reconnectMQTT();
  }
  // Reconnect to WiFi if disconnected
  if (WiFi.status() != WL_CONNECTED){
    startWifi();
  }
  // Keep MQTT connection alive
  client.loop();

}

/*************************************************  MQTT  *******************************************************/
// Function to handle incoming MQTT messages
void callback(char* topic, byte* payload, unsigned int length) {
  // Serial.print("Message received on topic: ");
  // Serial.println(topic);

  // // Check which topic the message is from
  // if (strcmp(topic, mqtt_topic) == 0) {
  //   handleTrainData(payload, length);
  //   //newDataReceived = true;
  // } else {
  //   Serial.println("Unknown topic");
  // }
}

void startWifi() {
  // Start connecting to the WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // Wait until connected
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(600);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP8266Client-neo2-ucjtdjw", mqtt_username, mqtt_password)) {
      Serial.println("connected");
      for (int i = 0; i < maxLights; i++) {
        client.subscribe(lightTopics[i]);
      }
      Serial.println("Subscribed to MQTT topics");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}


void sendmqtt(int lightIndex, int pixelIndex, int R, int G, int B, int W){
  // if (lightIndex >= currentLightCount) {
  //   return; // 如果灯索引超出当前亮灯数量，直接返回
  // }
  const char* currentTopic = lightTopics[lightIndex]; // 根据当前灯索引切换主题
  char mqtt_message[100]; // 缓冲区，存储完整的 JSON 消息
  sprintf(mqtt_message, "{\"pixelid\": %d, \"R\": %d, \"G\": %d, \"B\": %d, \"W\": %d}", pixelIndex, R, G, B, W);
  // Serial.println(currentTopic);
  // Serial.println(mqtt_message);
  if (client.publish(currentTopic, mqtt_message)) {
   //Serial.printf("Message published for light %d\n", lightIndex);
  } else {
   // Serial.printf("Failed to publish message for light %d\n", lightIndex);
  }
}
/*************************************************  MQTT  *******************************************************/

/*************************************************  Rotary Encoder  *******************************************************/
// Cyclically send to the next light
void sendNextLight() {
  // 更新当前灯的所有 pixel
  for (int i = 0; i < 12; i++) {
    // 如果当前灯的索引小于亮灯数量，则点亮灯；否则设置 RGB 为 0 0 0
    if (currentLightIndex < currentLightCount) {
      sendmqtt(currentLightIndex, i, 255, 0, 0, 0); // 点亮灯
    } 
  }
  // 更新到下一盏灯
  currentLightIndex++;
  if (currentLightIndex >= maxLights) {
    currentLightIndex = 0; // 循环回到第一盏灯
  }
}

void turnOffExtraLights() {
  for (int lightIndex = currentLightCount; lightIndex < maxLights; lightIndex++) {
    for (int pixelId = 0; pixelId < 12; pixelId++) {
      sendmqtt(lightIndex, pixelId, 0, 255, 0, 0); // 关闭灯
    }
  }
}

void checkRotaryEncoder() {
  static int lastCLK = LOW; // Static variable to hold the last CLK state

  int currentCLK = digitalRead(CLK_PIN);
  if (currentCLK != lastCLK && currentCLK == HIGH) { // Detect rising edge
    if (digitalRead(DT_PIN) != currentCLK) {
      // Clockwise rotation: increase the light count
      currentLightCount++;
      if (currentLightCount > maxLights) {
        currentLightCount = maxLights;
      }
      Serial.print("Rotated CW: Light Count = ");
    } else {
      // Counterclockwise rotation: decrease the light count
      currentLightCount--;
      if (currentLightCount < 1) {
        currentLightCount = 1;
      }
      turnOffExtraLights(); // 立即关闭多余的灯
      Serial.print("Rotated CCW: Light Count = ");
    }
    Serial.println(currentLightCount);
  }
  lastCLK = currentCLK;
}

// Check button press
// void checkRotaryEncoder() {
//   // Detect rotary encoder rotation
//   static int lastCLK = LOW;  // Static variable to hold the last CLK state
//   static int lastSW = HIGH;  // Static variable to hold the last SW state

//   int currentCLK = digitalRead(CLK_PIN);
//   if (currentCLK != lastCLK && currentCLK == HIGH) { // Detect rising edge
//     if (digitalRead(DT_PIN) != currentCLK) {
//       Serial.println("Rotated CW (Clockwise)"); // Clockwise rotation
//     } else {
//       Serial.println("Rotated CCW (Counterclockwise)"); // Counterclockwise rotation
//     }
//   }
//   lastCLK = currentCLK;

//   // Detect button press
//   int currentSW = digitalRead(SW_PIN);
//   if (currentSW == LOW && lastSW == HIGH) { // Detect press event
//     Serial.println("Button Pressed!");
//   }
//   lastSW = currentSW;
// }
/*************************************************  Rotary Encoder  *******************************************************/
