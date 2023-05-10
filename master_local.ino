/*© 2023 Nick Chatzilamprou <nickhatzi1998s@gmail.com>*/

//include libraries
#include <esp_now.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <Arduino_JSON.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <esp_wifi.h>
#include <DNSServer.h>

//declare router credentials
const char* ssid = "*********";
const char* password = "**********";

//threshold_input and enable_arm_input are threshold value and checkbox
const char* PARAM_INPUT_1 = "threshold_input1"; 
const char* PARAM_INPUT_2 = "enable_arm_input1";
const char* PARAM_INPUT_3 = "threshold_input2";
const char* PARAM_INPUT_4 = "enable_arm_input2";
const char* PARAM_INPUT_5 = "threshold_input3";
const char* PARAM_INPUT_6 = "enable_arm_input3";


#define DHTPIN 27     // Digital pin connected to the DHT sensor
#define DHTTYPE    DHT22  //define sensor type
DHT dht(DHTPIN, DHTTYPE); 

String threshold1 = "25.0"; //Set initial temperature threshold 1
String enableArmChecked1 = "checked";  //Set initial checkbox 1 status to checked
String inputMessage1 = "true"; //string to set the value "true" if checkbox 1 is checked
String fanStatus1; //string to save the fun status 1
String threshold2 = "25.0"; //Set initial temperature threshold 2
String enableArmChecked2 = "checked";  //Set initial checkbox 2 status to checked
String inputMessage2 = "true"; //string to set the value "true" if checkbox 2 is checked
String fanStatus2; //string to save the fun status 2
String threshold3 = "25.0"; //Set initial temperature threshold 3
String enableArmChecked3 = "checked";  //Set initial checkbox 3 status to checked
String inputMessage3 = "true"; //string to set the value "true" if checkbox 3 is checked
String fanStatus3; //string to save the fun status 3
int doorState; //doorstate either open or closed
float tempc1; //temperature value from sensor device 1
float tempc2; //temperature value from sensor device 2
float tempc3; //temperature value from sensor device 3
int drst1; //door state from sensor device 1
int drst2; //door state from sensor device 2
int drst3; //door state from sensor device 3
float volts1;  //battery voltage value from sensor device 1
float volts2; //battery voltage value from sensor device 2
float volts3; //battery voltage value from sensor device 3

unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 10000;        // Interval at which to publish sensor readings

//MAC Address of the receivers  
uint8_t broadcastAddress1[] = {0xEC, 0x94, 0xCB, 0x6F, 0x79, 0x94};
uint8_t broadcastAddress2[] = {0xC8, 0xF0, 0x9E, 0xF2, 0xC6, 0x40};
uint8_t broadcastAddress3[] = {0x1C, 0x9D, 0xC2, 0x54, 0x6E, 0xD8};

// Structure to receive data
typedef struct struct_message {
  int id; //id of each indivitual sensor device
  float tempc; //incoming temperature (in Celcius) readings
  float tempf; //incoming temperature (in Fahrenheit) readings
  float hum; //incoming humidity readings
  float volts; //incoming battery voltage readings
  float perc; //incoming battery percentage readings
  int drst; //incoming battery status readings
}
struct_message;

//Structure to send data
typedef struct struct_message2 {
  float thres; //outgoing threshold value for sensor device 1
  float thres2; //outgoing threshold value for sensor device 2
  float thres3; //outgoing threshold value for sensor device 3
  String checkbox; //outgoing checkbox state for sensor device 1
  String checkbox2; //outgoing checkbox state for sensor device 2
  String checkbox3; //outgoing checkbox state for sensor device 3
}
struct_message2;

struct_message incomingReadings; //link structure struct_mwssage1 to incomingReadings object
struct_message2 myData; ////link structure struct_message2 to myData object

JSONVar board;

// Set Static IP address
IPAddress local_IP(192, 168, 1, 10);
// Set Gateway IP address
IPAddress gateway(192, 168, 1, 1);

IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

