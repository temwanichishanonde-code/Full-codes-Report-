#include <LiquidCrystal.h>

// LCD Keypad Shield pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// ───────── MOTOR PINS ─────────
const int ENA = 11;
const int IN1 = A1;   // Left motor IN1
const int IN2 = 2;    // Left motor IN2
const int IN3 = 12;   // Right motor IN3
const int IN4 = 13;   // Right motor IN4
const int ENB = 3;    // Right motor speed

// ───────── LINE TRACKING SENSORS ─────────
const int LEFT_IR  = A2;
const int RIGHT_IR = A4;

// ───────── LM393 SPEED SENSORS ─────────
const int LEFT_ENCODER_DIGITAL = 1;   // LEFT sensor DO → Pin 1
const int RIGHT_ENCODER_ANALOG = A3;  // RIGHT sensor AO → A3

// ───────── ENCODER VARIABLES ─────────
volatile unsigned long leftPulseCount = 0;
volatile unsigned long rightPulseCount = 0;
unsigned long lastLeftCount = 0;
unsigned long lastRightCount = 0;
int lastRightAnalogValue = 0;
const int ANALOG_THRESHOLD = 100;

// ───────── ROBOT CONSTANTS ─────────
const float WHEEL_DIAMETER = 6.5;
const float WHEEL_CIRCUMFERENCE = WHEEL_DIAMETER * 3.1416;
const int SLOTS_PER_REVOLUTION = 20;
const float CM_PER_PULSE = WHEEL_CIRCUMFERENCE / SLOTS_PER_REVOLUTION;

// ───────── MOTOR SPEED SETTINGS ─────────
const int BASE_SPEED = 180;      // Base speed for straight
const int TURN_SPEED = 200;      // Faster turns
const int SHARP_TURN_SPEED = 300; // For 90-degree turns

// ───────── LINE FOLLOWING TUNING ─────────
const unsigned long TURN_DELAY = 15;    // Fast response
bool wasOnLine = false;
unsigned long lineLostTime = 0;
const unsigned long LINE_LOST_TIMEOUT = 1000;

// ───────── DISTANCE & TIMER ─────────
float totalDistance = 0.0;
float leftDistance = 0.0;
float rightDistance = 0.0;
unsigned long programStartTime = 0;
unsigned long totalPausedTime = 0;
unsigned long pauseStartTime = 0;
bool isPaused = false;
unsigned long elapsedSeconds = 0;
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_INTERVAL = 500;

// ───────── 510CM STOP FEATURE ─────────
const float STOP_DISTANCE = 510.0;      // Stop after 510 cm
bool hasStoppedForDistance = false;
bool isStoppingForDistance = false;
unsigned long stopStartTime = 0;
const unsigned long STOP_DURATION = 2000;  // Stop for 2 seconds

// ───────── INTERRUPT FOR LEFT ENCODER ─────────
void leftEncoderISR() {
  leftPulseCount++;
}

// ───────── POLLING FOR RIGHT ENCODER ─────────
void pollRightEncoder() {
  int currentValue = analogRead(RIGHT_ENCODER_ANALOG);
  if (abs(currentValue - lastRightAnalogValue) > ANALOG_THRESHOLD) {
    rightPulseCount++;
    lastRightAnalogValue = currentValue;
  }
}

// ───────── AGGRESSIVE MOTOR FUNCTIONS FOR 90° TURNS ─────────
void moveForward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, BASE_SPEED);
  analogWrite(ENB, BASE_SPEED);
}

// Pivot turn - one wheel forward, one reverse (for sharp 90° turns)
void pivotLeft() {
  // CORRECTED: Left wheel reverse, Right wheel forward
  digitalWrite(IN1, HIGH);   // Left REVERSE
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);   // Right FORWARD
  digitalWrite(IN4, LOW);
  analogWrite(ENA, SHARP_TURN_SPEED);
  analogWrite(ENB, SHARP_TURN_SPEED);
}

void pivotRight() {
  // Left wheel forward, Right wheel reverse
  digitalWrite(IN1, LOW);    // Left FORWARD
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);    // Right REVERSE
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, SHARP_TURN_SPEED);
  analogWrite(ENB, SHARP_TURN_SPEED);
}

