/*
Black Garlic Temperature Controller
Part V: Set point tracking with serial I/O
Communicates with slow-cooker-calibration.ipynb
*/
#include <math.h>

#define SERIES_RESISTOR 7000     // 7k resistor in series
#define THERMISTOR_PIN A0        // Thermistor between A0 and GND
#define RELAY_PIN 2              // output pin for relay
#define P_MAX 35                 // Heater wattage

// Values for B-Value temperature conversion
#define T0 298.15          // Nominal temperature for B-value equation (in Kelvin)
#define R0 10000                  // Nominal thermistor resistance
#define B 3428.                  // From 25-80 C range for Murata NCP15XH103F03RC

// PI controller gains, calculated from slow-cooker-calibration.ipynb
#define Kp 5.8
#define Ki 1.5e-3

int delayTime;                    // Time between readings in ms. Also duty cycle width
float dutyCycle = 0;              // percentage of delayTime spent with relay ON (calculated from ctrl law)
float T;                          // Temperature in Celsius, calcuated from the voltage across the thermistor
int setPoint;                     // Target temperature for controller (input from Serial)
float err_int = 0;                // Integrated error
boolean woundUp = false;              // "Warmup" stage of operation

void setup() {
  Serial.begin(9600);                      // Set up the serial for debugging output
  pinMode(RELAY_PIN, OUTPUT);
  Serial.println("Ready");                 // Message for Python script
  while(Serial.available() == 0){}         // Wait for response from serial
  delayTime = Serial.parseInt()*1000;      // Read delay time in seconds  
}

void loop() {
  
  // Go through a duty cycle of the relay: ON for dutyCycle % of dt, then off for the rest of dt
  digitalWrite(RELAY_PIN, HIGH);
  delay(delayTime*dutyCycle);
  digitalWrite(RELAY_PIN, LOW);
  delay(delayTime*(1-dutyCycle) );
    
  T = tempConvert(analogRead(THERMISTOR_PIN));
  Serial.println(T);
  if ((!woundUp) && (setPoint-T < P_MAX/Kp)){
    woundUp = true;              // System has warmed up and it's time to start integrating error
  }
  piUpdate();
}

void serialEvent(){
  setPoint = Serial.parseInt();     // Read instructions from Serial
  while(Serial.available()){
    Serial.read();
  }
}

// Update the duty cycle based on the PI control law
void piUpdate(){
   float err = setPoint-T;              // temperature error in degrees C
   if (woundUp){                          // Don't accumulate until the system winds up
     err_int += err*delayTime/1000;     // Accumulate integrated error (units are seconds)
   }
   dutyCycle = (Kp*err + Ki*err_int)/P_MAX;   // PI control law
   dutyCycle = min(1, max(0, dutyCycle));    // Restrict duty cycle to 0-100%
}

float tempConvert(int count){
  float R = SERIES_RESISTOR  / ((float) 1023/count - 1); // Convert to resistance across thermistor
  return 1 / ( (1/T0) + (1/B)*log(R/R0) ) - 273.15;  // B-value equation
}