AsyncWebServer server(80); //// Create AsyncWebServer object on port 80
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
  
  //access member variable to incomingReadings object
  board["id"] = incomingReadings.id; 
  board["temperaturec"] = incomingReadings.tempc;
  board["temperaturef"] = incomingReadings.tempf;
  board["humidity"] = incomingReadings.hum;
  board["volts"] = incomingReadings.volts;
  board["percentage"] = incomingReadings.perc;
  board["doorstate"] = incomingReadings.drst;
  String jsonString = JSON.stringify(board);
  events.send(jsonString.c_str(), "new_readings", millis());

  //set temperature, door state and battery voltage readings to object depending the ID of each sensor device
  //variables for the first sensor device
  if (incomingReadings.id == 1) {
    tempc1 = incomingReadings.tempc;
    drst1 = incomingReadings.drst;
    volts1 = incomingReadings.volts;
  //variables for the second sensor device
  } else if (incomingReadings.id == 2) {
    tempc2 = incomingReadings.tempc;
    drst2 = incomingReadings.drst;
    volts2 = incomingReadings.volts;
  //variables for the third sensor device
  } else if (incomingReadings.id == 3) {
    tempc3 = incomingReadings.tempc;
    drst3 = incomingReadings.drst;
    volts3 = incomingReadings.volts;
  }
  //print in serial monitor incoming readings (for debugging reasons)
  Serial.printf("Board ID %u: %u bytes\n", incomingReadings.id, len);
  Serial.printf("tc value: %4.2f \n", incomingReadings.tempc);
  Serial.printf("tf value: %4.2f \n", incomingReadings.tempf);
  Serial.printf("h value: %4.2f \n", incomingReadings.hum);
  Serial.printf("v value: %4.2f \n", incomingReadings.volts);
  Serial.printf("p value: %4.2f \n", incomingReadings.perc);
  Serial.print(incomingReadings.drst);
  Serial.println();
}

// callback function when data are sent
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

//html, javascript and css for functining and styling the web server
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
   <p>
  <p2> <strong><span id="volts1"></span></strong></p2>
  </p>
  </div>  
  <p></p>
  <form action="/get">
       <input type="checkbox" name="enable_arm_input1" value="true" id="checkbox1"  %ENABLE_ARM_INPUT1%></span> 
         
    <span class="dht-labels"><strong>Freezer 1 </strong> set to: <input type="number" step="0.1" name="threshold_input1" value="%THRESHOLD1%" required></span> <br><br>  
    <p3 id="message1"></p3> <br><br>
    <p4><strong>Freezer 1 is <span id="fanStatus1">%FAN_STATUS1%</span>.</strong></p4> <br><br>
    <p4><strong><span id="drst1"></span></strong></p4> <p></p>
 
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
   <p>
  <p2> <strong><span id="volts2"></span></strong></p2>
  </p>
   </div>   
 <p></p>

  <input type="checkbox" name="enable_arm_input2" value="true"  id="checkbox2"  %ENABLE_ARM_INPUT2%></span> 
  <span class="dht-labels"><strong>Freezer 2 </strong> set to: <input type="number" step="0.1" name="threshold_input2" value="%THRESHOLD2%" required></span> <br><br>  
  <p3 id="message2"></p3> <br><br>
  <p4><strong>Freezer 2 is <span id="fanStatus2">%FAN_STATUS2%</span>.</strong></p4> <br><br> 
  <p4><strong><span id="drst2"></span></strong></p4> <p></p>

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
   <p>
  <p2> <strong><span id="volts3"></span></strong></p2>
  </p>
   </div>  
  <p></p>
  <form action="/get">
       <input type="checkbox" name="enable_arm_input3" value="true" id="checkbox3"  %ENABLE_ARM_INPUT3%></span> 
         
    <span class="dht-labels"><strong>Freezer 3 </strong> set to: <input type="number" step="0.1" name="threshold_input3" value="%THRESHOLD3%" required></span> <br><br>  
    <p3 id="message3"></p3> <br><br>
    <p4><strong>Freezer 3 is <span id="fanStatus3">%FAN_STATUS3%</span>.</strong></p4> <br><br>
    <p4><strong><span id="drst3"></span></strong></p4> <p></p>
  
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
    }, 50); // Check every 50ms
    
 setInterval(function() {
      if (checkbox2.checked) {
        message2.innerHTML = "The freezer unit is working in automatic operation";
      } else {
        message2.innerHTML = "";
      }
    }, 50); // Check every 50ms

 setInterval(function() {
      if (checkbox3.checked) {
        message3.innerHTML = "The freezer unit is working in automatic operation";
      } else {
        message3.innerHTML = "";
      }
    }, 50); // Check every 50ms
   
     

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

  setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("drst1").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/drstatus1", true);
  xhttp.send();
}, 500 ) ;

  setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("drst2").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/drstatus2", true);
  xhttp.send();
}, 500 ) ;

  setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("drst3").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/drstatus3", true);
  xhttp.send();
}, 500 ) ;

  setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("volts1").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/btstatus1", true);
  xhttp.send();
}, 500 ) ;

  setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("volts2").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/btstatus2", true);
  xhttp.send();
}, 500 ) ;

  setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("volts3").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/btstatus3", true);
  xhttp.send();
}, 500 ) ;

