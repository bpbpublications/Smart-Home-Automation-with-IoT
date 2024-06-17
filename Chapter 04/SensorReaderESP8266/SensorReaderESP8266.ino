// Include the necessary libraries
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>


//MQTT Connection
const char *mqtt_server = "<mqtt serverhostname>";
const int mqtt_port = 1883;
const char *mqtt_topic = "<mqtt topic>";
const char *nodeID = "<hostname of NodeMCU>";


//Wifiserver instance at port 80
ESP8266WebServer server(80);

//MQTT client at port 1883
WiFiClient wifimqClient;
PubSubClient mqttclient;

const char* otapassword = "<ota password>";

String content;                       // creating required variables
String st;
int statusCode = 0;
String outputString = "";



//connect to Wifi network
void connectWifi()
{
  Serial.println("Startup");
  //---------------------------------------- Read EEPROM for SSID and pass
  Serial.println("Reading EEPROM ssid");

  String esid;
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.println();
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");

  String epass = "";
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);


  WiFi.begin(esid.c_str(), epass.c_str());
  if (testWifi())
  {
    Serial.println("Successfully Connected!!!");
    // Start the server
    Serial.println("");
    Serial.println("WiFi connected");

    Serial.print(WiFi.localIP());

    // Set up mDNS responder:
    // - first argument is the domain name, in this example
    //   the fully-qualified domain name is "<hostname>.local"
    // - second argument is the IP address to advertise
    //   we send our IP address on the WiFi network
    if (!MDNS.begin(nodeID, WiFi.localIP())) {
      Serial.println("Error setting up MDNS responder!");
      while (1) {
        delay(1000);
      }
    }
    Serial.println("mDNS responder started");

    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);

    return;
  }
  else
  {
    Serial.println("Turning the HotSpot On");
    launchWeb();
    setupAP();// Setup HotSpot
  }

  Serial.println();
  Serial.println("Waiting.");

  while ((WiFi.status() != WL_CONNECTED))
  {
    Serial.print(".");
    delay(200);
    server.handleClient();
  }

}

//Fuctions used for WiFi credentials saving and connecting to it
bool testWifi()
{
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 100 ) {
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    delay(200);
    Serial.print("*");
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}

void launchWeb()
{
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED)
    Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer();
  // Start the server
  server.begin();
  Serial.println("Server started");
}


//setup hotspot
void setupAP()
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);

    st += ")";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.softAP(nodeID, "");
  Serial.println("softap");
  launchWeb();
  Serial.println("over");
}


//launch web page for Wifi config
void createWebServer()
{
  {
    server.on("/", []() {

      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
      content += "<form action=\"/scan\" method=\"POST\"><input type=\"submit\" value=\"scan\"></form>";
      content += ipStr;
      content += "<p>";
      content += st;
      content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
      content += "</html>";
      server.send(200, "text/html", content);
    });
    server.on("/scan", []() {

      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);

      content = "<!DOCTYPE HTML>\r\n<html>go back";
      server.send(200, "text/html", content);
    });

    server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      if (qsid.length() > 0 && qpass.length() > 0) {
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");

        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(i, qsid[i]);
          Serial.print("Wrote: ");
          Serial.println(qsid[i]);
        }
        Serial.println("writing eeprom pass:");
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(32 + i, qpass[i]);
          Serial.print("Wrote: ");
          Serial.println(qpass[i]);
        }
        EEPROM.commit();

        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200;
        ESP.reset();
      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        Serial.println("Sending 404");
      }
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);

    });
  }
}
//WiFi Config Functions end


void enableOTA()
{
  // Port defaults to 8266
  //ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(nodeID);

  // No authentication by default
  ArduinoOTA.setPassword(otapassword);

  // Password can be set with it's md5 value as well
  //ArduinoOTA.setPasswordHash("b0dc539ad687b8ed4da9e44629b778ea");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();          //OTA initialization
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());     // Display the IP address of the ESP on the serial monitor
}

//connect to MQTT
int connectMQTT()
{
  Serial.print("Connecting to MQTT Broker ");
  Serial.print("Connecting to MQTT Broker ");
  Serial.print(mqtt_server);
  Serial.println(" at port " + mqtt_port);

  //connect to MQTT server
  mqttclient.setClient(wifimqClient);
  mqttclient.setServer(mqtt_server, mqtt_port);

  if (mqttclient.connect(nodeID))
  {
    Serial.println("Connected to MQTT Broker");
    boolean subs = mqttclient.subscribe(mqtt_topic);
    Serial.print("Subscribed to topic ");
    Serial.println(mqtt_topic);
    String msg = String("Connected. State : " + String(mqttclient.state()) + " Topic Subscribed :" + String(subs) + "  Topic : " + String(mqtt_topic));
  }
  else
  {
    Serial.print("Failed to connect to MQTT broker with state ");
    Serial.println(mqttclient.state());
    String msg = String("Not Connected : " + mqttclient.state());
    delay(2000);
  }
  return mqttclient.state();
}



//publish message to MQTT broker
void publishMQTTMessage(String paramName, float paramValue)
{
  //create JSCON message for MQTT publish
  StaticJsonDocument<200> doc;
  doc.clear();
  String controlState = "";
  doc["ParamName"] = paramName;
  doc["ParamValue"] = paramValue;
  serializeJson(doc, controlState);
  Serial.println(controlState);

  //publish MQTT message
  mqttclient.publish(mqtt_topic, controlState.c_str());
  doc.clear();
}


void setup() {
  Serial.begin(9600);       //Initialising the Serial Monitor
  Serial.println();
  Serial.println("Disconnecting previously connected WiFi");
  WiFi.disconnect();
  EEPROM.begin(512);        //Initialasing EEPROM
  delay(10);
  connectWifi();            //connect to Wifi
}

void loop() {

  //check the connections and connect if disconnected
  if ((WiFi.status() != WL_CONNECTED))
  {
    Serial.println("wifi Disconnected");
    connectWifi();
  }

  MDNS.update();
  //check MQTT Connection
  if (mqttclient.state() != 0)
  {
    Serial.println("MQTT Disconnected");
    connectMQTT();
  }
  mqttclient.loop();
  ArduinoOTA.handle();

  StaticJsonDocument<200> doc;
  String outputString = Serial.readStringUntil('\n');
  DeserializationError error = deserializeJson(doc, outputString);
  if (error) {
    Serial.print("Error in parsing JSON: ");
    Serial.println(error.c_str());    // If there is any error it will print that
    return;
  }
    // Fetch values
  String paramName = doc["ParamName"];
  float paramValue = doc["ParamValue"];
  publishMQTTMessage(paramName, paramValue);
}
