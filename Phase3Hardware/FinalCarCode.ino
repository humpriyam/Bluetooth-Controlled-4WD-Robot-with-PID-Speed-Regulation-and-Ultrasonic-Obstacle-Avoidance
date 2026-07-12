#include <PID_v1.h>

// --- Pin Definitions ---
// Motor Driver 1 (Front Motors)
const int ENA = 4, IN1 = 22, IN2 = 23;  // Front Right
const int ENB = 5, IN3 = 24, IN4 = 25;  // Front Left

// Motor Driver 2 (Rear Motors)
const int ENC = 6, IN5 = 26, IN6 = 27;  // Rear Left
const int END = 7, IN7 = 28, IN8 = 29;  // Rear Right

// Ultrasonic Sensor Pins
const int TRIG = 52, ECHO = 53;

// Encoder Pins (Arduino Mega Interrupt Pins: 2, 3, 18, 19, 20, 21)
const int ENC_FL = 2, ENC_FR = 3, ENC_RL = 18, ENC_RR = 19;

// --- PID and Speed Variables ---
double setpoint = 0;               // Target speed (pulses per 100ms)
double rpmFL, rpmFR, rpmRL, rpmRR; // Feedback (Current pulses)
double outFL, outFR, outRL, outRR; // PWM Output (0-255)

// PID Tuning: Start with low Kp, and very low Ki/Kd for small robots.
double Kp = 1.5, Ki = 5.0, Kd = 0.1;

PID pidFL(&rpmFL, &outFL, &setpoint, Kp, Ki, Kd, DIRECT);
PID pidFR(&rpmFR, &outFR, &setpoint, Kp, Ki, Kd, DIRECT);
PID pidRL(&rpmRL, &outRL, &setpoint, Kp, Ki, Kd, DIRECT);
PID pidRR(&rpmRR, &outRR, &setpoint, Kp, Ki, Kd, DIRECT);

// Encoder Counters
volatile long pulseFL = 0, pulseFR = 0, pulseRL = 0, pulseRR = 0;
unsigned long lastTime = 0;

char command = 's';
bool obstacleDetected = false;

void setup() {
  Serial.begin(9600);   
  Serial2.begin(9600);  // HC-05 Bluetooth
  
  int pins[] = {ENA, IN1, IN2, ENB, IN3, IN4, ENC, IN5, IN6, END, IN7, IN8};
  for(int i=0; i<12; i++) pinMode(pins[i], OUTPUT);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  pinMode(ENC_FL, INPUT_PULLUP);
  pinMode(ENC_FR, INPUT_PULLUP);
  pinMode(ENC_RL, INPUT_PULLUP);
  pinMode(ENC_RR, INPUT_PULLUP);
  
  attachInterrupt(digitalPinToInterrupt(ENC_FL), []{ pulseFL++; }, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_FR), []{ pulseFR++; }, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_RL), []{ pulseRL++; }, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_RR), []{ pulseRR++; }, RISING);

  pidFL.SetMode(AUTOMATIC); pidFR.SetMode(AUTOMATIC);
  pidRL.SetMode(AUTOMATIC); pidRR.SetMode(AUTOMATIC);
  
  pidFL.SetOutputLimits(0, 255); pidFR.SetOutputLimits(0, 255);
  pidRL.SetOutputLimits(0, 255); pidRR.SetOutputLimits(0, 255);
}

void loop() {
  checkObstacle(); 

  if (Serial2.available()) {
    command = Serial2.read();
  }

  calculateRPM();  
  executeCommand(); 
}

void checkObstacle() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  
  // pulseIn with 25ms timeout (approx 4 meters max distance)
  long duration = pulseIn(ECHO, HIGH, 25000); 
  
  if (duration == 0) {
    // No echo received (out of range/clear path)
    obstacleDetected = false;
  } else {
    float distance = (duration / 2.0) * 0.0343;
    // Obstacle is valid if between 2cm and 40cm
    obstacleDetected = (distance > 2 && distance <= 40);
  }
}

void calculateRPM() {
  unsigned long now = millis();
  if (now - lastTime >= 100) { 
    // Atomic read of volatile variables
    noInterrupts();
    rpmFL = pulseFL; rpmFR = pulseFR;
    rpmRL = pulseRL; rpmRR = pulseRR;
    pulseFL = pulseFR = pulseRL = pulseRR = 0;
    interrupts();

    lastTime = now;
  }
}

void executeCommand() {
  // --- A. Auto-Avoidance Mode ---
  if (command == 'a' || command == 'A') {
    if (obstacleDetected) {
      setpoint = 60; 
      moveRight(); // Turn away
    } else {
      setpoint = 100;
      moveForward();
    }
    return;
  }

  // --- B. Safety Override (Manual) ---
  if ((command == 'f' || command == 'F') && obstacleDetected) {
    stopMotors();
    return;
  }

  // --- C. Manual Control ---
  switch (command) {
    case 'f': setpoint = 40;  moveForward(); break; 
    case 'F': setpoint = 80;  moveForward(); break; 
    case 'b': setpoint = 40;  moveBackward(); break; 
    case 'B': setpoint = 80;  moveBackward(); break; 
    case 'l': case 'L': setpoint = 60; moveLeft(); break; 
    case 'r': case 'R': setpoint = 60; moveRight(); break; 
    case 's': case 'S': stopMotors(); break;
  }
}

// --- Direction Control Functions ---

void computeAllPID() {
  pidFL.Compute(); pidFR.Compute(); pidRL.Compute(); pidRR.Compute();
}

void moveForward() {
  computeAllPID();
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  analogWrite(ENA, outFR);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);  analogWrite(ENB, outFL);
  digitalWrite(IN5, HIGH); digitalWrite(IN6, LOW);  analogWrite(ENC, outRL);
  digitalWrite(IN7, HIGH); digitalWrite(IN8, LOW);  analogWrite(END, outRR);
}

void moveBackward() {
  computeAllPID();
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH); analogWrite(ENA, outFR);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH); analogWrite(ENB, outFL);
  digitalWrite(IN5, LOW); digitalWrite(IN6, HIGH); analogWrite(ENC, outRL);
  digitalWrite(IN7, LOW); digitalWrite(IN8, HIGH); analogWrite(END, outRR);
}

void moveRight() {
  computeAllPID();
  // Right wheels back, Left wheels forward
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); analogWrite(ENA, outFR);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);  analogWrite(ENB, outFL);
  digitalWrite(IN5, HIGH); digitalWrite(IN6, LOW);  analogWrite(ENC, outRL);
  digitalWrite(IN7, LOW);  digitalWrite(IN8, HIGH); analogWrite(END, outRR);
}

void moveLeft() {
  computeAllPID();
  // Right wheels forward, Left wheels back
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  analogWrite(ENA, outFR);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); analogWrite(ENB, outFL);
  digitalWrite(IN5, LOW);  digitalWrite(IN6, HIGH); analogWrite(ENC, outRL);
  digitalWrite(IN7, HIGH); digitalWrite(IN8, LOW);  analogWrite(END, outRR);
}

void stopMotors() {
  setpoint = 0;
  analogWrite(ENA, 0); analogWrite(ENB, 0);
  analogWrite(ENC, 0); analogWrite(END, 0);
}