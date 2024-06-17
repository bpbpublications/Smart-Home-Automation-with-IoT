/*
  Web switch for 8 Channel Relay
  Uses MQTT messages for switch on/off
  Also uses webserver on ESP32 for direct switch control
*/
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <WebServer.h>
#include <ESPmDNS.h>

//MQTT Connection
const char* mqtt_server = "<serverhostname>";
const int mqtt_port = 1883;
const char* mqtt_topic = "<mqtt_topic>";

const char* nodeID = "<nodeID>";

//JSON message for switch control
String controlState = "";
StaticJsonDocument<200> doc;

int i = 0;
int statusCode;
const char* otapassword = "<OTA password>";
String st;
String content;

//PIN Switch Mapping
const int Switch1 = 23;  //D23
const int Switch2 = 22;  //D22
const int Switch3 = 21;  //D21
const int Switch4 = 19;  //D19
const int Switch5 = 18;  //D18
const int Switch6 = 5;   //D5
const int Switch7 = 25;  //D25
const int Switch8 = 26;  //D26

//PIN Button Mapping
const int Button1 = 13;  //D13
const int Button2 = 12;  //D12
const int Button3 = 14;  //D14
const int Button4 = 27;  //D27
const int Button5 = 33;  //D33
const int Button6 = 32;  //D32
const int Button7 = 15;  //D15
const int Button8 = 4;   //D4

//Button State
int Button1State = 1;
int Button2State = 1;
int Button3State = 1;
int Button4State = 1;
int Button5State = 1;
int Button6State = 1;
int Button7State = 1;
int Button8State = 1;

String switch1name = String(nodeID) + ".Switch1";
String switch2name = String(nodeID) + ".Switch2";
String switch3name = String(nodeID) + ".Switch3";
String switch4name = String(nodeID) + ".Switch4";
String switch5name = String(nodeID) + ".Switch5";
String switch6name = String(nodeID) + ".Switch6";
String switch7name = String(nodeID) + ".Switch7";
String switch8name = String(nodeID) + ".Switch8";

//Wifiserver instance at port 80
WiFiServer wifiServer(80);

WebServer server(80);

//MQTT client at port 1883
WiFiClient wifimqClient;
PubSubClient mqttclient(wifimqClient);


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
    if (!MDNS.begin(nodeID)) {
      Serial.println("Error setting up MDNS responder!");
      while (1) {
        delay(1000);
      }
    }
    Serial.println("mDNS responder started");
  
    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);
    wifiServer.begin();
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
        ESP.restart();
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

//function to enable OTA for code update
void enableOTA()
{
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

//callback function for MQTT message
void MQTTcallback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Message received in topic: ");
  Serial.println(topic);
  Serial.println("Message:");
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

  // Fetch values.
  // Most of the time, you can rely on the implicit casts.
  // In other case, you can do doc["time"].as<long>();
  String switchID = doc["SwitchName"];
  int switchState = doc["SwitchState"];

  //Check switch to toggle
  if (switchID.equals(switch1name) && (digitalRead(Switch1) == switchState))
  {
    digitalWrite(Switch1, !switchState);
  }
  if (switchID.equals(switch2name) && (digitalRead(Switch2) == switchState))
  {
    digitalWrite(Switch2, !switchState);
  }
  if (switchID.equals(switch3name) && (digitalRead(Switch3) == switchState))
  {
    digitalWrite(Switch3, !switchState);
  }
  if (switchID.equals(switch4name) && (digitalRead(Switch4) == switchState))
  {
    digitalWrite(Switch4, !switchState);
  }
  if (switchID.equals(switch5name) && (digitalRead(Switch5) == switchState))
  {
    digitalWrite(Switch5, !switchState);
  }
  if (switchID.equals(switch6name) && (digitalRead(Switch6) == switchState))
  {
    digitalWrite(Switch6, !switchState);
  }
  if (switchID.equals(switch7name) && (digitalRead(Switch7) == switchState))
  {
    digitalWrite(Switch7, !switchState);
  }
  if (switchID.equals(switch8name) && (digitalRead(Switch8) == switchState))
  {
    digitalWrite(Switch8, !switchState);
  }
}

