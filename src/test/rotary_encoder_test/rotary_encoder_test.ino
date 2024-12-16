// Define the pins for the rotary encoder
#define CLK_PIN 14  // CLK connected to D5 (GPIO14)
#define DT_PIN 12   // DT connected to D6 (GPIO12)
#define SW_PIN 13   // SW connected to D7 (GPIO13)

int lastCLK = LOW;       // Store the previous state of the CLK pin
int lastSW = HIGH;       // Store the previous state of the button (SW) pin

void setup() {
  pinMode(CLK_PIN, INPUT);
  pinMode(DT_PIN, INPUT);
  pinMode(SW_PIN, INPUT_PULLUP); // Internal pull-up resistor
  Serial.begin(115200);
  Serial.println("Rotary Encoder Initialized with D5, D6, D7");
}

void loop() {
  // Detect rotary encoder rotation
  int currentCLK = digitalRead(CLK_PIN);
  if (currentCLK != lastCLK && currentCLK == HIGH) { // Detect rising edge
    if (digitalRead(DT_PIN) != currentCLK) {
      Serial.println("Rotated CW (Clockwise)"); // Clockwise rotation
    } else {
      Serial.println("Rotated CCW (Counterclockwise)"); // Counterclockwise rotation
    }
  }
  lastCLK = currentCLK;

  // Detect button press
  int currentSW = digitalRead(SW_PIN);
  if (currentSW == LOW && lastSW == HIGH) { // Detect press event
    Serial.println("Button Pressed!");
  }
  lastSW = currentSW;
}