</script>
</body>
</html>)rawliteral";

//function that replaces all the %PLACEHOLDERS% on the HTML 
String processor(const String& var){
  if(var == "THRESHOLD1"){ //if placeholder is the specific variable, replace with var threshold1
    return threshold1;
  }
  else if(var == "ENABLE_ARM_INPUT1"){ //if placeholder is the specific variable, replace with var enableArmChecked1
    return enableArmChecked1;
  }  
  else if(var == "THRESHOLD2"){ //if placeholder is the specific variable, replace with var threshold2
    return threshold2;
  }  
  else if(var=="ENABLE_ARM_INPUT2"){ //if placeholder is the specific variable, replace with var enableArmChecked2
    return enableArmChecked2;
  } 
  else if(var == "THRESHOLD3"){ //if placeholder is the specific variable, replace with var threshold3
    return threshold3;
  }  
  else if(var=="ENABLE_ARM_INPUT3"){ //if placeholder is the specific variable, replace with var enableArmChecked3
    return enableArmChecked3;
  }                 
  return String();
  }  
//Handle this in case where the url is not found on the server.
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

  //if configuration of ip,gw and sn is not successfull print message to serial monitor
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  // serial print ip address (for debugging)
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());
  Serial.println(WiFi.macAddress());

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  //callback function that will be called when a message is received
  esp_now_register_recv_cb(OnDataRecv);
  //callback function that will be called when a message is sent
  esp_now_register_send_cb(OnDataSent);

  // register peer
  esp_now_peer_info_t peerInfo;
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  //Add peer to send the data to sensor device 1  
  memcpy(peerInfo.peer_addr, broadcastAddress1, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  //Add peer to send the data to sensor device 2 
  memcpy(peerInfo.peer_addr, broadcastAddress2, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  //Add peer to send the data to sensor device 3
  memcpy(peerInfo.peer_addr, broadcastAddress3, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  //Handle requests when http req is received for the processor string
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  //Handle requests when http req is received for fan status 1
  server.on("/status1", HTTP_GET, [](AsyncWebServerRequest *request){
    // Return the fan status for freezer unit 1
    String fanStatus1 = (tempc1 > myData.thres && myData.checkbox == "true" && drst1==LOW) ? "on" : "off"; 
    // Send the HTML response with the appropriate HTTP status code
    request->send(200, "text/plain", fanStatus1);
  });

  //Handle requests when http req is received for fan status 2
  server.on("/status2", HTTP_GET, [](AsyncWebServerRequest *request){
    // Return the fan status for freezer unit 2
    String fanStatus2 = (tempc2 > myData.thres2 && myData.checkbox2 == "true" && drst2==LOW) ? "on" : "off";
    // Send the HTML response with the appropriate HTTP status code
    request->send(200, "text/plain", fanStatus2);
  });

  //Handle requests when http req is received for fan status 3
  server.on("/status3", HTTP_GET, [](AsyncWebServerRequest *request){
    // Return the fan status for freezer unit 3
    String fanStatus3 = (tempc3 > myData.thres3 && myData.checkbox3 == "true" && drst3==LOW) ? "on" : "off";
    // Send the HTML response with the appropriate HTTP status code
    request->send(200, "text/plain", fanStatus3);
  });

  //Handle requests when http req is received for door status 1
  server.on("/drstatus1", HTTP_GET, [] (AsyncWebServerRequest *request) {
    // Determine the state of the magnetic reed switch
    bool doorOpen = (drst1 == HIGH);
    // Set the text color based on the state of the door
    String textColor = doorOpen ? "red" : "green";
    // Create an HTML response with the appropriate text color
    String htmlResponse = "<html><body><p4 style=\"color: " + textColor + "\">The door is " + (doorOpen ? "open." : "closed.") + "</p4></body></html>";
   // Send the HTML response with the appropriate HTTP status code
    request->send(200, "text/html", htmlResponse);
  });

  //Handle requests when http req is received for door status 2
  server.on("/drstatus2", HTTP_GET, [] (AsyncWebServerRequest *request) {
    // Determine the state of the magnetic reed switch
    bool doorOpen = (drst2 == HIGH);
    // Set the text color based on the state of the door
    String textColor = doorOpen ? "red" : "green";
    // Create an HTML response with the appropriate text color
    String htmlResponse = "<html><body><p4 style=\"color: " + textColor + "\">The door is " + (doorOpen ? "open." : "closed.") + "</p4></body></html>";
    // Send the HTML response with the appropriate HTTP status code
    request->send(200, "text/html", htmlResponse);
  });

  //Handle requests when http req is received for door status 3
  server.on("/drstatus3", HTTP_GET, [] (AsyncWebServerRequest *request) {
    // Determine the state of the magnetic reed switch
    bool doorOpen = (drst3 == HIGH);
    // Set the text color based on the state of the door
    String textColor = doorOpen ? "red" : "green";
    // Create an HTML response with the appropriate text color
    String htmlResponse = "<html><body><p4 style=\"color: " + textColor + "\">The door is " + (doorOpen ? "open." : "closed.") + "</p4></body></html>";
    // Send the HTML response with the appropriate HTTP status code
    request->send(200, "text/html", htmlResponse);
  });

  //Handle requests when http req is received for battery voltage voltage 1
  server.on("/btstatus1", HTTP_GET, [](AsyncWebServerRequest *request) {
    float batt_value1 = volts1; //float variable to check the batt voltage of sensor device 1
    String text1; //string for th displayed text on the server
    if (batt_value1 >= 3.90 && batt_value1 <= 4.40) {
      text1 = "Battery is fully charged!";
    } else if (batt_value1 >= 3.20 && batt_value1 < 3.90) {
      text1 = "Battery is OK.";
    } else if (batt_value1 >= 2.90 && batt_value1 < 3.20) {
      text1 = "Battery needs to be charged";
    } else if (batt_value1 > 2.50 && batt_value1 < 2.90) {
      text1 = "Battery is critical! Please charge.";
    } else {
      text1 = "LOADING...";
    }
    // Create an HTML response with the appropriate text 
    String htmlResponse = "<html><body>";
    //if battery is critical print text in red
    if (text1 == "Battery is critical! Please charge.") {
      htmlResponse += "<p2 style='color:red'>" + text1 + "</p2>";
    } else {
      htmlResponse += "<p2>" + text1 + "</p2>";
    }
    htmlResponse += "</body></html>";
    // Send the HTML response with the appropriate HTTP status code
    request->send(200, "text/html", htmlResponse);
  });

  //Handle requests when http req is received for battery voltage voltage 2
  server.on("/btstatus2", HTTP_GET, [](AsyncWebServerRequest *request) {
    float batt_value2 = volts2; //float variable to check the batt voltage of sensor device 2
    String text2; //string for th displayed text on the server
    if (batt_value2 >= 4.30 && batt_value2 <= 3.90) {
      text2 = "Battery is fully charged!";
    } else if (batt_value2 >= 3.20 && batt_value2 < 3.90) {
      text2 = "Battery is OK.";
    } else if (batt_value2 >= 2.90 && batt_value2 < 3.20) {
      text2 = "Battery needs to be charged";
    } else if (batt_value2 > 2.50 && batt_value2 < 2.90) {
      text2 = "Battery is critical! Please charge.";
    } else {
      text2 = "LOADING...";
    }
    // Create an HTML response with the appropriate text 
    String htmlResponse = "<html><body>";
    //if battery is critical print text in red
    if (text2 == "Battery is critical! Please charge.") {
      htmlResponse += "<p2 style='color:red'>" + text2 + "</p2>";
    } else {
      htmlResponse += "<p2>" + text2 + "</p2>";
    }
    htmlResponse += "</body></html>";
    // Send the HTML response with the appropriate HTTP status code
    request->send(200, "text/html", htmlResponse);
  });

  //Handle requests when http req is received for battery voltage voltage 3
  server.on("/btstatus3", HTTP_GET, [](AsyncWebServerRequest *request) {
    float batt_value3 = volts3; //float variable to check the batt voltage of sensor device 3
    String text3; //string for th displayed text on the server
    if (batt_value3 >= 4.30 && batt_value3 <= 3.90) {
      text3 = "Battery is fully charged!";
    } else if (batt_value3 >= 3.20 && batt_value3 < 3.90) {
      text3 = "Battery is OK.";
    } else if (batt_value3 >= 2.90 && batt_value3 < 3.20) {
      text3 = "Battery needs to be charged";
    } else if (batt_value3 > 2.50 && batt_value3 < 2.90) {
      text3 = "Battery is critical! Please charge.";
    } else {
      text3 = "LOADING...";
    }
    // Create an HTML response with the appropriate text 
    String htmlResponse = "<html><body>";
    //if battery is critical print text in red
    if (text3 == "Battery is critical! Please charge.") {
      htmlResponse += "<p2 style='color:red'>" + text3 + "</p2>";
    } else {
      htmlResponse += "<p2>" + text3 + "</p2>";
    }
    htmlResponse += "</body></html>";
    // Send the HTML response with the appropriate HTTP status code
    request->send(200, "text/html", htmlResponse);
  });

  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    // if request has input parameter 1
    if (request->hasParam(PARAM_INPUT_1)) {
      // save this parameter into variable threshold1
      threshold1 = request->getParam(PARAM_INPUT_1)->value();
      // if request has input parameter 1
      if (request->hasParam(PARAM_INPUT_2)) {
        // save this parameter into variable inputMessage1
        inputMessage1 = request->getParam(PARAM_INPUT_2)->value();
        //change checkbox 1 status to checked
        enableArmChecked1 = "checked";
      }
      //else checkbox is not checked set string as false
      else {
        inputMessage1 = "false";
        enableArmChecked1= "";
      }
    }
    //serial print for debug
    Serial.println(threshold1);
    Serial.println(inputMessage1);

    // if request has input parameter 3
    if (request->hasParam(PARAM_INPUT_3)) {
      // save this parameter into variable threshold2
      threshold2 = request->getParam(PARAM_INPUT_3)->value();
      // if request has input parameter 4
      if (request->hasParam(PARAM_INPUT_4)) {
        // save this parameter into variable inputMessage2
        inputMessage2 = request->getParam(PARAM_INPUT_4)->value();
        //change checkbox 2 status to checked
        enableArmChecked2 = "checked";
      }
      //else checkbox 2 is not checked set string as false
      else {
        inputMessage2 = "false";
        enableArmChecked2 = "";
      }
    }
    //serial print for debug
    Serial.println(threshold2);
    Serial.println(inputMessage2);

    // if request has input parameter 5
    if (request->hasParam(PARAM_INPUT_5)) {
      // save this parameter into variable threshold3
      threshold3 = request->getParam(PARAM_INPUT_5)->value();
      // if request has input parameter 6
      if (request->hasParam(PARAM_INPUT_6)) {
        // save this parameter into variable inputMessage3
        inputMessage3 = request->getParam(PARAM_INPUT_6)->value();
        //change checkbox 3 status to checked
        enableArmChecked3 = "checked";
      }
      //else checkbox 3 is not checked set string as false
      else {
        inputMessage3 = "false";
        enableArmChecked3 = "";
      }
    }
    //serial print for debug
    Serial.println(threshold3);
    Serial.println(inputMessage3);

    //after clicking the "submit" button display a page saying the req was sent succesfully
    request->send(200, "text/html", "HTTP GET request sent to your ESP.<br><a href=\"/\">Return to Home Page</a>");
  });
  
  server.onNotFound(notFound); //set up a handler function when url is not found    

  //Set up the event source on the server 
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  server.begin(); //start server
}
 
