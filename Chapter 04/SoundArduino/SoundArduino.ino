#include <ArduinoJson.h>
#include <SoftwareSerial.h>

const int soundPin = A2;
const int threshold = 200;       // Setting a threshold value for sound
SoftwareSerial espSerial(0, 1);   // Rx:0, Tx:1
StaticJsonDocument<200> doc;     // JSON message for sound control

void setup() {
  espSerial.begin(9600);          // Starting serial communication at 9600 baudrate
  pinMode(soundPin, INPUT);      // Output from sensor will be received at pin A2
}

void loop() {
  int soundSense = analogRead(soundPin);  	    // Read analog data from the sound sensor
  outputSoundStatus(soundSense >= threshold);  // Send sound detection status as JSON
  delay(1000);
}

void outputSoundStatus(bool isSoundDetected) {
  String outputString = "";
  doc.clear();
  doc["ParamName"] = "Sound";
  doc["ParamValue"] = isSoundDetected;
  serializeJson(doc, outputString);
  espSerial.println(outputString);  		// Write to serial for NodeMCU to read
doc.clear();
}
