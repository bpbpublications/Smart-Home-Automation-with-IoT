#include <ArduinoJson.h>
#include <SoftwareSerial.h>

SoftwareSerial espSerial(0, 1);  // Rx:0, Tx:1
int trigPin = 9;                // TRIG pin
int echoPin = 8;                // ECHO pin
float duration_us, distance_cm;
StaticJsonDocument<200> doc;    // JSON message for distance data

void setup() {
  espSerial.begin(9600);        // Set the baudrate as per requirement
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

void loop() {
  digitalWrite(trigPin, LOW);   // Clear the trigger pin
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);  // Generate a 10-microsecond pulse to TRIG pin
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration_us = pulseIn(echoPin, HIGH);  // Measure duration of pulse from ECHO pin
  distance_cm = ((duration_us/2)/29.1);     // Calculate the distance in centimeters

  doc["ParamName"] = "Distance";
  doc["ParamValue"] = distance_cm;
  String outputString = "";
  serializeJson(doc, outputString);
  espSerial.println(outputString);  // Write to serial for NodeMCU to read
  doc.clear();
  delay(1000);
}
