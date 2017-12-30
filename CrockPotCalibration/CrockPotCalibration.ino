/*
Black Garlic Temperature Controller
Part IV: Receive signals from Python via Serial (Manual system control)
Communicates with slow-cooker-calibration.ipynb
*/
#include <math.h>

#define SERIES_RESISTOR 7000     // 7k resistor in series
#define THERMISTOR_PIN A0        // Thermistor between A0 and GND
#define RELAY_PIN 2              // output pin for relay

// Values for B-Value temperature conversion
#define T0 298.15          // Nominal temperature for B-value equation (in Kelvin)
#define R0 10000                  // Nominal thermistor resistance
#define B 3428                  // From 25-80 C range for Murata NCP15XH103F03RC

int delayTime;                    // Time between readings in ms. Also duty cycle width
float dutyCycle = 0;                // percentage of delayTime spent with relay ON (controlled by Serial)
float T;                          // Temperature in Celsius, calcuated from the voltage across the thermistor

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
    
  T = tempConvert(analogRead(THERMISTOR_PIN));               // Read the voltage across the thermesistor
  Serial.println(T);
  Serial.println(dutyCycle);
}

void serialEvent(){
  dutyCycle = (float) Serial.parseInt()/100;     // Read instructions from Serial
  while(Serial.available()){
    Serial.read();
  }
}

float tempConvert(int count){
  float R = SERIES_RESISTOR  / ((float) 1023/count - 1); // Convert to resistance across thermistor
  return 1 / ( (1/T0) + (1/B)*log(R/R0) ) - 273.15;  // B-value equation
}
