#include <ArduinoJson.h>
#include <SoftwareSerial.h>

int mq3 = A0;             // Connect Gas sensor to analog pin A0
int buzzer = 7;           // Connect buzzer to digital pin 7
int red_led = 8;          // Connect red led to digital pin 8 
int green_led = 9;        // Connect green led to digital pin 9
int threshold = 400;      // Change the threshold value for your use
String outputString = "";
SoftwareSerial espSerial(0, 1);
StaticJsonDocument<200> doc;  // JSON message for switch control

void setup() {
  pinMode(red_led, OUTPUT);      // Define red_led as output
  pinMode(green_led, OUTPUT);    // Define green_led as output
  pinMode(buzzer, OUTPUT);       // Define buzzer as output
  digitalWrite(red_led, LOW);    // Turn red_led off
  digitalWrite(green_led, LOW);  // Turn green_led off
  espSerial.begin(9600);
}

void loop() {
  int mq3Value = analogRead(mq3); // Read sensor value
  if (mq3Value >= threshold) {    // Check if it exceeds the threshold value
    digitalWrite(red_led, HIGH);  // Turn red_led on
    digitalWrite(green_led, LOW); // Turn green_led off
    tone(buzzer, 200, 500);        // Play buzzer

    outputString = "";
    doc["ParamName"] = "Alcohol";
    doc["ParamValue"] = true;
    serializeJson(doc, outputString);
    espSerial.println(outputString); // Send the JSON message
	 doc.clear();
    delay(500);
  } else {
    digitalWrite(red_led, LOW);    // Turn red_led off
    digitalWrite(green_led, HIGH); // Turn green_led on
  }
  delay(100);                      // Slow down the output
}
