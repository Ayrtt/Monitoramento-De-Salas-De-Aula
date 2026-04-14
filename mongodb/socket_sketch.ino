#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>

#include <string.h>
#include "time.h"

#include <DHT.h>
#include <DHT_U.h>
#include <IRremote.hpp>

#include "config.h"
#include "ac_codes.h" 

SocketIOclient socketIO;

WiFiClient client;
HTTPClient http;

SemaphoreHandle_t pirSemaphore;
DHT_Unified dht(DHT_PIN, DHTTYPE);

//bool ac_status = false; 
bool temp_flag = false;
unsigned long last_capture = 0;
int last_temperature = 0;
int target_temperature = 0;
int current_temperature = 0;

String getCurrentTime();
void setup_wifi();
bool wifiConnection();
void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length);

void pir_manager_task(void *pvParameter);
void IRAM_ATTR sensor_isr_handler();
void sendRegisterPIR(bool presence);
void updateRelay(bool presence);

int getTemperatureDHT();
void sendRegisterTemperature(int temperature);
void updateTemperature(int temperature);
void sendCodeAC(int temperature);
void sendCodeACOff();
void sendConvertedRaw(const uint16_t* data_ticks, size_t size);

void sendRegisterPIR(bool presence) {
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();

  // Identifies which API route will receive this 'event'. Needs to be the first field of the json
  array.add("register_presence");

  JsonObject param1 = array.createNestedObject();
  param1["device_id"] = deviceID; //const char* deviceID = "0001";
  param1["event"] = presence ? "on" : "off";
  param1["timestamp"] = getCurrentTime();

  String output;
  serializeJson(doc, output);
  socketIO.sendEVENT(output);

  Serial.print("Enviado: ");
  Serial.println(output);
}

void updateRelay(bool presence) {
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();

  array.add("update_relay");

  JsonObject param1 = array.createNestedObject();
  param1["device_id"] = deviceID;
  param1["relay_status"] = presence;

  String output;
  serializeJson(doc, output);
  socketIO.sendEVENT(output);

  Serial.print("Status Relay: ");
  Serial.println(output);
}

void sendRegisterTemperature(int temperature) {
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();

  array.add("register_temperature");

  JsonObject param1 = array.createNestedObject();
  param1["device_id"] = deviceID; //const char* deviceID = "0001";
  param1["temperature"] = temperature;
  param1["timestamp"] = getCurrentTime();

  String output;
  serializeJson(doc, output);
  socketIO.sendEVENT(output);

  Serial.print("Enviado: ");
  Serial.println(output);
}

void updateTemperature(int temperature) {
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();

  array.add("update_temperature");

  JsonObject param1 = array.createNestedObject();
  param1["device_id"] = deviceID;
  param1["temperature"] = temperature;

  String output;
  serializeJson(doc, output);
  socketIO.sendEVENT(output);

  Serial.print("Status Temperature: ");
  Serial.println(output);
}

void setup() {
  Serial.begin(115200);

  Serial.println("\nConecting to Wi-Fi");
  setup_wifi();

  pinMode(RELAY_PIN, OUTPUT); 
  pinMode(PIR_PIN, INPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);

  pirSemaphore = xSemaphoreCreateBinary();
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), sensor_isr_handler, RISING);
  xTaskCreate(&pir_manager_task, "pir_manager_task", 12288, NULL, 5, NULL);

  dht.begin();

  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  Serial.println("\nTime synchronized via NTP!");

  socketIO.begin(serverName, serverPort, "/socket.io/?EIO=4");
  socketIO.onEvent(socketIOEvent);
}

void loop() {
  socketIO.loop(); // Required to keep the socket open. Never use delay() inside loop.

  if (millis() - last_capture > DHT_READ_DELAY) {
    current_temperature = getTemperatureDHT();
    Serial.printf("[DHT Capture] Temperature: %d°C \n", current_temperature);
    if (last_temperature != current_temperature){      
      sendRegisterTemperature(current_temperature);
      updateTemperature(current_temperature);
      last_temperature = current_temperature;
    }
    
    last_capture = millis();
  }
}

