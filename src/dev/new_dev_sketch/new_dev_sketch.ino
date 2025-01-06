/********* ----------------------------------------- Header ----------------------------------------- *********/
// #include <WiFiNINA.h>   
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "arduino_secrets.h" 

/********* ----------------------------------------- Pin Definitions ----------------------------------------- *********/
// Touch Sensors
#define STUDY_TOUCH_SENSOR 4    // Study mode touch sensor
#define REST_TOUCH_SENSOR 5     // Rest mode touch sensor

// Rotary Encoder
#define CLK_PIN 14             // CLK connected to GPIO14
#define DT_PIN 12              // DT connected to GPIO12
#define SW_PIN 13              // SW connected to GPIO13

/********* ----------------------------------------- Constants & Variables ----------------------------------------- *********/
// System state enumeration
enum TimerState {
    STUDY,      // Study mode
    REST,       // Rest mode
    IDLE,       // Idle state
    PAUSED      // Paused state
};

// WiFi & MQTT Settings
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;
const char* mqtt_username = SECRET_MQTTUSER;
const char* mqtt_password = SECRET_MQTTPASS;
const char* mqtt_server = "mqtt.cetools.org";
const int mqtt_port = 1884;

// Global variables
TimerState currentState = IDLE;
const int SINGLE_LIGHT = 1;  // Control only light #1
const char* MQTT_TOPIC = "student/CASA0014/light/1/pixel/";
const int TIME_STEP = 5;    // Adjust 5 minutes each time

// WiFi & MQTT clients
WiFiClient wifiClient;
PubSubClient client(wifiClient);

/********* ----------------------------------------- Light Control ----------------------------------------- *********/
void controlLight(int R, int G, int B) {
    static int lastR = -1, lastG = -1, lastB = -1;
    
    if (R != lastR || G != lastG || B != lastB) {
        char mqtt_message[100];
        sprintf(mqtt_message, "{\"pixelid\": 0, \"R\": %d, \"G\": %d, \"B\": %d, \"W\": 0}", 
                R, G, B);
        
        if (client.publish(MQTT_TOPIC, mqtt_message)) {
            lastR = R;
            lastG = G;
            lastB = B;
            Serial.println("Light state updated successfully");
        } else {
            Serial.println("Failed to update light state");
        }
    }
}

/********* ----------------------------------------- Timer Class ----------------------------------------- *********/
class Timer {
private:
    unsigned long study_time;     // Adjustable study time
    unsigned long rest_time;      // Adjustable rest time
    unsigned long startTime = 0;
    unsigned long pausedTime = 0;
    unsigned long timerDuration = 0;
    bool isPaused = false;
    
    // Variables for time adjustment
    int lastEncoderState;
    TimerState selectedMode = STUDY;  // Default to study mode
    TimerState pausedFromState;    // Record which state was paused from
    TimerState nextState;          // Record which state to switch to next
    unsigned long study_remaining; // Remaining time for study mode
    unsigned long rest_remaining;  // Remaining time for rest mode
    const int maxLights = 12;           // Maximum number of lights
    int currentLightCount = maxLights; // Current number of lights in use
    int lastCLK = LOW;  // Store the previous state of CLK pin
    int currentCLK;
    int lastDT = LOW;    // Add DT state tracking
    unsigned long lastDebounceTime = 0;  // For debouncing
    const unsigned long debounceDelay = 1;  // Debounce time in ms
    unsigned long lastPrintTime = 0;    // Track when we last printed the time
    const unsigned long PRINT_INTERVAL = 5000;  // Print every 5 seconds

public:
    Timer() {
        study_time = 25 * 60 * 1000;  // Default 25 minutes
        rest_time = 5 * 60 * 1000;    // Default 5 minutes
        lastCLK = digitalRead(CLK_PIN);
        lastDT = digitalRead(DT_PIN);
        study_remaining = study_time;
        rest_remaining = rest_time;
        pausedFromState = IDLE;
        nextState = IDLE;
        currentLightCount = maxLights;
    }

    void handleEncoder() {
        if (currentState != IDLE && currentState != PAUSED) return;

        currentCLK = digitalRead(CLK_PIN);
        int currentDT = digitalRead(DT_PIN);
        
        // Check if enough time has passed since last change
        if ((millis() - lastDebounceTime) > debounceDelay) {
            // If CLK changed, check if it's a valid rotation
            if (currentCLK != lastCLK) {
                lastDebounceTime = millis();  // Reset debounce timer
                
                // If CLK changed first, use DT to determine direction
                if (currentCLK == HIGH) {
                    if (currentDT != currentCLK) {
                        adjustTime(true);   // Clockwise
                    } else {
                        adjustTime(false);  // Counter-clockwise
                    }
                }
            }
        }
        
        lastCLK = currentCLK;
        lastDT = currentDT;
    }

