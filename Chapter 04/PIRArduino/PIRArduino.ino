#include <ArduinoJson.h>
#include <SoftwareSerial.h>

int ledPin = 13;               // choose the inbuilt pin for the LED
int inputPin = 2;              // choose the input pin (for PIR sensor)
int pirState = LOW;            // current motion detection state
int val = 0;                   // variable for reading the pin status
String outputString = "";
StaticJsonDocument<200> doc;   // JSON message for motion control
SoftwareSerial espSerial(0, 1); // Rx:0, Tx:1

void setup() {
  pinMode(ledPin, OUTPUT);     // declare LED as output
  pinMode(inputPin, INPUT);    // declare sensor as input
  espSerial.begin(9600);
}

void loop() {
  val = digitalRead(inputPin);          // read input value from the PIR sensor
  if (val == HIGH) {                   // check if the input is HIGH(motion detected)
    digitalWrite(ledPin, HIGH);         // turn LED ON
    if (pirState == LOW) {              // Motion has just been detected
      outputString = "";
      doc["ParamName"] = "Motion";
      doc["ParamValue"] = true;
      serializeJson(doc, outputString);
      espSerial.println(outputString);   // Send the JSON message
      pirState = HIGH;                  // Update the state to HIGH
      doc.clear();
    }
  } else {
    digitalWrite(ledPin, LOW);          // turn LED OFF
    if (pirState == HIGH) {             // Motion has just ended
      outputString = "";
      doc["ParamName"] = "Motion";
      doc["ParamValue"] = false;
      serializeJson(doc, outputString);
      espSerial.println(outputString);   // Send the JSON message
      pirState = LOW;                   // Update the state to LOW
      doc.clear();
    }
  }
}
