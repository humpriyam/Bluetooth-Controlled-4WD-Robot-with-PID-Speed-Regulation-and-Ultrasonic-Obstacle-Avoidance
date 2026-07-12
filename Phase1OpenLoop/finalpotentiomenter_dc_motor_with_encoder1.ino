#include <Encoder.h>

// Encoder on pins 2 and 3 for hardware interrupts
Encoder encoder(2, 3);

// Pin Definitions
const int potPin = A2;    
const int lpIn4 = 5;      
const int lpIn3 = 6;      

// --- TINKERCAD MOTOR CONSTANTS ---
// Most Tinkercad encoder motors have 360 pulses per output shaft revolution
const float PPR = 360.0;          
const float MAX_MOTOR_RPM = 260.0; 

// Variables for RPM calculation
unsigned long lastTime = 0;
long lastPosition = 0;

void setup() {
  Serial.begin(9600);
  pinMode(lpIn4, OUTPUT);
  pinMode(lpIn3, OUTPUT);
  pinMode(potPin, INPUT); 
  
  // Header for Serial Plotter (Tools > Serial Plotter)
  Serial.println("Desired_RPM,Actual_RPM");
}

void loop() {
  // 1. Read Potentiometer and set Motor Speed
  int potValue = analogRead(potPin);
  int pwmSpeed = map(potValue, 0, 1023, 0, 255);
  
  analogWrite(lpIn4, pwmSpeed);
  digitalWrite(lpIn3, LOW);

  // 2. Calculate RPM every 100 milliseconds
  unsigned long currentTime = millis();
  unsigned long deltaTime = currentTime - lastTime;

  if (deltaTime >= 100) { // Update every 100ms
    long currentPosition = encoder.read();
    long deltaPos = currentPosition - lastPosition;
    
    // RPM Formula: (Change in Pulses / Pulses Per Revolution) * (60000ms / Time Elapsed)
    float actualRPM = ((abs(deltaPos) / PPR) * (60000.0 / deltaTime))/3;
    
    // Calculate Desired RPM based on the PWM signal percentage
    float desiredRPM = (float(pwmSpeed) / 255.0) * MAX_MOTOR_RPM;

    // 3. Print to Serial Monitor/Plotter
    Serial.print(desiredRPM); 
    Serial.print(",");
    Serial.println(actualRPM); 

    // Update trackers for the next 100ms window
    lastPosition = currentPosition; 
    lastTime = currentTime;
  }
}