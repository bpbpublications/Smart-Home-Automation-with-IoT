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
int statusCode, Pwm_status;
const char* otapassword = "<ota_password>";
String st;
String content;

// L293D Motor Driver Connections
const int MotorInput1 = 14;  // Connected to IN1
const int MotorInput2 = 12;  // Connected to IN2
const int MotorInput3 = 13;  // Connected to IN3
const int MotorInput4 = 15;  // Connected to IN4
const int MotorEnable1 = 4;  // Connected to EN1 (Enable PWM)
const int MotorEnable2 = 5;  // Connected to EN2 (Enable PWM)

int MotorSpeed = 0;         // Motor speed (0-255)
int MotorDirection = 0;     // Motor direction

//JSON message for motor control
String controlState = "";
StaticJsonDocument<200> doc;

//Wifiserver instance at port 80
WiFiServer wifiServer(80);

//Establishing Local server at port 80 whenever required
ESP8266WebServer server(80);

//MQTT client at port 1883
WiFiClient wifimqClient;
PubSubClient mqttclient;

void enableOTA()
{
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(nodeID);

  // No authentication by default
  ArduinoOTA.setPassword(otapassword);

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
  
  // Check if the MQTT message is for motor control
  if (strcmp(topic, mqtt_topic) == 0) 
  {
    // Fetch values for motor control
    int newSpeed = doc["MotorSpeed"];
    int newDirection = doc["MotorDirection"];

    // Set the new motor speed and direction
    setMotor(newSpeed, newDirection);
  }
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



//publish message to MQTT broker
void publishMQTTMessage(int speed, int direction)
{
  //create JSCON message for MQTT publish
  doc.clear();
  controlState = "";
  doc["MotorSpeed"] = speed;
  doc["MotorDirection"] = direction;
  serializeJson(doc, controlState);
  Serial.println(controlState);

  //publish MQTT message
  mqttclient.publish(mqtt_topic, controlState.c_str());
  doc.clear();
}

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

// Function to set the motor speed and direction
void setMotor(int speed, int direction) {
  MotorSpeed = speed;
  MotorDirection = direction;

  if (MotorDirection == 1) {            
    digitalWrite(MotorInput1, HIGH);    // Start first motor in forward direction
    digitalWrite(MotorInput2, LOW);
    digitalWrite(MotorInput3, HIGH);    //Start second motor in forward direction
    digitalWrite(MotorInput4, LOW);
  } else if (MotorDirection == 2) {     
    digitalWrite(MotorInput1, LOW);     // Start first motor in backward direction
    digitalWrite(MotorInput2, HIGH);
    digitalWrite(MotorInput3, LOW);     // Start second motor in backward direction
    digitalWrite(MotorInput4, HIGH);
  } else if (MotorDirection == 0) {     
    digitalWrite(MotorInput1, LOW);     // Stop the first motor
    digitalWrite(MotorInput2, LOW);
    digitalWrite(MotorInput3, LOW);     // Stop the second motor
    digitalWrite(MotorInput4, LOW);
  }

  // Control the motor using the L293D driver, set the motor speed
  analogWrite(MotorEnable1, MotorSpeed); 
  analogWrite(MotorEnable2, MotorSpeed);
}


//initialization function executes once when powered on
void setup() {
  Serial.begin(115200); //Initialising if(DEBUG)Serial Monitor
  Serial.println();
  Serial.println("Disconnecting previously connected WiFi");
  WiFi.disconnect();
  EEPROM.begin(512); //Initialasing EEPROM
  delay(10);

  //connect to Wifi
  connectWifi();

  //enable OTA update
  enableOTA();

  //connect to MQTT broker
  connectMQTT();
  
  // Setup motor control pins
  pinMode(MotorInput1, OUTPUT);
  pinMode(MotorInput2, OUTPUT);
  pinMode(MotorInput3, OUTPUT);
  pinMode(MotorInput4, OUTPUT);
  pinMode(MotorEnable1, OUTPUT);
  pinMode(MotorEnable2, OUTPUT);

  // Initialize the motor direction and speed
  setMotor(0, 0); // Initially, stop the motor
}

//continuous execution function
void loop() {
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

  // Match the request
  if (request.indexOf("/start=1") != -1)  { 
    MotorSpeed = 1023;    // Setting duty cycle to 100%
    MotorDirection=1;
    setMotor(MotorSpeed, MotorDirection);           //Start motors
    publishMQTTMessage(MotorSpeed, MotorDirection);
  }

  if (request.indexOf("/stop=1") != -1)  {
    MotorSpeed=0;    
    MotorDirection=0;
    setMotor(MotorSpeed, MotorDirection);           //Stop motors
    publishMQTTMessage(MotorSpeed, MotorDirection);
  }
  
  if (request.indexOf("/tog=1") != -1)  {
    if(MotorDirection != 0) {
      MotorDirection = (MotorDirection == 1) ? 2 : 1;   // Change motor's rotation direction
    }
    setMotor(MotorSpeed, MotorDirection);
    publishMQTTMessage(MotorSpeed, MotorDirection);
  }
  
  if (request.indexOf("/Req=2") != -1)  {  
    MotorSpeed = 767;
    setMotor(MotorSpeed, MotorDirection);    //Pwm duty cycle 75%
    publishMQTTMessage(MotorSpeed, MotorDirection);
    Pwm_status=1;
  }
  if (request.indexOf("/Req=3") != -1)  { 
    MotorSpeed = 512;
    setMotor(MotorSpeed, MotorDirection);    //Pwm duty cycle 50%
    publishMQTTMessage(MotorSpeed, MotorDirection);
    Pwm_status=2;
  }
  if (request.indexOf("/Req=4") != -1)  {  
    MotorSpeed = 255;
    setMotor(MotorSpeed, MotorDirection);    //Pwm duty cycle 25%
    publishMQTTMessage(MotorSpeed, MotorDirection);
    Pwm_status=3;
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
  wifiClient.println("<hr/><hr>");
  wifiClient.println("<h4><center>Nodemcu DC motor control over WiFi</center></h4>");
  wifiClient.println("<hr/><hr>");
  wifiClient.println("<center>");
  wifiClient.println("<table border=\"1\" width=\"100%\">");
  wifiClient.println("<tr>");

  wifiClient.println("<a href=\"/start=1\"\"><button>Start Motor </button></a><br/><br/>");
  wifiClient.println("<a href=\"/stop=1\"\"><button>Stop Motor </button></a><br/><br/>");
  wifiClient.println("<a href=\"/tog=1\"\"><button>Toggle Direction</button></a><br/><br/>");
  wifiClient.println("<a href=\"/Req=2\"\"><button>Speed - 75% </button></a><br/><br/>");
  wifiClient.println("<a href=\"/Req=3\"\"><button>Speed - 50% </button></a><br/><br/>");
  wifiClient.println("<a href=\"/Req=4\"\"><button>Speed - 25% </button></a><br/><br/>");

  if(MotorDirection!=0){
    wifiClient.println("Motor Powered & Working<br/>" );
  }
  else
    wifiClient.println("Motor at Halt<br/>" );

  if(MotorDirection==1){
    wifiClient.println("Motor rotating in forward direction<br/>" );
  }
  else
    wifiClient.println("Motor rotating in backward direction<br/>" );

  switch(Pwm_status){
    case 1:
      wifiClient.println("Pwm duty cycle 75%<br/>" );
      break;
    case 2:
      wifiClient.println("Pwm duty cycle 50%<br/>" );
      break;  
    case 3:
      wifiClient.println("Pwm duty cycle 25%<br/>" );
      break;    
    default:
      wifiClient.println("Pwm duty cycle 100%<br/>" );
  }
  
  wifiClient.println("</tr>");
  wifiClient.println("</table>");
  wifiClient.println("</center>");
  wifiClient.println("</html>");
  delay(1);
  Serial.println("Client disonnected");
  Serial.println("");

}
//-------- Fuctions used for WiFi credentials saving and connecting to it 
bool testWifi(void)
{
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 2000 ) {
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

void setupAP(void)
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
      //setupAP();
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
