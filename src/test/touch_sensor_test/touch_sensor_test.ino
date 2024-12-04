#define CLK 2  // 时钟引脚
#define DT 3   // 数据引脚
#define SW 4   // 按钮引脚

volatile int counter = 0;            // 计数器
volatile bool buttonPressed = false; // 按钮按下标志

void setup() {
  // 设置引脚模式
  pinMode(CLK, INPUT_PULLUP);
  pinMode(DT, INPUT_PULLUP);
  pinMode(SW, INPUT_PULLUP); // 使用内部上拉电阻

  // 附加中断服务程序
  attachInterrupt(digitalPinToInterrupt(CLK), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SW), checkButton, FALLING);

  // 初始化串口监视器
  Serial.begin(9600);
}

void loop() {
  // 主循环中处理计数器值的变化
  static int lastCounter = 0;
  if (counter != lastCounter) {
    Serial.print("计数器值：");
    Serial.println(counter);
    lastCounter = counter;
  }

  // // 处理按钮按下事件
  // if (buttonPressed) {
  //   Serial.println("按钮被按下");
  //   buttonPressed = false; // 重置按钮标志
  // }

  // 添加一个小的延迟以提高稳定性
  delay(5);
}

// 中断服务程序：更新编码器计数器
void updateEncoder() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  // 去抖动：忽略短时间内的重复中断
  if (interruptTime - lastInterruptTime > 5) {
    if (digitalRead(CLK) != digitalRead(DT)) {
      counter++;
      Serial.println("旋转方向：顺时针");
    } else {
      counter--;
      Serial.println("旋转方向：逆时针");
    }
  }
  lastInterruptTime = interruptTime;
}

// 中断服务程序：检测按钮按下
void checkButton() {
  static unsigned long lastButtonPress = 0;
  unsigned long buttonPressTime = millis();

  // 去抖动：忽略短时间内的重复按下
  if (buttonPressTime - lastButtonPress > 50) {
    buttonPressed = true;
    lastButtonPress = buttonPressTime;
  }
}
