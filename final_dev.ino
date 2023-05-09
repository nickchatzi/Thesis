#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Pangodream_18650_CL.h>
#include "ESPAsyncWebServer.h"
#include <Arduino_JSON.h>
#include <driver/gpio.h>
#include <esp_sleep.h>

// Set your Board ID (ESP32 Sender #1 = BOARD_ID 1, ESP32 Sender #2 = BOARD_ID 2, etc)
#define BOARD_ID 1

// Digital pin connected to the DHT sensor
#define DHTPIN 27  

#define DHTTYPE    DHT22     // DHT 22 (AM2302)

DHT dht(DHTPIN, DHTTYPE);

#define ADC_PIN 34
#define CONV_FACTOR 1.7
#define READS 20   

Pangodream_18650_CL BL(ADC_PIN, CONV_FACTOR, READS);

const gpio_num_t DOOR_SENSOR_PIN = GPIO_NUM_33;

bool triggerActive = false;
int doorState;

//MAC Address of the receiver 
uint8_t broadcastAddress[] = {0xE8, 0x31, 0xCD, 0x02, 0xC9, 0x0C};

typedef struct struct_message {
  int id;
  float tempc;
  float tempf;
  float hum;
  float volts;
  float perc;
  int drst;
  int readingId;
} struct_message;

typedef struct struct_message2 {
  float thres;
  float thres2;
  float thres3;
  String checkbox;
  String checkbox2;
  String checkbox3;
} struct_message2;

//Create a struct_message called myData
struct_message myData;
struct_message2 incomingReadings;

JSONVar board;

AsyncEventSource events("/events");

unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 10000;        // Interval at which to publish sensor readings

void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) { 
  // Copies the sender mac address to a string
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));

  board["threshold"] = incomingReadings.thres;
  board["checkbox"] = incomingReadings.checkbox;
  board["threshold2"] = incomingReadings.thres2;
  board["checkbox2"] = incomingReadings.checkbox2;
  board["threshold3"] = incomingReadings.thres3;
  board["checkbox3"] = incomingReadings.checkbox3;
  String jsonString = JSON.stringify(board);
  events.send(jsonString.c_str(), "new_readings", millis());
}

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



float readDHTTemperature() {                                    //string to read temperature in celcius
  float tc = dht.readTemperature();                               //define float t as the temperature in celcius
  if (isnan(tc)) {                                                //Check if any reads failed and exit early
    Serial.println("Failed to read from DHT sensor!");           // if failed try again and display --
    return 0;    }                                                 
  else {
    Serial.println(tc);                                           //else display the temperature in serial monitor
    return tc;
  }
}

float readDHTTemperatureF() {                                  //string to read temperature in fahrenheit  
  float tf= dht.readTemperature(true);                           //define float t as the temperature in fahrenheit
  if (isnan(tf)) {                                               //Check if any reads failed and exit early
    Serial.println("Failed to read from DHT sensor!");          // if failed try again and display --
    return 0;
  }
  else {
    Serial.println(tf);                                          //else display the temperature in serial monitor
    return tf;
  }
}

float readDHTHumidity() {
  //Read humidity
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    return 0;
  }
  else {
    Serial.println(h);
    return h;
  }
}

float readBatteryVolts(){
  float v = BL.getBatteryVolts();
  if (isnan(v)){
    Serial.println("Failed to read battery volts");
    return 0;
  }    
  else {
    Serial.println(v);
    return v;
    }  
  }

float readBatterypercentage(){
  float p = BL.getBatteryChargeLevel();
  if (isnan(p)){
    Serial.println("Failed to read battery percentage");
    return 0 ;
  }
  else {
    Serial.println(p);
    return p;
    }  
  }  


// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");}
 
void setup() {
  //Init Serial Monitor
  Serial.begin(115200);

  dht.begin();  

  pinMode(26, OUTPUT);
  digitalWrite(26, HIGH);
  pinMode(25, OUTPUT);
  digitalWrite(25, LOW);

  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);

  // Set device as a Wi-Fi Station and set channel
  WiFi.mode(WIFI_STA);

  int32_t channel = getWiFiChannel(WIFI_SSID);

  esp_wifi_set_promiscuous(true); //Set the ESP32's WiFi interface to promiscuous mode
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE); //change the channel on which the ESP32 is listening for WiFi traffic.
  esp_wifi_set_promiscuous(false); //Disable promiscuous mode
  WiFi.printDiag(Serial); // verify channel change

  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  
  //Register peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.encrypt = false;
  
  //Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  // Route for root / web page
  //esp_sleep_enable_timer_wakeup(4 * 1000000); // wake up after 30 seconds
}
 
void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    //Serial.println (incomingReadings.thres2);
    //Serial.println (incomingReadings.checkbox2);
    // Save the last time a new reading was published
    previousMillis = currentMillis;
    //Set values to send
    myData.id = BOARD_ID;
    myData.tempc = readDHTTemperature();
    myData.tempf = readDHTTemperatureF();
    myData.hum = readDHTHumidity();
    myData.volts = readBatteryVolts();
    myData.perc = readBatterypercentage();
    myData.drst = digitalRead(DOOR_SENSOR_PIN);

    //Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");}
    
  
  float temperature = dht.readTemperature();;  // read temperature in Celsius  

  doorState = digitalRead(DOOR_SENSOR_PIN); // read state

  if (doorState == LOW) { 
    Serial.println("The door is closed!");
    if (isnan (temperature)) {
      Serial. println ("Failed to read from DHT sensor!"); }
      else{
      if (temperature > incomingReadings.thres && incomingReadings.checkbox == "true" && !triggerActive){
        Serial.println ("Turn the freezer unit on");
        triggerActive = true;
        digitalWrite (26, LOW);  //turn on
        digitalWrite (25,HIGH);
        delay(10);
        digitalWrite (25,LOW);
        }   
      else if (temperature < incomingReadings.thres && incomingReadings.checkbox == "true" && triggerActive) {
         Serial.println ("Turn the freezer unit off");
         triggerActive = false;
         digitalWrite (26, HIGH);
          delay(10);
          digitalWrite (26,LOW);
         digitalWrite (25,LOW);}}}//turn off

  if (doorState == HIGH) {
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, HIGH);
    Serial.println("The door is open. Turn the freezer unit off");
    triggerActive = false;    
    digitalWrite(26, HIGH);
    delay(10);
    digitalWrite(26, LOW);
    digitalWrite(25, LOW);    
    } 
}
 
  }