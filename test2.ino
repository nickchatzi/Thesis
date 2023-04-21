#include <WiFiManager.h>
#include <esp_now.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <Arduino_JSON.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <esp_wifi.h>
#include <DNSServer.h>

// Replace with your network credentials (STATION)
const char* ssid = "COSMOTE-C59FA4";
const char* password = "zaFesY5dHSHEq54V";

bool triggerActive = false;

const char* PARAM_INPUT_1 = "threshold_input1";
const char* PARAM_INPUT_2 = "enable_arm_input1";
const char* PARAM_INPUT_3 = "threshold_input2";
const char* PARAM_INPUT_4 = "enable_arm_input2";
const char* PARAM_INPUT_5 = "threshold_input3";
const char* PARAM_INPUT_6 = "enable_arm_input3";


#define DHTPIN 27     // Digital pin connected to the DHT sensor
#define DHTTYPE    DHT22  //define sensor type
#define DOOR_SENSOR_PIN  33 // Digital pin connected to door sensor's pin 
DHT dht(DHTPIN, DHTTYPE); 

String threshold1 = "25.0"; //Set initial temperature threshold
String enableArmChecked1 = "checked";  //Set initial checkbox status to checked
String inputMessage1 = "true"; //string to set the value "true" if checkbox is checked
String fanStatus1;
String threshold2 = "25.0"; //Set initial temperature threshold
String enableArmChecked2 = "checked";  //Set initial checkbox status to checked
String inputMessage2 = "true"; //string to set the value "true" if checkbox is checked
String fanStatus2;
String threshold3 = "25.0"; //Set initial temperature threshold
String enableArmChecked3 = "checked";  //Set initial checkbox status to checked
String inputMessage3 = "true"; //string to set the value "true" if checkbox is checked
String fanStatus3;
int doorState;
float tempc1;
float tempc2;
float tempc3;
int drst1;
int drst2;
int drst3;

unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 10000;        // Interval at which to publish sensor readings

//MAC Address of the receiver 
uint8_t broadcastAddress1[] = {0xEC, 0x94, 0xCB, 0x6F, 0x79, 0x94};
uint8_t broadcastAddress2[] = {0xC8, 0xF0, 0x9E, 0xF2, 0xC6, 0x40};
uint8_t broadcastAddress3[] = {0x1C, 0x9D, 0xC2, 0x54, 0x6E, 0xD8};

// Structure to receive data
typedef struct struct_message {
  int id;
  float tempc;
  float tempf;
  float hum;
  float volts;
  float perc;
  int drst;
} struct_message;

typedef struct struct_message2 {
  float thres;
  float thres2;
  float thres3;
  String checkbox;
  String checkbox2;
  String checkbox3;
} struct_message2;

struct_message incomingReadings;
struct_message2 myData;

JSONVar board;

// Set your Static IP address
IPAddress local_IP(192, 168, 1, 10);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);

IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

AsyncWebServer server(80);
AsyncEventSource events("/events");


// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) { 
  // Copies the sender mac address to a string
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  
  board["id"] = incomingReadings.id;
  board["temperaturec"] = incomingReadings.tempc;
  board["temperaturef"] = incomingReadings.tempf;
  board["humidity"] = incomingReadings.hum;
  board["volts"] = incomingReadings.volts;
  board["percentage"] = incomingReadings.perc;
  board["doorstate"] = incomingReadings.drst;
  String jsonString = JSON.stringify(board);
  events.send(jsonString.c_str(), "new_readings", millis());


  if (incomingReadings.id == 1) {
    tempc1 = incomingReadings.tempc;
    drst1 = incomingReadings.drst;

  } else if (incomingReadings.id == 2) {
    tempc2 = incomingReadings.tempc;
    drst2 = incomingReadings.drst;

  } else if (incomingReadings.id == 3) {
    tempc3 = incomingReadings.tempc;
    drst3 = incomingReadings.drst;
  }
  
  Serial.printf("Board ID %u: %u bytes\n", incomingReadings.id, len);
  Serial.printf("tc value: %4.2f \n", incomingReadings.tempc);
  Serial.printf("tf value: %4.2f \n", incomingReadings.tempf);
  Serial.printf("h value: %4.2f \n", incomingReadings.hum);
  Serial.printf("v value: %4.2f \n", incomingReadings.volts);
  Serial.printf("p value: %4.2f \n", incomingReadings.perc);
  Serial.print(incomingReadings.drst);
  Serial.println();
}

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  Serial.print("Packet to: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Insert your SSID
constexpr char WIFI_SSID[] = "COSMOTE-C59FA4";

int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script src="https://kit.fontawesome.com/7ae1530eb9.js" crossorigin="anonymous"></script>
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    p2 {font-size: 1.1rem;}
    p3 {font-size: 1.1rem;
        color:#4CAF50;}
    p4 {font-size: 1.5rem;
    	vertical-align:middle;
      	padding-bottom: 15px;
        }
     p5 {font-size: 1.5rem;
     	}
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
    input[type="submit"]{
      background: #0066A2;
	    color: white;
	    border-style: outset;
	    border-color: #0066A2;
	    height: 50px;
	    width: 100px;
	    font: bold15px arial,sans-serif;
	    text-shadow: none;}
    .element1 {	
		display: inline-block;
      	background-color: lightgrey;
      	width:500px;
      	border-width: 5px;
      	border-color: #4CA64C;
      	border-style: solid;
      	text-align: center;
      	}
     .element2 {	
		display: inline-block; 
      	background-color: lightgrey;
      	width:500px;
      	border-width: 5px;
      	border-color: #4CA64C;
      	border-style: solid;
      	text-align: center;
      	}
      .element3 {	
		display: inline-block;
      	background-color: lightgrey;
      	width:500px;
      	border-width: 5px;
      	border-color: #FF8C00;
      	border-style: solid;
      	text-align: center;
      	}
     .element4 {	
		display: inline-block; 
      	background-color: lightgrey;
      	width:500px;
      	border-width: 5px;
      	border-color: #FF8C00;
      	border-style: solid;
      	text-align: center;
      	}
     .element5 {	
		display: inline-block; 
      	background-color: lightgrey;
      	width:500px;
      	border-width: 5px;
      	border-color: #800080;
      	border-style: solid;
      	text-align: center;
      	}
     .element6 {	
		display: inline-block; 
      	background-color: lightgrey;
      	width:500px;
      	border-width: 5px;
      	border-color: #800080;
      	border-style: solid;
      	text-align: center;
      	} 
          
  </style>
</head>
<body>
  <h2> FREEZER UNIT CONTROL SERVER </h2>
  <div class="element1">
  <p4> 
    <br><strong> INDOOR</strong></p4>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature in Celsius</span> 
    <span id="tc1"></span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature in Fahrenheit</span> 
    <span id="tf1"></span>
    <sup class="units">&deg;F</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Humidity</span>
    <span id="h1"></span>
    <sup class="units">&percnt;</sup>
  </p>
  </div>
    <div class="element2">
   
   <p4> 
    <br><strong><i class="fa-solid fa-battery-full" style="color:#008000;"></i> BATTERY STATUS</strong></p4>
   <p>
   	<i class="fa-solid fa-bolt" style="color:#FFA500"></i>
     <span class="dht-labels"> Battery Voltage:</span> 
    <span id="v1"></span>
    <span class="units">V </span>
   </p>
   <p>
   	<i class="fa-solid fa-percent" style="color:#800000"></i>
     <span class="dht-labels"> Battery Percentage: </span>
    <span id="p1"></span>
    <sup class="units">&percnt;</sup>
   </p>
  <p><br></p>
   </div>  
  <p></p>
  <form action="/get">
       <input type="checkbox" name="enable_arm_input1" value="true" id="checkbox1"  %ENABLE_ARM_INPUT1%></span> 
         
    <span class="dht-labels"><strong>Freezer 1 </strong> set to: <input type="number" step="0.1" name="threshold_input1" value="%THRESHOLD1%" required></span> <br><br>  
    <p3 id="message1"></p3> <br><br>
    <p4><strong>Freezer 1 is <span id="fanStatus1">%FAN_STATUS1%</span>.</strong></p4> <p></p>
 
  <div class="element3">
  <p4> 
    <br><strong> INDOOR</strong></p4>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature in Celsius</span> 
    <span id="tc2"></span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature in Fahrenheit</span> 
    <span id="tf2"></span>
    <sup class="units">&deg;F</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Humidity</span>
    <span id="h2"></span>
    <sup class="units">&percnt;</sup>
  </p>
  </div>
    <div class="element4">
   
   <p4> 
    <br><strong><i class="fa-solid fa-battery-full" style="color:#008000;"></i> BATTERY STATUS</strong></p4>
   <p>
   	<i class="fa-solid fa-bolt" style="color:#FFA500"></i>
     <span class="dht-labels"> Battery Voltage:</span> 
    <span id="v2"></span>
    <span class="units">V </span>
   </p>
   <p>
   	<i class="fa-solid fa-percent" style="color:#800000"></i>
     <span class="dht-labels"> Battery Percentage: </span>
    <span id="p2"></span>
    <sup class="units">&percnt;</sup>
   </p>
  <p><br></p>
   </div>   
 <p></p>

  <input type="checkbox" name="enable_arm_input2" value="true"  id="checkbox2"  %ENABLE_ARM_INPUT2%></span> 
  <span class="dht-labels"><strong>Freezer 2 </strong> set to: <input type="number" step="0.1" name="threshold_input2" value="%THRESHOLD2%" required></span> <br><br>  
  <p3 id="message2"></p3> <br><br>
  <p4><strong>Freezer 2 is <span id="fanStatus2">%FAN_STATUS2%</span>.</strong></p4> <p></p> 
  
    <div class="element5">
  <p4> 
    <br><strong> INDOOR</strong></p4>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature in Celsius</span> 
    <span id="tc3"></span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature in Fahrenheit</span> 
    <span id="tf3"></span>
    <sup class="units">&deg;F</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Humidity</span>
    <span id="h3"></span>
    <sup class="units">&percnt;</sup>
  </p>
  </div>
    <div class="element6">
   
   <p4> 
    <br><strong><i class="fa-solid fa-battery-full" style="color:#008000;"></i> BATTERY STATUS</strong></p4>
   <p>
   	<i class="fa-solid fa-bolt" style="color:#FFA500"></i>
     <span class="dht-labels"> Battery Voltage:</span> 
    <span id="v3"></span>
    <span class="units">V </span>
   </p>
   <p>
   	<i class="fa-solid fa-percent" style="color:#800000"></i>
     <span class="dht-labels"> Battery Percentage: </span>
    <span id="p3"></span>
    <sup class="units">&percnt;</sup>
   </p>
  <p><br></p>
   </div>  
  <p></p>
  <form action="/get">
       <input type="checkbox" name="enable_arm_input3" value="true" id="checkbox3"  %ENABLE_ARM_INPUT3%></span> 
         
    <span class="dht-labels"><strong>Freezer 3 </strong> set to: <input type="number" step="0.1" name="threshold_input3" value="%THRESHOLD3%" required></span> <br><br>  
    <p3 id="message3"></p3> <br><br>
    <p4><strong>Freezer 3 is <span id="fanStatus3">%FAN_STATUS3%</span>.</strong></p4> <p></p>
  
  <p2><input type="submit" value="Submit"></p2>   
  </form> 
<script>
 var checkbox1 = document.getElementById("checkbox1");
 var message1 = document.getElementById("message1");
 var checkbox2 = document.getElementById("checkbox2");
 var message2 = document.getElementById("message2");
 var checkbox3 = document.getElementById("checkbox3");
 var message3 = document.getElementById("message3");
 
 
 setInterval(function() {
      if (checkbox1.checked) {
        message1.innerHTML = "The freezer unit is working in automatic operation";
      } else {
        message1.innerHTML = "";
      }
    }, 50); // Check every 500ms
    
 setInterval(function() {
      if (checkbox2.checked) {
        message2.innerHTML = "The freezer unit is working in automatic operation";
      } else {
        message2.innerHTML = "";
      }
    }, 50); // Check every 500ms

 setInterval(function() {
      if (checkbox3.checked) {
        message3.innerHTML = "The freezer unit is working in automatic operation";
      } else {
        message3.innerHTML = "";
      }
    }, 50); // Check every 500ms
   
     

if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);
 
 source.addEventListener('new_readings', function(e) {
  console.log("new_readings", e.data);
  var obj = JSON.parse(e.data);
  document.getElementById("tc"+obj.id).innerHTML = obj.temperaturec.toFixed(1);
  document.getElementById("tf"+obj.id).innerHTML = obj.temperaturef.toFixed(1);
  document.getElementById("h"+obj.id).innerHTML = obj.humidity.toFixed(1);
  document.getElementById("v"+obj.id).innerHTML = obj.volts.toFixed(2);
  document.getElementById("p"+obj.id).innerHTML = obj.percentage.toFixed(2);
 }, false);
}

  setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("fanStatus1").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/status1", true);
  xhttp.send();
}, 500 ) ;

  setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("fanStatus2").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/status2", true);
  xhttp.send();
}, 500 ) ;

  setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("fanStatus3").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/status3", true);
  xhttp.send();
}, 500 ) ;
</script>
</body>
</html>)rawliteral";

