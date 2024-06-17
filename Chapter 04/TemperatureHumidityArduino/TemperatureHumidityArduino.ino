#include "DHT.h"
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

#define DHT11_PIN 7
#define DHTTYPE DHT11 

SoftwareSerial espSerial(0,1);

double temperature = 0.0;
double humidity = 0.0;
String outputString = "";
StaticJsonDocument<200> doc;        
double oldtemperature = 0.0;
double oldhumidity = 0.0;

DHT dht(DHT11_PIN, DHTTYPE);

void setup() {
  espSerial.begin(9600);
  dht.begin();
}

void loop() {
  oldtemperature = temperature;
  oldhumidity = humidity;

  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  if ((oldtemperature != temperature)  && !isnan(temperature)){
    outputString = "";
    doc["ParamName"] = "Temperature";
    doc["ParamValue"] = temperature;
    serializeJson(doc, outputString);
    espSerial.write(outputString.c_str());         //write to serial for NodeMCU to read
    doc.clear();
    delay(1000);
  }

  if ((oldhumidity != humidity) && !isnan(humidity)){
    outputString = "";
    doc["ParamName"] = "Humidity";
    doc["ParamValue"] = humidity;
    serializeJson(doc, outputString);
    espSerial.write(outputString.c_str());       //write to serial for NodeMCU to read
    doc.clear();
    delay(1000);
  }
}
