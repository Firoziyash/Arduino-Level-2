/*
 * Bluetooth Controlled Car using HC-05, L298N, and Arduino Uno
 * 
 * Connections:
 * L298N Motor Driver:
 *   - ENA -> Arduino Pin 10 (PWM for speed control - right motors)
 *   - ENB -> Arduino Pin 11 (PWM for speed control - left motors)
 *   - IN1 -> Arduino Pin 9
 *   - IN2 -> Arduino Pin 8
 *   - IN3 -> Arduino Pin 7
 *   - IN4 -> Arduino Pin 6
 *   - VCC -> Battery positive (7-12V)
 *   - GND -> Battery negative & Arduino GND
 * 
 * HC-05 Bluetooth Module:
 *   - VCC -> Arduino 5V
 *   - GND -> Arduino GND
 *   - TX  -> Arduino RX (Pin 0)
 *   - RX  -> Arduino TX (Pin 1)
 */

// Motor Driver Pins
const int ENA = 10;  // Enable pin for right motors (PWM)
const int ENB = 11;  // Enable pin for left motors (PWM)
const int IN1 = 9;   // Right motor input 1
const int IN2 = 8;   // Right motor input 2
const int IN3 = 7;   // Left motor input 1
const int IN4 = 6;   // Left motor input 2

// Motor Speed (0-255)
int motorSpeed = 200;  // Default speed

// Bluetooth data
char command;

void setup() {
  // Set motor control pins as outputs
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  // Initialize motors to stopped state
  stopMotors();
  
  // Initialize Bluetooth serial communication (for HC-05)
  Serial.begin(9600);
  
  // Optional: Print ready message (won't be sent to phone)
  // but can be seen in Serial Monitor
  Serial.println("Bluetooth Car Ready! Send commands:");
  Serial.println("F - Forward");
  Serial.println("B - Backward");
  Serial.println("L - Left");
  Serial.println("R - Right");
  Serial.println("S - Stop");
  Serial.println("X - Increase Speed");
  Serial.println("Z - Decrease Speed");
}

void loop() {
  // Check if data is available from Bluetooth
  if (Serial.available() > 0) {
    command = Serial.read();
    
    // Execute command
    switch(command) {
      case 'F':  // Forward
      case 'f':
        moveForward();
        break;
        
      case 'B':  // Backward
      case 'b':
        moveBackward();
        break;
        
      case 'L':  // Left turn
      case 'l':
        turnLeft();
        break;
        
      case 'R':  // Right turn
      case 'r':
        turnRight();
        break;
        
      case 'S':  // Stop
      case 's':
        stopMotors();
        break;
        
      case 'X':  // Increase speed
      case 'x':
        increaseSpeed();
        break;
        
      case 'Z':  // Decrease speed
      case 'z':
        decreaseSpeed();
        break;
        
      default:
        // Unknown command - do nothing
        break;
    }
  }
}

// Function to move forward
void moveForward() {
  // Right motor forward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  
  // Left motor forward
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  
  // Set speed
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);
  
  Serial.println("Moving Forward");
}

// Function to move backward
void moveBackward() {
  // Right motor backward
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  
  // Left motor backward
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  
  // Set speed
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);
  
  Serial.println("Moving Backward");
}

// Function to turn left
void turnLeft() {
  // Right motor forward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  
  // Left motor backward
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  
  // Set speed (may want slower turning speed)
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);
  
  Serial.println("Turning Left");
}

// Function to turn right
void turnRight() {
  // Right motor backward
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  
  // Left motor forward
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  
  // Set speed
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);
  
  Serial.println("Turning Right");
}

// Function to stop motors
void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  
  Serial.println("Stopped");
}

// Increase motor speed
void increaseSpeed() {
  if (motorSpeed < 250) {
    motorSpeed += 25;
    Serial.print("Speed Increased to: ");
    Serial.println(motorSpeed);
  } else {
    Serial.println("Maximum Speed Reached!");
  }
}

// Decrease motor speed
void decreaseSpeed() {
  if (motorSpeed > 50) {
    motorSpeed -= 25;
    Serial.print("Speed Decreased to: ");
    Serial.println(motorSpeed);
  } else {
    Serial.println("Minimum Speed Reached!");
  }
}