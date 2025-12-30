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

// ───────── ULTRASONIC SENSOR PINS ─────────
const int TRIG_PIN = A5;  // Trigger pin
const int ECHO_PIN = A0;  // Echo pin

// ───────── ULTRASONIC SETTINGS ─────────
const float OBSTACLE_DISTANCE = 7.0;  // Distance in cm to detect obstacle
const unsigned long OBSTACLE_STOP_DURATION = 2000;  // Stop for 2 seconds
const unsigned long SHARP_TURN_DURATION = 600;  // Duration for sharp left turn

// Motor speed settings
const int FORWARD_SPEED = 180;
const int TURN_SPEED = 255; // Max speed for sharp turn

// State tracking
enum RobotState {
  STATE_MOVING_FORWARD,
  STATE_STOPPED,
  STATE_TURNING_LEFT
};
RobotState currentState = STATE_MOVING_FORWARD;

unsigned long obstacleStopStartTime = 0;

// ───────── ULTRASONIC SENSOR FUNCTION ─────────
float getUltrasonicDistance() {
  // Clear trigger pin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // Send 10us pulse
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Read echo pin
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  
  // Calculate distance (duration/58.0 for cm)
  float distance = duration / 58.0;
  
  // Return large value if out of range
  if (distance <= 0 || distance > 400) {
    return 400.0;
  }
  
  return distance;
}

// ───────── MOTOR CONTROL FUNCTIONS ─────────
void moveForward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, FORWARD_SPEED);
  analogWrite(ENB, FORWARD_SPEED);
}

void sharpLeftTurn() {
  // Sharp left pivot: Left wheel reverse, Right wheel forward
  digitalWrite(IN1, HIGH);   // Left REVERSE
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);   // Right FORWARD
  digitalWrite(IN4, LOW);
  analogWrite(ENA, TURN_SPEED);
  analogWrite(ENB, TURN_SPEED);
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// ───────── OBSTACLE DETECTION AND HANDLING ─────────
void handleObstacle() {
  float distance = getUltrasonicDistance();
  
  switch (currentState) {
    case STATE_MOVING_FORWARD:
      // Check for obstacle while moving forward
      if (distance <= OBSTACLE_DISTANCE && distance > 0) {
        currentState = STATE_STOPPED;
        obstacleStopStartTime = millis();
        stopMotors();
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("OBSTACLE!");
        lcd.setCursor(0, 1);
        lcd.print("Stop 2s");
      } else {
        // No obstacle, keep moving forward
        moveForward();
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("MOVING FORWARD");
        lcd.setCursor(0, 1);
        lcd.print("Dist: ");
        lcd.print(distance, 1);
        lcd.print("cm");
      }
      break;
      
    case STATE_STOPPED:
      // Check if 2 seconds have passed
      if (millis() - obstacleStopStartTime >= OBSTACLE_STOP_DURATION) {
        currentState = STATE_TURNING_LEFT;
        obstacleStopStartTime = millis(); // Reset timer for turn duration
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("SHARP LEFT");
        lcd.setCursor(0, 1);
        lcd.print("TURN...");
      } else {
        // Still waiting, show countdown
        unsigned long timeLeft = OBSTACLE_STOP_DURATION - (millis() - obstacleStopStartTime);
        lcd.setCursor(0, 1);
        lcd.print("Wait ");
        lcd.print((timeLeft + 999) / 1000);
        lcd.print("s   ");
      }
      break;
      
    case STATE_TURNING_LEFT:
      // Perform sharp left turn
      sharpLeftTurn();
      
      // Check if turn duration complete
      if (millis() - obstacleStopStartTime >= SHARP_TURN_DURATION) {
        currentState = STATE_MOVING_FORWARD;
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("TURN COMPLETE");
        lcd.setCursor(0, 1);
        lcd.print("Forward...");
      }
      break;
  }
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
  
  // Ultrasonic sensor pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // LCD Setup
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("STARTING");
  lcd.setCursor(0, 1);
  lcd.print("Obstacle: 7cm");
  delay(2000);
  
  // Start moving forward
  moveForward();
  currentState = STATE_MOVING_FORWARD;
  
  lcd.clear();
}

// ───────── MAIN LOOP ─────────
void loop() {
  handleObstacle();
  delay(50); // Small delay for stability
} this is The HC please do section 3