// Required function to keep the library Socket.IO working internally
void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case sIOtype_DISCONNECT:
      Serial.println("[WebSocket] Disconnected!");
      break;
    case sIOtype_CONNECT:
      Serial.printf("[WebSocket] Connected! URL: %s\n", payload);
      socketIO.send(sIOtype_CONNECT, "/");
      break;
    case sIOtype_EVENT:
    {
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.print("[WebSocket] JSON Reading Error: ");
        Serial.println(error.c_str());
        return;
      }

      String eventName = doc[0].as<String>();

      if (eventName == "command_target_temperature"){
        JsonObject data = doc[1];
        String id_received = data["device_id"].as<String>();

        if (id_received == deviceID) {
          target_temperature = data["target_temperature"].as<int>();
          sendCodeAC(target_temperature);
        }
      }
      else if (eventName == "command_ac"){
        JsonObject data = doc[1];
        String id_received = data["device_id"].as<String>();

        if (id_received == deviceID) {
          sendCodeACOff(target_temperature);
        }
      }
      else if (eventName == "command_relay"){

      }
      else {
        Serial.printf("[WebSocket] Resposta do Servidor: %s\n", payload);
      }
      break;
    }
  }
}

void setup_wifi(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
  }
}

bool wifiConnection(){
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not conected.");
    return false;
  }
  else {return true;}
}

void pir_manager_task(void *pvParameter) {
  unsigned long lastActivityTime = 0;
  bool isPresenceActive = false;

  while (1) {
    if (xSemaphoreTake(pirSemaphore, 0) == pdTRUE) {
      lastActivityTime = millis(); 

      if (!isPresenceActive) {
        isPresenceActive = true;
        Serial.println("[PIR Manager] Moviment detected! Turning light on.");
        digitalWrite(RELAY_PIN, RELAY_ON);
        sendRegisterPIR(isPresenceActive);
        updateRelay(isPresenceActive);
      }
    }

    if (isPresenceActive && (millis() - lastActivityTime > INACTIVITY_TIMEOUT_MS)) {
      isPresenceActive = false;
      Serial.println("[PIR Manager] Inactivity. Turning light off.");
      digitalWrite(RELAY_PIN, RELAY_OFF);
      sendRegisterPIR(isPresenceActive);
      updateRelay(isPresenceActive);
    }

    vTaskDelay(pdMS_TO_TICKS(50)); 
  }
}

void IRAM_ATTR sensor_isr_handler() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(pirSemaphore, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

String getCurrentTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Error obtaining time");
    return "Time Unavailable";
  }
  char timeStringBuff[50];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(timeStringBuff);
}

int getTemperatureDHT() {
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) return -999; 
  return (int)event.temperature;
}

void sendCodeAC(int temperature){
  Serial.printf("[IR] Sending %d°C signal", temperature);

  int index = temperature - 16;

  if (index >= 0 && index < 16) {
    const uint16_t* signal_data = array_ac_signals_on[index].rawData;
    size_t signal_size = array_ac_signals_on[index].length;

    if (signal_size > 0) {
      sendConvertedRaw(signal_data, signal_size);
      Serial.println("[IR] Code sent.");
    } else {
      Serial.println("[WARNING IR] Empty code!");
    }
  } else {
    Serial.printf("[ERROR IR] Temperature %d outside of range.\n", temperature);
  }
}

void sendCodeACOff() {
  Serial.println("[IR] Turning AC off");

  const uint16_t* signal_data = array_ac_signal_off.rawData;
  size_t signal_size = array_ac_signal_off.length;

  if (signal_size > 0) {
    sendConvertedRaw(signal_data, signal_size);
    Serial.println("[IR] OFF Code sent.");
  } else {
    Serial.println("[WARNING IR] Empty OFF code");
  }
}

void sendConvertedRaw(const uint16_t* data_ticks, size_t size) {
  // Temporary buffer stores microseconds data (If too large, stack can burst)

  uint16_t* data_micros = (uint16_t*)malloc(size * sizeof(uint16_t));
  if (data_micros == NULL) {
    Serial.println("[ERROR IR] Memory allocation error!");
    return;
  }

  // Conversion: Value * 50 (standard conversion factor for small raw codes)
  for (size_t i = 0; i < size; i++) {
    data_micros[i] = data_ticks[i] * 50;
  }

  for(int i = 0; i<3; i++){
    Serial.println("[IR] Sending signal...");
    IrSender.sendRaw(data_micros, size, 38);
    delay(1000);
  }

  free(data_micros);
}