    void adjustTime(bool increase) {
        unsigned long adjustment = 60 * 1000;  // 1 minute in milliseconds
        
        if (selectedMode == STUDY) {
            if (increase && study_time < 60 * 60 * 1000) {  // Max 1 hour
                study_time += adjustment;
                study_remaining = study_time;  // Update remaining time as well
            } else if (!increase && study_time > 1 * 60 * 1000) {  // Min 1 minute
                study_time -= adjustment;
                study_remaining = study_time;
            }
            Serial.print("[Settings] Study time: ");
            Serial.print(study_time / 60000);
            Serial.println(" minutes");
        } else {
            if (increase && rest_time < 30 * 60 * 1000) {  // Max 30 minutes
                rest_time += adjustment;
                rest_remaining = rest_time;
            } else if (!increase && rest_time > 1 * 60 * 1000) {  // Min 1 minute
                rest_time -= adjustment;
                rest_remaining = rest_time;
            }
            Serial.print("[Settings] Rest time: ");
            Serial.print(rest_time / 60000);
            Serial.println(" minutes");
        }
    }

    void start(TimerState state) {
        currentState = state;
        startTime = millis();
        lastPrintTime = startTime;  // Reset the print timer when starting
        timerDuration = (state == STUDY) ? study_time : rest_time;
        isPaused = false;
        Serial.print("[State Change] Started ");
        Serial.print(state == STUDY ? "Study" : "Rest");
        Serial.print(" mode - Duration: ");
        Serial.print(timerDuration / 60000);
        Serial.println(" minutes");
        updateLight();
    }

    void togglePause() {
        if (!isPaused) {
            // Enter paused state
            pausedTime = millis();
            isPaused = true;
            pausedFromState = currentState;
            nextState = currentState;  // Add this line: Default to resume to current mode
            
            // Save remaining time
            unsigned long elapsed = millis() - startTime;
            if (currentState == STUDY) {
                study_remaining = timerDuration - elapsed;
                Serial.print("[Pause] Study mode paused - ");
                Serial.print(study_remaining / 60000);
                Serial.println(" minutes remaining");
            } else if (currentState == REST) {
                rest_remaining = timerDuration - elapsed;
                Serial.print("[Pause] Rest mode paused - ");
                Serial.print(rest_remaining / 60000);
                Serial.println(" minutes remaining");
            }
            currentState = PAUSED;
        } else {
            // Resume from paused state
            startTime = millis();
            isPaused = false;
            currentState = nextState;
            Serial.print("[Resume] Resumed to ");
            Serial.print(nextState == STUDY ? "Study" : "Rest");
            Serial.println(" mode");
            // Set corresponding remaining time
            timerDuration = (nextState == STUDY) ? study_remaining : rest_remaining;
        }
        updateLight();
    }

    void switchModeWhilePaused(TimerState newState) {
        if (currentState == PAUSED && newState != pausedFromState) {
            nextState = newState;
            // Update timer duration
            if (newState == STUDY) {
                timerDuration = study_remaining;
                Serial.print("[Mode Switch] Study mode selected - Will resume with ");
                Serial.print(study_remaining / 60000);
                Serial.println(" minutes");
            } else if (newState == REST) {
                timerDuration = rest_remaining;
                Serial.print("[Mode Switch] Rest mode selected - Will resume with ");
                Serial.print(rest_remaining / 60000);
                Serial.println(" minutes");
            }
        }
    }

    void update() {
        if (currentState == IDLE || currentState == PAUSED) return;

        unsigned long currentTime = millis();
        unsigned long elapsed = currentTime - startTime;
        
        // Print remaining time every 5 seconds
        if (currentTime - lastPrintTime >= PRINT_INTERVAL) {
            printRemainingTime(timerDuration - elapsed);
            lastPrintTime = currentTime;
        }

        if (elapsed >= timerDuration) {
            currentState = IDLE;
            if (pausedFromState == STUDY) {
                study_remaining = study_time;
                Serial.println("[Complete] Study session finished!");
            } else if (pausedFromState == REST) {
                rest_remaining = rest_time;
                Serial.println("[Complete] Rest session finished!");
            }
            updateLight();
        }
    }

    void printRemainingTime(unsigned long remainingMillis) {
        unsigned long totalSeconds = remainingMillis / 1000;
        int minutes = totalSeconds / 60;
        int seconds = totalSeconds % 60;
        
        Serial.print("[Time Remaining] ");
        if (currentState == STUDY) {
            Serial.print("Study - ");
        } else if (currentState == REST) {
            Serial.print("Rest - ");
        }
        
        // Format minutes and seconds with leading zeros if needed
        if (minutes < 10) Serial.print("0");
        Serial.print(minutes);
        Serial.print(":");
        if (seconds < 10) Serial.print("0");
        Serial.println(seconds);
    }