//connect to MQTT server
int connectMQTT()
{
  Serial.print("Connecting to MQTT Broker ");
  Serial.print(mqtt_server);
  Serial.println(" at port " + mqtt_port);

  Serial.print("Resolving Host....");
  IPAddress serverIp = MDNS.queryHost(mqtt_server, 30000);
  Serial.print("IP : ");

  String mqtt_serverIP = serverIp.toString();
  Serial.println(mqtt_serverIP);

  //connect to MQTT server 
  mqttclient.setClient(wifimqClient);
  mqttclient.setServer(mqtt_serverIP.c_str(), mqtt_port);
  mqttclient.setCallback(MQTTcallback);

  if (mqttclient.connect(nodeID))
  {
    Serial.println("Connected to MQTT Broker");
    mqttclient.subscribe(mqtt_topic);

    Serial.print("Subscribed to topic ");
    Serial.println(mqtt_topic);
  }
  else
  {
    Serial.print("Failed to connect to MQTT broker with state ");
    Serial.println(mqttclient.state());
    delay(2000);
  }
  return mqttclient.state();
}

//publish switch state message to MQTT server
void publishMQTTMessage(String switchName, int switchState)
{
  doc.clear();
  controlState = "";
  doc["SwitchName"] = switchName;
  doc["SwitchState"] = switchState;
  serializeJson(doc, controlState);
  Serial.println(controlState);

  mqttclient.publish(mqtt_topic, controlState.c_str());
  doc.clear();
}

//toggle by manual switch
void toggleSwitch(int switchID, String switchName)
{
  int switchState = digitalRead(switchID);
  digitalWrite(switchID, !switchState);

  publishMQTTMessage(switchName, switchState);
}


//initialization function
void setup()
{
  pinMode(Switch1, OUTPUT);
  pinMode(Switch2, OUTPUT);
  pinMode(Switch3, OUTPUT);
  pinMode(Switch4, OUTPUT);
  pinMode(Switch5, OUTPUT);
  pinMode(Switch6, OUTPUT);
  pinMode(Switch7, OUTPUT);
  pinMode(Switch8, OUTPUT);

  pinMode(Button1, INPUT_PULLUP);
  pinMode(Button2, INPUT_PULLUP);
  pinMode(Button3, INPUT_PULLUP);
  pinMode(Button4, INPUT_PULLUP);
  pinMode(Button5, INPUT_PULLUP);
  pinMode(Button6, INPUT_PULLUP);
  pinMode(Button7, INPUT_PULLUP);
  pinMode(Button8, INPUT_PULLUP);

  digitalWrite(Switch1, HIGH);
  digitalWrite(Switch2, HIGH);
  digitalWrite(Switch3, HIGH);
  digitalWrite(Switch4, HIGH);
  digitalWrite(Switch5, HIGH);
  digitalWrite(Switch6, HIGH);
  digitalWrite(Switch7, HIGH);
  digitalWrite(Switch8, HIGH);

  Serial.begin(115200);
  delay(10);

  EEPROM.begin(100);

  //connect to wifi
  connectWifi();

  //enable OTA
  enableOTA();

  //connect to MQTT broker
  connectMQTT();
}