String processor(const String& var){
  if(var == "THRESHOLD1"){
  return threshold1;
  }
  else if(var == "ENABLE_ARM_INPUT1"){
    return enableArmChecked1;
  }  
  else if(var=="FAN_STATUS1"){
    return fanStatus1;
  }
  else if(var == "THRESHOLD2"){
    return threshold2;
  }  
  else if(var=="ENABLE_ARM_INPUT2"){
    return enableArmChecked2;
  }    
  else if(var=="FAN_STATUS2"){
    return fanStatus2;  
  }
  else if(var == "THRESHOLD3"){
    return threshold3;
  }  
  else if(var=="ENABLE_ARM_INPUT3"){
    return enableArmChecked3;
  }    
  else if(var=="FAN_STATUS3"){
    return fanStatus3;  
  }
                  
  return String();
  }  

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // Set the device as a Station and Soft Access Point simultaneously
  WiFi.mode(WIFI_AP_STA);
  Serial.println(WiFi.macAddress());  

  // Set device as a Wi-Fi Station
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }

  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);

  int32_t channel = getWiFiChannel(WIFI_SSID);

  WiFi.printDiag(Serial); // Uncomment to verify channel number before
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  WiFi.printDiag(Serial); // Uncomment to verify channel change after

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }

  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());
  Serial.println(WiFi.macAddress());

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);

  // register peer
  esp_now_peer_info_t peerInfo;
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
    
  memcpy(peerInfo.peer_addr, broadcastAddress1, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  
  memcpy(peerInfo.peer_addr, broadcastAddress2, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  memcpy(peerInfo.peer_addr, broadcastAddress3, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/status1", HTTP_GET, [](AsyncWebServerRequest *request){
  // Return the fan status for freezer unit 1
  String fanStatus1 = (tempc1 > myData.thres && myData.checkbox == "true" && drst1==LOW) ? "on" : "off";
  request->send(200, "text/plain", fanStatus1);
  });

  server.on("/status2", HTTP_GET, [](AsyncWebServerRequest *request){
  // Return the fan status for freezer unit 2
  String fanStatus2 = (tempc2 > myData.thres2 && myData.checkbox2 == "true" && drst2==LOW) ? "on" : "off";
  request->send(200, "text/plain", fanStatus2);
  });

  server.on("/status3", HTTP_GET, [](AsyncWebServerRequest *request){
  // Return the fan status for freezer unit 3
  String fanStatus3 = (tempc3 > myData.thres3 && myData.checkbox3 == "true" && drst3==LOW) ? "on" : "off";
  request->send(200, "text/plain", fanStatus3);
  });

  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    // GET threshold_input value on <ESP_IP>/get?threshold_input=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      threshold1 = request->getParam(PARAM_INPUT_1)->value();
      // GET enable_arm_input value on <ESP_IP>/get?enable_arm_input=<inputMessage2>
      if (request->hasParam(PARAM_INPUT_2)) {
        inputMessage1 = request->getParam(PARAM_INPUT_2)->value();
        enableArmChecked1 = "checked";
      }
      else {
        inputMessage1 = "false";
        enableArmChecked1= "";
      }
    }
    Serial.println(threshold1);
    Serial.println(inputMessage1);

    if (request->hasParam(PARAM_INPUT_3)) {
      threshold2 = request->getParam(PARAM_INPUT_3)->value();
      // GET enable_arm_input value on <ESP_IP>/get?enable_arm_input=<inputMessage2>
      if (request->hasParam(PARAM_INPUT_4)) {
        inputMessage2 = request->getParam(PARAM_INPUT_4)->value();
        enableArmChecked2 = "checked";
      }
      else {
        inputMessage2 = "false";
        enableArmChecked2 = "";
      }
    }
    Serial.println(threshold2);
    Serial.println(inputMessage2);

    if (request->hasParam(PARAM_INPUT_5)) {
      threshold3 = request->getParam(PARAM_INPUT_5)->value();
      // GET enable_arm_input value on <ESP_IP>/get?enable_arm_input=<inputMessage2>
      if (request->hasParam(PARAM_INPUT_6)) {
        inputMessage3 = request->getParam(PARAM_INPUT_6)->value();
        enableArmChecked3 = "checked";
      }
      else {
        inputMessage3 = "false";
        enableArmChecked3 = "";
      }
    }
    Serial.println(threshold3);
    Serial.println(inputMessage3);
    request->send(200, "text/html", "HTTP GET request sent to your ESP.<br><a href=\"/\">Return to Home Page</a>");
  });
  server.onNotFound(notFound);    
   
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  server.begin();
}
 