    void updateLight() {
        switch (currentState) {
            case STUDY:
                setAllPixels(255, 0, 0, 0);  // Study mode shows red
                break;
            case REST:
                setAllPixels(0, 255, 0, 0);  // Rest mode shows green
                break;
            case PAUSED:
                setAllPixels(255, 255, 0, 0);  // Paused state shows yellow
                break;
            case IDLE:
                setAllPixels(0, 0, 0, 0);    // Idle state turns off all lights
                break;
        }
    }

    void setAllPixels(int R, int G, int B, int W) {
        // Set 12 pixels for each light
        for (int pixelIndex = 0; pixelIndex < 12; pixelIndex++) {
            sendmqtt(pixelIndex, R, G, B, W);
        }
    }

    void sendmqtt(int pixelIndex, int R, int G, int B, int W) {
        const char* currentTopic = "student/CASA0014/light/1/pixel/";
        char mqtt_message[100];
        sprintf(mqtt_message, "{\"pixelid\": %d, \"R\": %d, \"G\": %d, \"B\": %d, \"W\": %d}", 
                pixelIndex, R, G, B, W);
        
        if (client.publish(currentTopic, mqtt_message)) {
            // Send successful
        } else {
            // Send failure handling
        }
    }

    // Optional: Add method to set light count
    void setLightCount(int count) {
        if (count > 0 && count <= maxLights) {
            currentLightCount = count;
        }
    }

    // Add new method to set selected mode
    void selectMode(TimerState mode) {
        if (currentState == IDLE) {
            selectedMode = mode;
            // Update light display to show selected mode (using darker colors)
            if (mode == STUDY) {
                setAllPixels(64, 0, 0, 0);  // Dark red indicates selected study mode
                Serial.println("[Select] Study mode selected");
            } else {
                setAllPixels(0, 64, 0, 0);  // Dark green indicates selected rest mode
                Serial.println("[Select] Rest mode selected");
            }
        }
    }

    // Add public method to get current selected mode
    TimerState getSelectedMode() {
        return selectedMode;
    }
};

Timer timer;  // Create Timer instance

/********* ----------------------------------------- Functions ----------------------------------------- *********/
void handleUserInput() {
    static int lastSW = HIGH;
    
    // Handle encoder button
    int currentSW = digitalRead(SW_PIN);
    if (currentSW == LOW && lastSW == HIGH) {
        if (currentState == IDLE) {
            timer.start(timer.getSelectedMode());  // Use getter method to get selected mode
        } else if (currentState == STUDY || currentState == REST || currentState == PAUSED) {
            timer.togglePause();
        }
        delay(200);
    }
    lastSW = currentSW;
    
    // Handle touch sensors
    if (currentState == PAUSED) {
        if (digitalRead(STUDY_TOUCH_SENSOR) == HIGH) {
            timer.switchModeWhilePaused(STUDY);
        } 
        else if (digitalRead(REST_TOUCH_SENSOR) == HIGH) {
            timer.switchModeWhilePaused(REST);
        }
    } else if (currentState == IDLE) {
        // Only change selected mode in idle state
        if (digitalRead(STUDY_TOUCH_SENSOR) == HIGH) {
            timer.selectMode(STUDY);
        } 
        else if (digitalRead(REST_TOUCH_SENSOR) == HIGH) {
            timer.selectMode(REST);
        }
    }

    // Handle encoder rotation
    timer.handleEncoder();
}

void startWifi() {
    Serial.print("Connecting to WiFi");
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nWiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        String clientId = "ArduinoClient-";
        clientId += String(random(0xffff), HEX);
        
        if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("connected");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

/********* ----------------------------------------- Setup & Loop ----------------------------------------- *********/
void setup() {
    Serial.begin(115200);
    delay(1000);

    // Set input pins
    pinMode(STUDY_TOUCH_SENSOR, INPUT);
    pinMode(REST_TOUCH_SENSOR, INPUT);
    pinMode(CLK_PIN, INPUT_PULLUP);  // Use internal pull-up resistor
    pinMode(DT_PIN, INPUT_PULLUP);   // Use internal pull-up resistor
    pinMode(SW_PIN, INPUT_PULLUP);  // Use internal pull-up resistor

    // WiFi and MQTT settings
    startWifi();
    client.setServer(mqtt_server, mqtt_port);
    client.setBufferSize(512);
    
    Serial.println("Setup complete");
}

void loop() {
    if (!client.connected()) {
        reconnectMQTT();
    }
    client.loop();
    
    handleUserInput();
    timer.update();
}

