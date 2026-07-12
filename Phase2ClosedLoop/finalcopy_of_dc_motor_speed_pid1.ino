#include <Encoder.h> 

// Motor Pins
const int motorPin1 = 5; 
const int motorPin2 = 6; 
const int driverICEnablePin = 9; 

// Encoder Pins
const int encoderChA = 2; 
const int encoderChB = 3; 

// Control Pins
const int speedDownButton = 11; 
const int speedUpButton = 10;  

// Variables to track button states
bool lastDownState = LOW;
bool lastUpState = LOW;

// Data Variables
volatile long totalPulses = 0; 
int motorPWM = 60;
int motorSetSpeed = 10; 
int encoder_pos = 0;
float gearRatio = 180.0;  
float encoderPPR = 12.0; 
int gearBoxoutputSpeedRPM;   

unsigned long timeStamp = 0;
unsigned long timeElapsed = 0; 
int sampleRate = 200; 

// PID Math
double error = 0, error_I = 0, error_D = 0, past_error = 0;
const float Kp = 2.0, Ki = 5.0, Kd = 2.0;  

void setup() {
  pinMode(encoderChA, INPUT_PULLUP);  
  pinMode(encoderChB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encoderChA), encoderISR, RISING);
  
  pinMode(speedDownButton, INPUT);
  pinMode(speedUpButton, INPUT); 
  
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(driverICEnablePin, OUTPUT);
  
  digitalWrite(driverICEnablePin, HIGH); 
  Serial.begin(9600);  
}

void loop() {
  // --- INCREMENTAL SPEED CONTROL ---
  bool currentDownState = digitalRead(speedDownButton);
  if (currentDownState == HIGH && lastDownState == LOW) {
    motorSetSpeed -= 10;
    if (motorSetSpeed < 0) motorSetSpeed = 0; 
    delay(50); 
  }
  lastDownState = currentDownState;

  bool currentUpState = digitalRead(speedUpButton);
  if (currentUpState == HIGH && lastUpState == LOW) {
    motorSetSpeed += 10;
    if (motorSetSpeed > 200) motorSetSpeed = 200; 
    delay(50); 
  }
  lastUpState = currentUpState;

  // --- LOGIC ---
  timeElapsed = millis() - timeStamp; 
  if (timeElapsed >= sampleRate) {
    getSpeed(); 
    computerPID(); 
    timeStamp = millis(); 
  }

  MotorCCW(0, motorPWM); 

  // --- PLOTTER OUTPUT ---
  // Format: SetSpeed, ActualRPM
  Serial.print(motorSetSpeed);
  Serial.print(",");
  Serial.println(gearBoxoutputSpeedRPM);
  
  delay(10); 
}

void encoderISR() {   
  if (digitalRead(encoderChB) == HIGH) {
    encoder_pos++;
    totalPulses++;
  } else {
    encoder_pos--;
    totalPulses--;
  }  
}

void getSpeed() {  
  float countSpeed_PPM = 1000.0 * 60.0 * (float)encoder_pos / (timeElapsed); 
  encoder_pos = 0; 
  float encoderSpeedRPM = countSpeed_PPM / encoderPPR; 
  gearBoxoutputSpeedRPM = encoderSpeedRPM / gearRatio; 
}

void computerPID() {
  error = (motorSetSpeed - gearBoxoutputSpeedRPM); 
  error_I += error; 
  past_error = error;
  if (error_I > 200) error_I = 200; 
  if (error_I < -200) error_I = -200;

  motorPWM = (Kp * error) + (Ki * error_I) + (Kd * error_D);
  motorPWM = constrain(motorPWM, 0, 255);
}

void MotorCCW(int dir, int pwm) {
  digitalWrite(motorPin1, dir);
  analogWrite(motorPin2, pwm);
}