void loop() {
  static unsigned long lastEventTime = millis();
  static const unsigned long EVENT_INTERVAL_MS = 10000;
  if ((millis() - lastEventTime) > EVENT_INTERVAL_MS) {
    events.send("ping",NULL,millis());
    lastEventTime = millis();
  }
   unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // Save the last time a new reading was published
    previousMillis = currentMillis;
    myData.thres = threshold1.toFloat();
    myData.checkbox = inputMessage1;
    myData.thres2 = threshold2.toFloat();
    myData.checkbox2 = inputMessage2;
    myData.thres3 = threshold3.toFloat();
    myData.checkbox3 = inputMessage3; }

 esp_err_t result1 = esp_now_send(broadcastAddress1, (uint8_t *) &myData, sizeof(myData));
  if (result1 == ESP_OK) {
    Serial.println("Sent with success");
    }
  else {
    Serial.println("Error sending the data");}

  esp_err_t result2 = esp_now_send(broadcastAddress2, (uint8_t *) &myData, sizeof(myData));
  if (result2 == ESP_OK) {
    Serial.println("Sent with success");
    }
  else {
    Serial.println("Error sending the data");}
  
  esp_err_t result3 = esp_now_send(broadcastAddress3, (uint8_t *) &myData, sizeof(myData));
  if (result3 == ESP_OK) {
    Serial.println("Sent with success");
    }
  else {
    Serial.println("Error sending the data");} 
  }   
