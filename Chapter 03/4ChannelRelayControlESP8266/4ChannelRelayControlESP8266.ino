#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

//MQTT Connection
const char *mqtt_server = "<serverhostname>";
const int mqtt_port = 1883;
const char *mqtt_topic = "<mqtt_topic>";
const char *nodeID = "<nodeID>";

//Variables
int i = 0;
int statusCode;
const char* otapassword = "<ota_password>";
String st;
String content;

//JSON message for switch control
String controlState = "";
StaticJsonDocument<200> doc;

//Wifiserver instance at port 80
WiFiServer wifiServer(80);

//Establishing Local server at port 80 whenever required
ESP8266WebServer server(80);

//MQTT client at port 1883
WiFiClient wifimqClient;
PubSubClient mqttclient;


//PIN Relay Mapping
const int Switch1 = 4;
const int Switch2 = 0;
const int Switch3 = 2;
const int Switch4 = 14;

//PIN Switch Mapping
const int Button1 = 10;
const int Button2 = 9;
const int Button3 = 13;
const int Button4 = 3;

//Switch State
int Button1State = 1;
int Button2State = 1;
int Button3State = 1;
int Button4State = 1;

//Switch name
String switch1name = String(nodeID) + ".Switch1";
String switch2name = String(nodeID) + ".Switch2";
String switch3name = String(nodeID) + ".Switch3";
String switch4name = String(nodeID) + ".Switch4";

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
  mqttclient.setCallback(MQTTcallback);

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

//callback function for MQTT message
void MQTTcallback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Message received in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  String message;
  for (int i = 0; i < length; i++)
  {
    message = message + (char)payload[i];
  }
  Serial.flush();
  Serial.println(message);

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, message);

  // Test if parsing succeeds.
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }

  // Fetch values
  String switchName = doc["SwitchName"];
  int switchState = doc["SwitchState"];

  //Check switch to toggle
  if (switchName.equals(switch1name) && (digitalRead(Switch1) == switchState))
  {
    controlSwitch(Switch1, !switchState);
  }
  if (switchName.equals(switch2name) && (digitalRead(Switch2) == switchState))
  {
    controlSwitch(Switch2, !switchState);
  }
  if (switchName.equals(switch3name) && (digitalRead(Switch3) == switchState))
  {
    controlSwitch(Switch3, !switchState);
  }
  if (switchName.equals(switch4name) && (digitalRead(Switch4) == switchState))
  {
    controlSwitch(Switch4, !switchState);
  }
}


//read switch state to toggle
void toggleSwitch(int switchID)
{
  int switchState = digitalRead(switchID);
  controlSwitch(switchID, !switchState);
}


//switch on or off by changing digitalState of relay pin
void controlSwitch(int switchID, int switchState)
{
  //change digitalState of relay pin as per instruction
  digitalWrite(switchID, switchState);
}


//publish message to MQTT broker
void publishMQTTMessage(String switchName, int switchState)
{
  //create JSCON message for MQTT publish
  doc.clear();
  controlState = "";
  doc["SwitchName"] = switchName;
  doc["SwitchState"] = switchState;
  serializeJson(doc, controlState);
  Serial.println(controlState);

  //publish MQTT message
  mqttclient.publish(mqtt_topic, controlState.c_str());
  doc.clear();
}


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

    // Start the server
    wifiServer.begin();
    Serial.println("Server started");

    // Print the IP address
    Serial.print("Use this URL to connect: ");
    Serial.print("http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");

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


//setup/initialization function will be executed once when powered on
void setup() {
  Serial.begin(115200); //Initialising if(DEBUG)Serial Monitor
  Serial.println();
  Serial.println("Disconnecting previously connected WiFi");
  WiFi.disconnect();
  EEPROM.begin(512); //Initialasing EEPROM
  delay(10);
  
  pinMode(Switch1, OUTPUT);
  pinMode(Switch2, OUTPUT);
  pinMode(Switch3, OUTPUT);
  pinMode(Switch4, OUTPUT);

  pinMode(Button1, INPUT_PULLUP);
  pinMode(Button2, INPUT_PULLUP);
  pinMode(Button3, INPUT_PULLUP);
  pinMode(Button4, INPUT_PULLUP);

  digitalWrite(Switch1, HIGH);
  digitalWrite(Switch2, HIGH);
  digitalWrite(Switch3, HIGH);
  digitalWrite(Switch4, HIGH);

  //connect to Wifi
  connectWifi();

  //enable OTA update
  enableOTA();

  //connect to MQTT broker
  connectMQTT();

}


