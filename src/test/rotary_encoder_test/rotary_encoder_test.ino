#define SIG_PIN A0  // 定义信号引脚为 A0
#define MAX_ANGLE 300  // 传感器的最大旋转角度

void setup() {
  // 初始化串口监视器
  Serial.begin(9600);
}

void loop() {
  // 读取模拟值 (0 - 1023)
  int sensorValue = analogRead(SIG_PIN);
  
  // 将模拟值转换为角度
  float angle = map(sensorValue, 0, 1023, 0, MAX_ANGLE);

  // 打印角度值到串口监视器
  Serial.print("角度值: ");
  Serial.print(angle);
  Serial.println(" 度");

  // 延迟以便观察输出
  delay(100);
}
