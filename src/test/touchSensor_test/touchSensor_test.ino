/*
 * This ESP8266 NodeMCU code was developed by newbiely.com
 *
 * This ESP8266 NodeMCU code is made available for public use without any restriction
 *
 * For comprehensive instructions and wiring diagrams, please visit:
 * https://newbiely.com/tutorials/esp8266/esp8266-touch-sensor
 */

#define STUDY_TOUCH_SENSOR 4 // The ESP8266 NodeMCU input pin that connects to the sensor's SIGNAL pin
#define REST_TOUCH_SENSOR 5  // D2 对应于 GPIO4
void setup() {
  // Initialize the Serial to communicate with the Serial Monitor.
  Serial.begin(9600);
  // initialize the ESP8266 NodeMCU's pin as an input
  pinMode(STUDY_TOUCH_SENSOR, INPUT);

  // initialize the ESP8266 NodeMCU's pin as an input
  pinMode(REST_TOUCH_SENSOR, INPUT);
}

void loop() {
  //// Study Sensor
  // Static variables to track the button state
  static int studyLastState = LOW;    // Track the last state of the button
  int studyCurrentState = digitalRead(STUDY_TOUCH_SENSOR); // Read the current state of the button
  // Detect a state change: LOW -> HIGH (button pressed)
  if (studyLastState == LOW && studyCurrentState == HIGH) {
    Serial.println("Study Touch Sensor Pressed!"); // Print message when button is pressed
  }
  studyLastState = studyCurrentState;


  ////Rest Sensor
  // Static variables to track the button state
  static int restLastState = LOW;    // Track the last state of the button
  int restCurrentState = digitalRead(REST_TOUCH_SENSOR); // Read the current state of the button
  // Detect a state change: LOW -> HIGH (button pressed)
  if (restLastState == LOW && restCurrentState == HIGH) {
    Serial.println("Rest Touch Sensor Pressed!"); // Print message when button is pressed
  }
  restLastState = restCurrentState;

}