void loop() {
  //Serial.println (volts1);
  //create an event every 10 secs
  static unsigned long lastEventTime = millis();
  static const unsigned long EVENT_INTERVAL_MS = 10000;
  if ((millis() - lastEventTime) > EVENT_INTERVAL_MS) {
    events.send("ping",NULL,millis());
    lastEventTime = millis();
  }
   unsigned long currentMillis = millis();
   //every 10 seconds send the outgoing data of threshold and checkbox status to each sensor device
  if (currentMillis - previousMillis >= interval) {
    // Save the last time a new reading was published
    previousMillis = currentMillis;
    myData.thres = threshold1.toFloat(); //myData.thres is the threshold 1 value that's being sent to sensor device 1
    myData.checkbox = inputMessage1; //myData.checkbox is the checkbox 1 status that's being sent to sensor device 1
    myData.thres2 = threshold2.toFloat(); //myData.thres is the threshold 2 value that's being sent to sensor device 2
    myData.checkbox2 = inputMessage2; //myData.checkbox is the checkbox 2 status that's being sent to sensor device 1
    myData.thres3 = threshold3.toFloat(); //myData.thres is the threshold 3 value that's being sent to sensor device 2
    myData.checkbox3 = inputMessage3; //myData.checkbox is the checkbox 3 status that's being sent to sensor device 1

    //send the message structure via ESP-NOW to sensor device 1
    esp_err_t result1 = esp_now_send(broadcastAddress1, (uint8_t *) &myData, sizeof(myData));
    if (result1 == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");}

    //send the message structure via ESP-NOW to sensor device 2
    esp_err_t result2 = esp_now_send(broadcastAddress2, (uint8_t *) &myData, sizeof(myData));
    if (result2 == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");}
  
    //send the message structure via ESP-NOW to sensor device 3
    esp_err_t result3 = esp_now_send(broadcastAddress3, (uint8_t *) &myData, sizeof(myData));
    if (result3 == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");} 
    }
}