//will be continuously executed when the board is powered on
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

  //check the digital state of the button pin (physical switch) to toggle the relay channel if needed
  if ((digitalRead(Button1) != Button1State))
  {
    toggleSwitch(Switch1);
    publishMQTTMessage(switch1name, digitalRead(Switch1));
    Button1State = digitalRead(Button1);
  }
  if (digitalRead(Button2) != Button2State)
  {
    toggleSwitch(Switch2);
    publishMQTTMessage(switch2name, digitalRead(Switch2));
    Button2State = digitalRead(Button2);
  }
  if (digitalRead(Button3) != Button3State)
  {
    toggleSwitch(Switch3);
    publishMQTTMessage(switch3name, digitalRead(Switch3));
    Button3State = digitalRead(Button3);
  }
  if (digitalRead(Button4) != Button4State)
  {
    toggleSwitch(Switch4);
    publishMQTTMessage(switch4name, digitalRead(Switch4));
    Button4State = digitalRead(Button4);
  }

  //Web request Handler
  // Check if a client has connected
  WiFiClient wifiClient = wifiServer.available();
  if (!wifiClient.available()) {
    delay(100);
    if (!wifiClient) {
      return;
    }
  }

  // Read the first line of the request
  String request = wifiClient.readStringUntil('\r');
  Serial.println(request);
  wifiClient.flush();

  //Check amd control switch state
  if (request.indexOf("/switch1on") > 0)  {
    controlSwitch(Switch1, LOW);
    publishMQTTMessage(switch1name, HIGH);
  }

  if (request.indexOf("/switch1off") > 0)  {
    controlSwitch(Switch1, HIGH);
    publishMQTTMessage(switch1name, LOW);
  }

  if (request.indexOf("/switch2on") > 0)  {
    controlSwitch(Switch2, LOW);
    publishMQTTMessage(switch2name, HIGH);
  }

  if (request.indexOf("/switch2off") > 0)  {
    controlSwitch(Switch2, HIGH);
    publishMQTTMessage(switch2name, LOW);
  }

  if (request.indexOf("/switch3on") > 0)  {
    controlSwitch(Switch3, LOW);
    publishMQTTMessage(switch3name, HIGH);
  }

  if (request.indexOf("/switch3off") > 0)  {
    controlSwitch(Switch3, HIGH);
    publishMQTTMessage(switch3name, LOW);
  }

  if (request.indexOf("/switch4on") > 0)  {
    controlSwitch(Switch4, LOW);
    publishMQTTMessage(switch4name, HIGH);
  }

  if (request.indexOf("/switch4off") > 0)  {
    controlSwitch(Switch4, HIGH);
    publishMQTTMessage(switch4name, LOW);
  }

  // Display the Page
  wifiClient.println("HTTP/1.1 200 OK");
  wifiClient.println("Content-Type: text/html");
  wifiClient.println(""); //  do not forget this one
  wifiClient.println("<!DOCTYPE HTML>");
  wifiClient.println("<html>");
  wifiClient.println("<head>");
  wifiClient.println("<meta name='apple-mobile-web-app-capable' content='yes' />");
  wifiClient.println("<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />");
  wifiClient.println("</head>");
  wifiClient.println("<body bgcolor = \"#f7e6ec\">");
  wifiClient.println("<style>");
  wifiClient.println(".redbutton {");
  wifiClient.println("background-color: #ff0000;");
  wifiClient.println("border: 2px solid black;");
  wifiClient.println("color: white;");
  wifiClient.println("padding: 15px 32px;");
  wifiClient.println("text-align: center;");
  wifiClient.println("text-decoration: none;");
  wifiClient.println("display: inline-block;");
  wifiClient.println("font-size: 16px;");
  wifiClient.println("margin: 4px 2px;");
  wifiClient.println("cursor: pointer;");
  wifiClient.println("}");
  wifiClient.println(".greenbutton {");
  wifiClient.println("background-color: #00ff75;");
  wifiClient.println("border: 2px solid black;");
  wifiClient.println("color: black;");
  wifiClient.println("padding: 15px 32px;");
  wifiClient.println("text-align: center;");
  wifiClient.println("text-decoration: none;");
  wifiClient.println("display: inline-block;");
  wifiClient.println("font-size: 16px;");
  wifiClient.println("margin: 4px 2px;");
  wifiClient.println("cursor: pointer;");
  wifiClient.println("}");
  wifiClient.println("</style>");
  wifiClient.println("<hr/><hr>");
  wifiClient.println("<h4><center> Smart Switch Control - " + String(nodeID) + "</center></h4>");
  wifiClient.println("<hr/><hr>");
  wifiClient.println("<center>");
  wifiClient.println("<table border=\"1\" width=\"100%\">");
  wifiClient.println("<tr>");

  if (!digitalRead(Switch1))
  {
    wifiClient.println("<td><b>Switch 1</B><BR><a href=\"/switch1off\"\"><button class=\"redbutton\">Turn Off</button></a><br/>Switch 1 is ON<BR><BR></td>");
  }
  else
  {
    wifiClient.println("<td><b>Switch 1</B><BR><a href=\"/switch1on\"\"><button class=\"greenbutton\">Turn On</button></a><br/>Switch 1 is OFF<BR><BR></td>");
  }
  if (!digitalRead(Switch2))
  {
    wifiClient.println("<td><b>Switch 2</B><BR><a href=\"/switch2off\"\"><button class=\"redbutton\">Turn Off</button></a><br/>Switch 2 is ON<BR><BR></td>");
  }
  else
  {
    wifiClient.println("<td><b>Switch 2</B><BR><a href=\"/switch2on\"\"><button class=\"greenbutton\">Turn On</button></a><br/>Switch 2 is OFF<BR><BR></td>");
  }
  if (!digitalRead(Switch3))
  {
    wifiClient.println("<td><b>Switch 3</B><BR><a href=\"/switch3off\"\"><button class=\"redbutton\">Turn Off</button></a><br/>Switch 3 is ON<BR><BR></td>");
  }
  else
  {
    wifiClient.println("<td><b>Switch 3</B><BR><a href=\"/switch3on\"\"><button class=\"greenbutton\">Turn On</button></a><br/>Switch 3 is OFF<BR><BR></td>");
  }

  if (!digitalRead(Switch4))
  {
    wifiClient.println("<td><b>Switch 4</B><BR><a href=\"/switch4off\"\"><button class=\"redbutton\">Turn Off</button></a><br/>Switch 4 is ON<BR><BR></td>");
  }
  else
  {
    wifiClient.println("<td><b>Switch 4</B><BR><a href=\"/switch4on\"\"><button class=\"greenbutton\">Turn On</button></a><br/>Switch 4 is OFF<BR><BR></td>");
  }

  wifiClient.println("</tr>");
  wifiClient.println("</table>");
  wifiClient.println("<BR>");
  wifiClient.println("</center>");
  wifiClient.println("</html>");
  Serial.println("Client disonnected");
  Serial.println("");
}
