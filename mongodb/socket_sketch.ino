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
#include "codigos_ar.h" 

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
bool wifiConnection();
void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length);

void pir_manager_task(void *pvParameter);
void IRAM_ATTR sensor_isr_handler();
void sendRegisterPIR(bool presence);
void updateRelay();

int getTemperatureDHT();
void sendRegisterAC(int temperature);
void updateTemperature(int temperature);

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

  // Identifies which API route will receive this 'event'. Needs to be the first field of the json
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

  sendRegisterPIR(1);
  //db_get();
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
      //(AC status: %s), ac_status ? "ON" : "OFF"ac_status,
      last_temperature = current_temperature;
    }
    
    last_capture = millis();
  }
}

// Required function to keep the library Socket.IO working internally
void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case sIOtype_DISCONNECT:
      Serial.println("[WebSocket] Desconectado!");
      break;
    case sIOtype_CONNECT:
      Serial.printf("[WebSocket] Conectado! URL: %s\n", payload);
      socketIO.send(sIOtype_CONNECT, "/"); // Confirma a entrada no canal padrão
      break;
    case sIOtype_EVENT:
    {
      // 1. Desempacota o texto recebido para um formato JSON inteligível
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.print("[WebSocket] Falha ao ler JSON: ");
        Serial.println(error.c_str());
        return;
      }

      // 2. Extrai o nome do evento (posição 0 do array)
      String eventName = doc[0].as<String>();

      // 3. Verifica se é o evento de comando do ar condicionado
      if (eventName == "command_target_temperature") {
        JsonObject data = doc[1]; // Os dados estão na posição 1
        String id_recebido = data["device_id"].as<String>();

        // 4. Confirma se a ordem é realmente para este dispositivo (ESP32 atual)
        if (id_recebido == deviceID) {
          int nova_temperatura = data["target_temperature"].as<int>();
          
          // Salva na variável global que você já havia declarado no topo do código
          target_temperature = nova_temperatura;
          
          Serial.println("=========================================");
          Serial.printf(">>> ORDEM RECEBIDA: Setar AC para %d°C <<<\n", target_temperature);
          Serial.println("=========================================");
        }
      } 
      else {
        // Se for outro evento qualquer, apenas imprime por curiosidade
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
