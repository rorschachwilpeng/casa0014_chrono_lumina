const int touchPin = 2;  // Touch Sensor SIG 连接到 D2 引脚

void setup() {
  pinMode(touchPin, INPUT);
  Serial.begin(115200);
}

void loop() {
  int touchState = digitalRead(touchPin);  // 读取触摸状态
  if (touchState == HIGH) {
    Serial.println("Touched!");
  } else {
    //Serial.println("Not Touched!");
  }
  delay(100);
}
