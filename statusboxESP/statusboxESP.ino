#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <string.h>
#include "homeassistant.h"

#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <SoftwareSerial.h>


#define STASSID "WIFI"
#define STAPSK  "12345678"

const char* mqtt_server = "10.1.1.1";
const char* mqtt_user = "mqtt";
const char* mqtt_pass = "mqtt";


const char *ssid = STASSID;
const char *password = STAPSK;

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

WiFiClient espClient;
PubSubClient mqtt;
SoftwareSerial BoxSerial(4,5);  // RX und TX


enum spacestatus_open_t {closed = 0, open_internal = 1, open_public = 2, unknown = 254};

typedef struct {
  spacestatus_open_t open_status;
  uint8_t closing_soon_flag;
  uint8_t state_changed;
} spacestatus_t;

spacestatus_t spacestatus;

void setLedMode(uint8_t id, uint8_t interval){
  BoxSerial.write(0xbf);
  BoxSerial.write(id);
  BoxSerial.write(interval);  
}

void setLedModeAll(uint8_t interval){
  for(uint8_t i = 0; i < 5; i++){
    setLedMode(i, interval);  
  }  
}

void mqttcallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
 
  Serial.println();
  Serial.println("-----------------------");

  if(strcmp(topic, "spacestatus/state") == 0){
    
    if(memcmp(payload, "open", 4) == 0){
      Serial.println("mqtt status is open");
      spacestatus.open_status = open_public;
    } else if(memcmp(payload, "internal", 8) == 0){
      Serial.println("mqtt status is internal");
      spacestatus.open_status = open_internal;
    } else if(memcmp(payload, "closed", 6) == 0){
      Serial.println("mqtt status is closed");
      spacestatus.open_status = closed;
    } else {
      Serial.println("mqtt status is unkonwn");
      spacestatus.open_status = unknown;
    }
    spacestatus.state_changed = true;
  }else if(strcmp(topic, "spacestatus/leavingflag") == 0){
    if(memcmp(payload, "on", 2) == 0){
      Serial.println("mqtt leaving flag is on");
      spacestatus.closing_soon_flag = true;
    } else if(memcmp(payload, "off", 3) == 0){
      Serial.println("mqtt leaving flag is off");
      spacestatus.closing_soon_flag = false;  
    }

    spacestatus.state_changed = true;
  } else {
    Serial.print("mqtt unkown topic >");
    Serial.print(topic);
    Serial.println("<");
  }
}

void mqtt_init(){
  mqtt.setClient(espClient);

  Serial.println("Setting MQTT Server");
  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(mqttcallback);
  mqtt.setBufferSize(600);

  mqtt_reconnect();  
}

void mqtt_reconnect() {
  while (!mqtt.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (mqtt.connect("SpaceStatusPanel", mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      mqtt.subscribe("spacestatus/state");
      mqtt.subscribe("spacestatus/leavingflag");

    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }

     mqtt.loop();
  }
}

void connectWifi(void){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  setLedModeAll(0x05);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  setLedModeAll(0x00);

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  BoxSerial.begin(9600);  

  spacestatus.open_status = unknown;
  spacestatus.closing_soon_flag = false;
  spacestatus.state_changed = false;
  
  connectWifi();

  httpUpdater.setup(&httpServer);
  httpServer.begin();

  mqtt_init();

}

void loop() {
  httpServer.handleClient();

  while(WiFi.status() != WL_CONNECTED) {
    connectWifi();
    mqtt_reconnect();
  }


  if(!mqtt.connected()) {
    mqtt_reconnect();
  }
  
  mqtt.loop();

  if (BoxSerial.available() >= 3) {
    uint8_t header = BoxSerial.read();
    if(header == 0xbf){
      uint8_t btnid = BoxSerial.read();
      uint8_t btnlen = BoxSerial.read();
      Serial.print("BTN Pressed: #");
      Serial.print(btnid);
      Serial.print(" - Len: ");
      Serial.println(btnlen);

      String topic = "spacestatus/btnpress/"+String(btnid);
      String value = String(btnlen);
  
      mqtt.publish(topic.c_str(), value.c_str());

      if(btnlen == 1){
        if(btnid == 0){
          spacestatus.closing_soon_flag = false;
          spacestatus.open_status = closed;
        } else if(btnid == 1) {
          spacestatus.open_status = open_internal;
        } else if(btnid == 2) {
          spacestatus.open_status = open_public;
        } else if(btnid == 4) {
          spacestatus.closing_soon_flag = !spacestatus.closing_soon_flag;
        }

        topic = "spacestatus/state";
        switch(spacestatus.open_status){
          case closed: { value = "closed"; break; }  
          case open_internal: { value = "internal"; break; }
          case open_public: {value = "open"; break; }
        }
        mqtt.publish(topic.c_str(), value.c_str());


        topic = "spacestatus/leavingflag";
        value = spacestatus.closing_soon_flag ? "on" : "off";
        mqtt.publish(topic.c_str(), value.c_str());
        
        spacestatus.state_changed = true;
      }
    }
    
  }

  if(spacestatus.state_changed){
    setLedMode(0, 0x00);
    setLedMode(1, 0x00);
    setLedMode(2, 0x00);
    
    if(spacestatus.open_status == closed){
      setLedMode(0, 0xFF);
    } else if(spacestatus.open_status == open_internal){
      setLedMode(1, 0xFF);
    } else if(spacestatus.open_status == open_public){
      setLedMode(2, 0xFF);
    }

    setLedMode(4, 0x00);
    if(spacestatus.closing_soon_flag){
      setLedMode(4, 0xFF);  
    }
    
    spacestatus.state_changed = false;
  }

}