// Standard turn - one wheel fast, one wheel slow
void turnLeft() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);   // Left forward slow
  digitalWrite(IN3, HIGH);   // Right forward fast
  digitalWrite(IN4, LOW);
  analogWrite(ENA, BASE_SPEED/2);
  analogWrite(ENB, TURN_SPEED);
}

void turnRight() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);   // Left forward fast
  digitalWrite(IN3, HIGH);   // Right forward slow
  digitalWrite(IN4, LOW);
  analogWrite(ENA, TURN_SPEED);
  analogWrite(ENB, BASE_SPEED/2);
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// ───────── TIMER FUNCTIONS ─────────
void startTimer() {
  if (isPaused) {
    totalPausedTime += millis() - pauseStartTime;
    isPaused = false;
  }
}

void pauseTimer() {
  if (!isPaused) {
    pauseStartTime = millis();
    isPaused = true;
  }
}

unsigned long getElapsedTime() {
  unsigned long currentTime = millis();
  if (isPaused) {
    return (pauseStartTime - programStartTime - totalPausedTime) / 1000;
  } else {
    return (currentTime - programStartTime - totalPausedTime) / 1000;
  }
}

void resetTimer() {
  programStartTime = millis();
  totalPausedTime = 0;
  pauseStartTime = 0;
  isPaused = false;
  elapsedSeconds = 0;
}

// ───────── DISTANCE CALCULATION ─────────
void updateDistance() {
  pollRightEncoder();
  
  unsigned long currentLeft = leftPulseCount;
  unsigned long currentRight = rightPulseCount;
  
  unsigned long leftDelta = currentLeft - lastLeftCount;
  unsigned long rightDelta = currentRight - lastRightCount;
  
  leftDistance += leftDelta * CM_PER_PULSE;
  rightDistance += rightDelta * CM_PER_PULSE;
  
  totalDistance = (leftDistance + rightDistance) / 2.0;
  
  lastLeftCount = currentLeft;
  lastRightCount = currentRight;
}

void resetDistance() {
  leftPulseCount = 0;
  rightPulseCount = 0;
  lastLeftCount = 0;
  lastRightCount = 0;
  totalDistance = 0.0;
  leftDistance = 0.0;
  rightDistance = 0.0;
  lastRightAnalogValue = analogRead(RIGHT_ENCODER_ANALOG);
  hasStoppedForDistance = false;
  isStoppingForDistance = false;
}

// ───────── 510CM STOP CHECK ─────────
void check510cmStop() {
  if (!hasStoppedForDistance && !isStoppingForDistance) {
    if (totalDistance >= STOP_DISTANCE) {
      isStoppingForDistance = true;
      hasStoppedForDistance = true;
      stopStartTime = millis();
      stopMotors();
      pauseTimer();
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("510cm Reached!");
      lcd.setCursor(0, 1);
      lcd.print("Wait 2 sec...");
    }
  }
  
  if (isStoppingForDistance && millis() - stopStartTime >= STOP_DURATION) {
    isStoppingForDistance = false;
    startTimer();
    lcd.clear();
    lastDisplayUpdate = millis();
  }
}

