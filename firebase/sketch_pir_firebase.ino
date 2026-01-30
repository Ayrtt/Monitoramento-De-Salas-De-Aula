#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <FirebaseJson.h>
#include "time.h"

#include "config.h"

SemaphoreHandle_t pirSemaphore;

// Firebase components
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;

UserAuth user_auth(FIREBASE_API_KEY, FIREBASE_USER_EMAIL, FIREBASE_USER_PASSWORD);

FirebaseJson jsonData;

void processData(AsyncResult &aResult);

void pir_manager_task(void *pvParameter) {
  unsigned long lastActivityTime = 0;
  bool isPresenceActive = false;

  while (1) {
    // Tenta "pegar" o semáforo. O timeout de 0 significa que não vai esperar.
    // Se conseguir (retornar pdTRUE), significa que a ISR foi acionada.
    if (xSemaphoreTake(pirSemaphore, 0) == pdTRUE) {
      // Movimento detectado pela ISR!
      lastActivityTime = millis(); // Atualiza o cronômetro

      if (!isPresenceActive) {
        isPresenceActive = true;
        Serial.println("Sensor ativado");
        sendRegisterPIR(true);
      }
    }

    // A lógica de timeout continua a mesma, usando millis()
    if (isPresenceActive && (millis() - lastActivityTime > INACTIVITY_TIMEOUT_MS)) {
      isPresenceActive = false;
      Serial.println("Sensor desativado");
      sendRegisterPIR(false);
    }

    vTaskDelay(pdMS_TO_TICKS(50)); // Delay da task
  }
}

void IRAM_ATTR sensor_isr_handler() {
  xSemaphoreGiveFromISR(pirSemaphore, NULL);
}

void setup() {
  Serial.begin(9600);
  Serial.println("Setup.");

  pirSemaphore = xSemaphoreCreateBinary();

  pinMode(PIR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), sensor_isr_handler, RISING);

  xTaskCreate(&pir_manager_task, "pir_manager_task", 4096, NULL, 5, NULL);

  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

  // Connect to Wi-Fi
  setup_wifi();
  Serial.println();
  
  // Configure SSL client
  setup_ssl();
  
  // Initialize Firebase
  setup_firebase();
}

void loop() {
  // Maintain authentication and async tasks
  app.loop();
  // Check if authentication is ready
  if (app.ready()){ 
  }
}

// Função para log dos resultados das operações assíncronas do Firebase
void processData(AsyncResult &aResult) {
  if (!aResult.isResult())
    return;

  if (aResult.isEvent())
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());

  if (aResult.isDebug())
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());

  if (aResult.isError())
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());

  if (aResult.available())
    Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
}

void setup_wifi(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
}

void setup_ssl(){
  ssl_client.setInsecure();
  ssl_client.setConnectionTimeout(1000);
  ssl_client.setHandshakeTimeout(5);
}

void setup_firebase(){
  initializeApp(aClient, app, getAuth(user_auth), processData, "🔐 authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(FIREBASE_DATABASE_URL);
}

String getTimeDate() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Falha ao obter hora do NTP");
    return "00/00/0000, 00:00:00";
  }
  char timeStringBuff[50];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%d/%m/%Y, %H:%M:%S", &timeinfo);
  return String(timeStringBuff);
}

void sendRegisterPIR(bool presenca) {
  object_t jsonData, dataHoraObj, estadoObj;
  JsonWriter writer;

  writer.create(dataHoraObj, "dataHora", string_t(getTimeDate()));
  writer.create(estadoObj, "estado", presenca ? 1 : 0);

  writer.join(jsonData, 2, dataHoraObj, estadoObj);

  // Imprime o JSON que será enviado para fins de depuração.
  Serial.print("Enviando objeto JSON para: ");
  Serial.println(PATH_LUZ_REGISTROS);
  Serial.println(jsonData);

  Database.push<object_t>(aClient, PATH_LUZ_REGISTROS, jsonData, processData, "pushRegistroTask");
}
