#include <ArduinoJson.h>
#include <SoftwareSerial.h>

//constants
const int BOARD_RESOLUTION = 1023 ; // The analog board resolution


const int lightSensorAPin = A0;
const int lightSensorDPin = 7;
float lastLightValue;
StaticJsonDocument<200> doc;
SoftwareSerial espSerial(0, 1);    // Rx:0, Tx:1


void setup() {
  espSerial.begin(9600);          // Initialize serial communication at 9600 baud
  Serial.begin(9600);
  pinMode(lightSensorAPin,INPUT);
  pinMode(lightSensorDPin, INPUT);
}

void loop() {
  float lightValue = BOARD_RESOLUTION - analogRead(lightSensorAPin);  // Read the analog value from the sensor
  bool isLight = digitalRead(lightSensorDPin);

  if(lastLightValue != lightValue)
  {
    String outputString = "";
    doc["ParamName"] = "Light_Intensity";
    doc["ParamValue"] = lightValue;
    serializeJson(doc, outputString);
    espSerial.println(outputString);           // Write the value to NodeMCU
    doc.clear();
  }


  lastLightValue = lightValue;
  delay(1000);
}