// ───────── IMPROVED LINE FOLLOWING WITH 90° TURN DETECTION ─────────
void followLine(int leftVal, int rightVal) {
  static int lastLeft = 0;
  static int lastRight = 0;
  
  bool lineJustLost = (lastLeft == 1 || lastRight == 1) && (leftVal == 0 && rightVal == 0);
  
  if (lineJustLost) {
    lineLostTime = millis();
  }
  
  lastLeft = leftVal;
  lastRight = rightVal;
  
  if (leftVal == 0 && rightVal == 0) {
    // Both sensors see WHITE - line NOT detected
    
    // Check for 90° turn (line lost for >200ms)
    if (wasOnLine && (millis() - lineLostTime > 150)) {
      // 90° TURN DETECTED - Use pivot turn
      if (lastLeft == 1) {
        // Last saw line on LEFT → Pivot RIGHT to find it
        pivotRight();
      } else if (lastRight == 1) {
        // Last saw line on RIGHT → Pivot LEFT to find it
        pivotLeft();
      }
    } 
    else if (wasOnLine && (millis() - lineLostTime <= 200)) {
      // Recently lost line - gentle search turn
      if (lastLeft == 1) {
        turnRight();
      } else if (lastRight == 1) {
        turnLeft();
      }
    }
    else {
      // Normal forward - line centered
      moveForward();
    }
    
    startTimer();
  }
  else if (leftVal == 1 && rightVal == 0) {
    // Left sensor on line - PIVOT RIGHT for sharp correction
    pivotRight();
    wasOnLine = true;
    lineLostTime = 0;
    startTimer();
  }
  else if (leftVal == 0 && rightVal == 1) {
    // Right sensor on line - PIVOT LEFT for sharp correction
    pivotLeft();
    wasOnLine = true;
    lineLostTime = 0;
    startTimer();
  }
  else if (leftVal == 1 && rightVal == 1) {
    // Both sensors on line (intersection)
    stopMotors();
    pauseTimer();
    wasOnLine = true;
    lineLostTime = 0;
  }
  
  // Update wasOnLine for next iteration
  if (leftVal == 0 && rightVal == 0) {
    wasOnLine = false;
  }
}

// ───────── DISPLAY FUNCTIONS ─────────
void updateDisplay() {
  elapsedSeconds = getElapsedTime();
  updateDistance();
  
  if (isStoppingForDistance) {
    unsigned long timeLeft = STOP_DURATION - (millis() - stopStartTime);
    lcd.setCursor(0, 1);
    lcd.print("Resume in ");
    lcd.print((timeLeft + 999) / 1000);
    lcd.print("s   ");
    return;
  }
  
  if (millis() - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    lastDisplayUpdate = millis();
    
    lcd.clear();
    
    // Line 1: Time
    lcd.setCursor(0, 0);
    lcd.print("Time: ");
    lcd.print(elapsedSeconds);
    lcd.print("s");
    
    // Show if paused
    if (isPaused && !isStoppingForDistance) {
      lcd.setCursor(12, 0);
      lcd.print("PAU");
    }
    
    // Line 2: Distance
    lcd.setCursor(0, 1);
    lcd.print("Dist: ");
    
    if (totalDistance < 100) {
      if (totalDistance < 10) lcd.print(" ");
      lcd.print(totalDistance, 1);
      lcd.print("cm");
    } else {
      lcd.print(totalDistance / 100.0, 1);
      lcd.print("m");
    }
    
    // Show "510D" after completing stop
    if (hasStoppedForDistance) {
      lcd.setCursor(12, 0);
      lcd.print("510");
    }
  }
}

void resetAll() {
  resetTimer();
  resetDistance();
  wasOnLine = false;
  lineLostTime = 0;
}

// ───────── SETUP ─────────
void setup() {
  // Motor pins
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);
  
  // Line sensors
  pinMode(LEFT_IR, INPUT);
  pinMode(RIGHT_IR, INPUT);
  
  // Encoder setup
  pinMode(LEFT_ENCODER_DIGITAL, INPUT_PULLUP);
  pinMode(RIGHT_ENCODER_ANALOG, INPUT);
  
  // Attach interrupt
  attachInterrupt(digitalPinToInterrupt(LEFT_ENCODER_DIGITAL), leftEncoderISR, FALLING);
  
  // LCD Setup
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("READY");
  delay(1000);
  
  // Initialize
  resetAll();
  lastDisplayUpdate = millis();
  lastRightAnalogValue = analogRead(RIGHT_ENCODER_ANALOG);
  
  // Show initial display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Time: 0s");
  lcd.setCursor(0, 1);
  lcd.print("Dist: 0.0cm");
}

// ───────── MAIN LOOP ─────────
void loop() {
  // Check if we need to stop at 510cm
  check510cmStop();
  
  if (isStoppingForDistance) {
    delay(100);
    return;
  }
  
  int leftVal  = digitalRead(LEFT_IR);
  int rightVal = digitalRead(RIGHT_IR);
  
  // Use aggressive line following with 90° turn detection
  followLine(leftVal, rightVal);
  
  updateDisplay();
  
  delay(TURN_DELAY);
} 