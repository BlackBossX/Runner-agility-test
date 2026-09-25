#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// OLED SPI pins
#define OLED_MOSI   11 // SDA pin on your display
#define OLED_CLK    13 // SCK pin on your display
#define OLED_DC     8  // DC pin
#define OLED_CS     7  // CS pin
#define OLED_RESET  6  // RES pin

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// Ultrasonic sensor pins
#define TRIG1_PIN 9
#define ECHO1_PIN 10
#define TRIG2_PIN A0
#define ECHO2_PIN A1
#define TRIG3_PIN A2
#define ECHO3_PIN A3
#define TRIG4_PIN A4
#define ECHO4_PIN A5

// LEDs and Button
#define LED_A_PIN 3
#define LED_B_PIN 4
#define BUTTON_PIN 2

enum State {
  CALIBRATING,
  READY,
  WAITING_TO_LEAVE,
  RUNNING_TO_VBEND,
  RUNNING_TO_END,
  FINISHED
};

State currentState = CALIBRATING;
unsigned long startTime = 0;
unsigned long finalTime = 0;
unsigned long runningCooldown = 0;

bool lastButtonState = HIGH;
int chosenPath = 0; // 0 for Path A, 1 for Path B

// Helper function to dynamically center text on the screen
void printCentered(String text, int y, int textSize) {
  display.setTextSize(textSize);
  int width = text.length() * 6 * textSize;
  display.setCursor((SCREEN_WIDTH - width) / 2, y);
  display.print(text);
}

void setup() {
  Serial.begin(9600);
  
  // Set up all 4 sensors
  pinMode(TRIG1_PIN, OUTPUT);
  pinMode(ECHO1_PIN, INPUT);
  pinMode(TRIG2_PIN, OUTPUT);
  pinMode(ECHO2_PIN, INPUT);
  pinMode(TRIG3_PIN, OUTPUT);
  pinMode(ECHO3_PIN, INPUT);
  pinMode(TRIG4_PIN, OUTPUT);
  pinMode(ECHO4_PIN, INPUT);
  
  // Set up LEDs
  pinMode(LED_A_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);
  digitalWrite(LED_A_PIN, LOW);
  digitalWrite(LED_B_PIN, LOW);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC)) { 
    for(;;); // Halt if display fails
  }
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  printCentered("BOOTING", 24, 2);
  display.display();
  delay(1000);
}

long getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000); // 30ms timeout ~ 5 meters max
  if (duration == 0) return 999; // Out of range or no echo
  
  long distance = duration * 0.034 / 2;
  return distance;
}

void loop() {
  bool buttonPressed = false;
  
  // 1. Check physical button (with simple debounce)
  bool currentButtonState = digitalRead(BUTTON_PIN);
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    buttonPressed = true;
    delay(50); // basic debounce
  }
  lastButtonState = currentButtonState;

  // 2. Check for 'R' command from Serial Monitor
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'R' || c == 'r') {
      buttonPressed = true;
    }
  }

  // Handle Button Press: Go to READY state
  if (buttonPressed) {
    if (currentState == CALIBRATING) {
      // Use the unpredictable time of the first button press to seed randomness!
      randomSeed(micros());
    }
    currentState = READY;
    digitalWrite(LED_A_PIN, LOW);
    digitalWrite(LED_B_PIN, LOW);
  }

  display.clearDisplay();

  switch (currentState) {
    // Initial state: Use this to position your 1m bars for all 4 sensors
    case CALIBRATING: {
      long d1 = getDistance(TRIG1_PIN, ECHO1_PIN);
      long d2 = getDistance(TRIG2_PIN, ECHO2_PIN);
      long d3 = getDistance(TRIG3_PIN, ECHO3_PIN);
      long d4 = getDistance(TRIG4_PIN, ECHO4_PIN);
      
      printCentered("CALIBRATE", 0, 2);
      
      display.setTextSize(1);
      display.setCursor(0, 25);
      display.print("S1(Start) : "); display.print(d1); display.println(" cm");
      display.setCursor(0, 35);
      display.print("S2(V-Bend): "); display.print(d2); display.println(" cm");
      display.setCursor(0, 45);
      display.print("S3(Path A): "); display.print(d3); display.println(" cm");
      display.setCursor(0, 55);
      display.print("S4(Path B): "); display.print(d4); display.println(" cm");
      break;
    }

    case READY: {
      // Only ping the first sensor to save time
      long d1 = getDistance(TRIG1_PIN, ECHO1_PIN);
      printCentered("READY", 10, 3);
      printCentered(String(d1) + " cm", 40, 2);
      
      // Runner gets into position (less than 1m / 100cm)
      if (d1 > 0 && d1 < 100) {
        currentState = WAITING_TO_LEAVE;
      }
      break;
    }

    case WAITING_TO_LEAVE: {
      long d1 = getDistance(TRIG1_PIN, ECHO1_PIN);
      printCentered("SET...", 10, 3);
      printCentered(String(d1) + " cm", 40, 2);
      
      // Runner leaves the starting line
      if (d1 >= 100) {
        startTime = millis();
        currentState = RUNNING_TO_VBEND;
        runningCooldown = millis(); 
      }
      break;
    }

    case RUNNING_TO_VBEND: {
      long d2 = getDistance(TRIG2_PIN, ECHO2_PIN);
      unsigned long elapsed = millis() - startTime;
      
      printCentered("RUNNING", 5, 2);
      printCentered(String(elapsed / 1000.0, 2) + "s", 30, 3);
      
      // 1.5s cooldown so the runner's legs don't immediately trigger
      // something as they leave the starting line.
      if (millis() - runningCooldown > 1500) {
        // Reached the V-Bend (Sensor 2)
        if (d2 > 0 && d2 < 100) {
          // Choose a random path!
          chosenPath = random(0, 2); // 0 or 1
          
          if (chosenPath == 0) {
            digitalWrite(LED_A_PIN, HIGH);
          } else {
            digitalWrite(LED_B_PIN, HIGH);
          }
          
          currentState = RUNNING_TO_END;
          runningCooldown = millis();
        }
      }
      break;
    }
    
    case RUNNING_TO_END: {
      long dEnd = 999;
      // We only care about checking the sensor on the path they were TOLD to run
      if (chosenPath == 0) {
        dEnd = getDistance(TRIG3_PIN, ECHO3_PIN);
      } else {
        dEnd = getDistance(TRIG4_PIN, ECHO4_PIN);
      }
      
      unsigned long elapsed = millis() - startTime;
      
      printCentered(chosenPath == 0 ? "PATH A" : "PATH B", 5, 2);
      printCentered(String(elapsed / 1000.0, 2) + "s", 30, 3);
      
      // Short 1s cooldown to prevent arm/leg noise when passing Sensor 2
      if (millis() - runningCooldown > 1000) {
        // Runner crossed the correct finish line!
        if (dEnd > 0 && dEnd < 100) {
          finalTime = elapsed;
          digitalWrite(LED_A_PIN, LOW);
          digitalWrite(LED_B_PIN, LOW);
          currentState = FINISHED;
        }
      }
      break;
    }

    case FINISHED: {
      printCentered("TIME!", 5, 2);
      printCentered(String(finalTime / 1000.0, 2) + "s", 30, 3);
      break;
    }
  }
  
  display.display();
  delay(10); // Small delay for loop stability
}