//continuous loop function
void loop() 
{
  if ((WiFi.status() != WL_CONNECTED))
  {
    connectWifi();
  }

  //check MQTT Connection
  if (mqttclient.state() != 0)
  {
    connectMQTT();
  }

  mqttclient.loop();
  ArduinoOTA.handle();

  if ((digitalRead(Button1) != Button1State))
  {
    toggleSwitch(Switch1, switch1name);
    Button1State = digitalRead(Button1);    
  }
  if (digitalRead(Button2) != Button2State)
  {
    toggleSwitch(Switch2, switch2name);
    Button2State = digitalRead(Button2);
  }
  if (digitalRead(Button3) != Button3State)
  {
    toggleSwitch(Switch3, switch3name);
    Button3State = digitalRead(Button3); 
  }
  if (digitalRead(Button4) != Button4State)
  {
    toggleSwitch(Switch4, switch4name);
    Button4State = digitalRead(Button4); 
  }
  if (digitalRead(Button5) != Button5State)
  {
    toggleSwitch(Switch5, switch5name);
    Button5State = digitalRead(Button5);
  }
  if (digitalRead(Button6) != Button6State)
  {
    toggleSwitch(Switch6, switch6name);
    Button6State = digitalRead(Button6);
  }
  if (digitalRead(Button7) != Button7State)
  {
    toggleSwitch(Switch7, switch7name);
    Button7State = digitalRead(Button7);
  }
  if (digitalRead(Button8) != Button8State)
  {
    toggleSwitch(Switch8, switch8name);
    Button8State = digitalRead(Button8);
  }

  WiFiClient client = wifiServer.available();   // listen for incoming clients
  if (!client.available()) {
    delay(100);
    if (!client) {
      return;
    }
  }

  String req = client.readStringUntil('\r');
  Serial.println("New Client.");           // print a message out the serial port

  // Check to see the client request":
  if (req.indexOf("/switch1on") != -1) {
    digitalWrite(Switch1, LOW);               // GET /H turns the LED on
    publishMQTTMessage(switch1name, 1);
  }
  if (req.indexOf("/switch1off") != -1) {
    digitalWrite(Switch1, HIGH);                // GET /L turns the LED off
    publishMQTTMessage(switch1name, 0);
  }

  // Check to see the client request":
  if (req.indexOf("/switch2on") != -1) {
    digitalWrite(Switch2, LOW);               // GET /H turns the LED on
    publishMQTTMessage(switch2name, 1);
  }
  if (req.indexOf("/switch2off") != -1) {
    digitalWrite(Switch2, HIGH);                // GET /L turns the LED off
    publishMQTTMessage(switch2name, 0);
  }

  // Check to see the client request":
  if (req.indexOf("/switch3on") != -1) {
    digitalWrite(Switch3, LOW);               // GET /H turns the LED on
    publishMQTTMessage(switch3name, 1);
  }
  if (req.indexOf("/switch3off") != -1) {
    digitalWrite(Switch3, HIGH);                // GET /L turns the LED off
    publishMQTTMessage(switch3name, 0);
  }
  // Check to see the client request":
  if (req.indexOf("/switch4on") != -1) {
    digitalWrite(Switch4, LOW);               // GET /H turns the LED on
    publishMQTTMessage(switch4name, 1);
  }
  if (req.indexOf("/switch4off") != -1) {
    digitalWrite(Switch4, HIGH);                // GET /L turns the LED off
    publishMQTTMessage(switch4name, 0);
  }
  // Check to see the client request":
  if (req.indexOf("/switch5on") != -1) {
    digitalWrite(Switch5, LOW);               // GET /H turns the LED on
    publishMQTTMessage(switch5name, 1);
  }
  if (req.indexOf("/switch5off") != -1) {
    digitalWrite(Switch5, HIGH);                // GET /L turns the LED off
    publishMQTTMessage(switch5name, 0);
  }
  // Check to see the client request":
  if (req.indexOf("/switch6on") != -1) {
    digitalWrite(Switch6, LOW);               // GET /H turns the LED on
    publishMQTTMessage(switch6name, 1);
  }
  if (req.indexOf("/switch6off") != -1) {
    digitalWrite(Switch6, HIGH);                // GET /L turns the LED off
    publishMQTTMessage(switch6name, 0);
  }
  // Check to see the client request":
  if (req.indexOf("/switch7on") != -1) {
    digitalWrite(Switch7, LOW);               // GET /H turns the LED on
    publishMQTTMessage(switch7name, 1);
  }
  if (req.indexOf("/switch7off") != -1) {
    digitalWrite(Switch7, HIGH);                // GET /L turns the LED off
    publishMQTTMessage(switch7name, 0);
  }
  // Check to see the client request":
  if (req.indexOf("/switch8on") != -1) {
    digitalWrite(Switch8, LOW);               // GET /H turns the LED on
    publishMQTTMessage(switch8name, 1);
  }
  if (req.indexOf("/switch8off") != -1) {
    digitalWrite(Switch8, HIGH);                // GET /L turns the LED off
    publishMQTTMessage(switch8name, 0);
  }

  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  // Display the Page
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<head>");
  client.println("<meta name='apple-mobile-web-app-capable' content='yes' />");
  client.println("<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />");
  client.println("</head>");
  client.println("<body bgcolor = \"#f7e6ec\">");
  client.println("<style>");
  client.println(".redbutton {");
  client.println("background-color: #ff0000;");
  client.println("border: 2px solid black;");
  client.println("color: white;");
  client.println("padding: 15px 32px;");
  client.println("text-align: center;");
  client.println("text-decoration: none;");
  client.println("display: inline-block;");
  client.println("font-size: 16px;");
  client.println("margin: 4px 2px;");
  client.println("cursor: pointer;");
  client.println("}");

  client.println(".greenbutton {");
  client.println("background-color: #00ff75;");
  client.println("border: 2px solid black;");
  client.println("color: black;");
  client.println("padding: 15px 32px;");
  client.println("text-align: center;");
  client.println("text-decoration: none;");
  client.println("display: inline-block;");
  client.println("font-size: 16px;");
  client.println("margin: 4px 2px;");
  client.println("cursor: pointer;");
  client.println("}");

  client.println("</style>");
  client.println("<hr/><hr>");
  client.print("<h4><center> Smart Switch Control ");
  client.print(nodeID);
  client.println("</center></h4>");
  client.println("<hr/><hr>");

  client.println("<center>");
  client.println("<table border=\"1\" width=\"100%\">");
  client.println("<tr>");


  if (!digitalRead(Switch1))
  {
    client.println("<td><b>Switch 1</B><BR><a href=\"/switch1off\"\"><button class=\"redbutton\">Turn Off</button></a><br/>Switch 1 is ON<BR><BR></td>");
  }
  else
  {
    client.println("<td><b>Switch 1</B><BR><a href=\"/switch1on\"\"><button class=\"greenbutton\">Turn On</button></a><br/>Switch 1 is OFF<BR><BR></td>");
  }

  if (!digitalRead(Switch2))
  {
    client.println("<td><b>Switch 2</B><BR><a href=\"/switch2off\"\"><button class=\"redbutton\">Turn Off</button></a><br/>Switch 2 is ON<BR><BR></td>");
  }
  else
  {
    client.println("<td><b>Switch 2</B><BR><a href=\"/switch2on\"\"><button class=\"greenbutton\">Turn On</button></a><br/>Switch 2 is OFF<BR><BR></td>");
  }

  if (!digitalRead(Switch3))
  {
    client.println("<td><b>Switch 3</B><BR><a href=\"/switch3off\"\"><button class=\"redbutton\">Turn Off</button></a><br/>Switch 3 is ON<BR><BR></td>");
  }
  else
  {
    client.println("<td><b>Switch 3</B><BR><a href=\"/switch3on\"\"><button class=\"greenbutton\">Turn On</button></a><br/>Switch 3 is OFF<BR><BR></td>");
  }

  if (!digitalRead(Switch4))
  {
    client.println("<td><b>Switch 4</B><BR><a href=\"/switch4off\"\"><button class=\"redbutton\">Turn Off</button></a><br/>Switch 4 is ON<BR><BR></td>");
  }
  else
  {
    client.println("<td><b>Switch 4</B><BR><a href=\"/switch4on\"\"><button class=\"greenbutton\">Turn On</button></a><br/>Switch 4 is OFF<BR><BR></td>");
  }

  client.println("</tr><tr>");
  if (!digitalRead(Switch5))
  {
    client.println("<td><b>Switch 5</B><BR><a href=\"/switch5off\"\"><button class=\"redbutton\">Turn Off</button></a><br/>Switch 5 is ON<BR><BR></td>");
  }
  else
  {
    client.println("<td><b>Switch 5</B><BR><a href=\"/switch5on\"\"><button class=\"greenbutton\">Turn On</button></a><br/>Switch 5 is OFF<BR><BR></td>");
  }

  if (!digitalRead(Switch6))
  {
    client.println("<td><b>Switch 6</B><BR><a href=\"/switch6off\"\"><button class=\"redbutton\">Turn Off</button></a><br/>Switch 6 is ON<BR><BR></td>");
  }
  else
  {
    client.println("<td><b>Switch 6</B><BR><a href=\"/switch6on\"\"><button class=\"greenbutton\">Turn On</button></a><br/>Switch 6 is OFF<BR><BR></td>");
  }

  if (!digitalRead(Switch7))
  {
    client.println("<td><b>Switch 7</B><BR><a href=\"/switch7off\"\"><button class=\"redbutton\">Turn Off</button></a><br/>Switch 7 is ON<BR><BR></td>");
  }
  else
  {
    client.println("<td><b>Switch 7</B><BR><a href=\"/switch7on\"\"><button class=\"greenbutton\">Turn On</button></a><br/>Switch 7 is OFF<BR><BR></td>");
  }

  if (!digitalRead(Switch8))
  {
    client.println("<td><b>Switch 8</B><BR><a href=\"/switch8off\"\"><button class=\"redbutton\">Turn Off</button></a><br/>Switch 8 is ON<BR><BR></td>");
  }
  else
  {
    client.println("<td><b>Switch 8</B><BR><a href=\"/switch8on\"\"><button class=\"greenbutton\">Turn On</button></a><br/>Switch 8 is OFF<BR><BR></td>");
  }

  client.println("</tr>");
  client.println("</table>");
  client.println("<BR>");
  client.println("</center>");
  client.println("</html>");

  // close the connection:
  Serial.println("Client Disconnected.");
}
