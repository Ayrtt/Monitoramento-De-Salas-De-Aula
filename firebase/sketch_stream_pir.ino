#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h> 
#include <FirebaseJson.h>
#include <string.h>
#include "time.h"

#include "config.h"

void pir_manager_task(void *pvParameter);
void processData(AsyncResult &aResult);
void setup_wifi();
void setup_ssl();
void setup_stream_ssl();
void setup_firebase();
String getTimeDate();
void sendRegisterPIR(bool presenca);

FirebaseApp app;
WiFiClientSecure ssl_client, stream_ssl_client; 
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
AsyncClient streamClient(stream_ssl_client); 
RealtimeDatabase Database;

UserAuth user_auth(FIREBASE_API_KEY, FIREBASE_USER_EMAIL, FIREBASE_USER_PASSWORD);

SemaphoreHandle_t pirSemaphore;

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
  Serial.begin(115200); // Taxa de Baud padrão
  Serial.println("Setup: Inicializando...");

  setup_wifi();
  setup_ssl(); 
  setup_stream_ssl();
  setup_firebase();

  pirSemaphore = xSemaphoreCreateBinary();
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), sensor_isr_handler, RISING);
  xTaskCreate(&pir_manager_task, "pir_manager_task", 4096, NULL, 5, NULL);
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);

  // Usando Database.get no modo SSE (true) para iniciar o Stream.
  // processData é usado para receber tanto os erros/eventos QUANTO os dados de Stream.
  Database.get(streamClient, PATH_CONTROLE, processData, true /* SSE mode */, "streamControlTask");
}

void loop() {
  app.loop();
  delay(10);
}

void processData(AsyncResult &aResult) {
  if (!aResult.isResult())
    return;
    
  if (aResult.isEvent())
    Firebase.printf("[STATUS] Task: %s, Evento: %s, Code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());

  if (aResult.isError())
    Firebase.printf("[FIREBASE ERRO] Task: %s, Msg: %s, Code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
    
  // Lógica de Impressão de DADOS (Payload)
  if (aResult.available()) {
    // Converte o resultado assíncrono para o formato de Resultado do RealtimeDatabase
    RealtimeDatabaseResult &stream = aResult.to<RealtimeDatabaseResult>();
      
    if (stream.isStream()) {
      Serial.println("=========================================");
      Firebase.printf("[STREAM EVENTO] Evento: %s\n", stream.event().c_str());
      Firebase.printf("[STREAM DADOS] Caminho: %s\n", stream.dataPath().c_str());
      Firebase.printf("[STREAM DADOS] Payload: %s\n", stream.to<const char *>());

      if((strcmp(stream.event().c_str(), "put") == 0)) {
        if (strcmp(stream.dataPath().c_str(),"/luz/estado") == 0) {
          bool novo_estado = stream.to<bool>();
          if (novo_estado != digitalRead(RELAY_PIN)) {
            sendRegisterPIR(novo_estado);
          }
        }
      }
    }
    // Caso contrário, apenas imprime o payload (pode ser o resultado de um GET avulso ou SET)
    else if (aResult.uid() != "streamControlTask") {
      Firebase.printf("[PAYLOAD] Task: %s, Payload: %s\n", aResult.uid().c_str(), aResult.c_str());
    }
  }
}

void setup_wifi(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nConectado!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setup_ssl(){
  ssl_client.setInsecure();
  ssl_client.setConnectionTimeout(1000);
  ssl_client.setHandshakeTimeout(5);
}

void setup_stream_ssl(){
  stream_ssl_client.setInsecure();
  stream_ssl_client.setConnectionTimeout(1000);
  stream_ssl_client.setHandshakeTimeout(5);
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

  digitalWrite(RELAY_PIN, presenca ? 1 : 0);

  writer.join(jsonData, 2, dataHoraObj, estadoObj);

  // Imprime o JSON que será enviado para fins de depuração.
  Serial.print("Enviando objeto JSON para: ");
  Serial.println(PATH_LUZ_REGISTROS);
  Serial.println(jsonData);

  Database.set<int>(aClient, PATH_LUZ_ESTADO, presenca);
  Database.push<object_t>(aClient, PATH_LUZ_REGISTROS, jsonData, processData, "pushRegistroTask");
}
