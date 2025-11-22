#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h> 
#include <FirebaseJson.h>
#include "time.h"

#include "config.h" // Config.h utilizado para esse código está comentado a partir da linha 108

void processData(AsyncResult &aResult);
void setup_wifi();
void setup_ssl();
void setup_stream_ssl();
void setup_firebase();

FirebaseApp app;
WiFiClientSecure ssl_client, stream_ssl_client; 
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
AsyncClient streamClient(stream_ssl_client); 
RealtimeDatabase Database;

UserAuth user_auth(FIREBASE_API_KEY, FIREBASE_USER_EMAIL, FIREBASE_USER_PASSWORD);

void setup() {
  Serial.begin(115200); // Taxa de Baud padrão
  Serial.println("Setup: Inicializando...");

  setup_wifi();
  setup_ssl(); 
  setup_stream_ssl();
  setup_firebase();
  
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
          Serial.println("=========================================");
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

/* config h

#ifndef CONFIG_H
#define CONFIG_H

// --- Configurações de Rede ---
#define WIFI_SSID "brisa-644244"
#define WIFI_PASSWORD "0ybojsh2"

// --- Configurações do Firebase ---
#define FIREBASE_API_KEY "AIzaSyB6VS7sqzxv84g-WTGk0zxmrZuP6xv9pow"
#define FIREBASE_DATABASE_URL "https://monitoramento-de-dispositivos-default-rtdb.firebaseio.com"
#define FIREBASE_USER_EMAIL "ayrtton.lucas@academico.ifpb.edu.br"
#define FIREBASE_USER_PASSWORD "teste1"

// ID do dispositivo para facilitar a troca no futuro
#define DEVICE_ID "0001"

// Caminhos para os registros históricos
#define PATH_CONTROLE "/Dispositivos/" DEVICE_ID

// Caminhos específicos para o controle de Temperatura
#define PATH_TEMPERATURA_FLAG PATH_CONTROLE "/temperatura_flag"
#define PATH_TEMPERATURA PATH_CONTROLE "/temperatura"

#endif
*/
