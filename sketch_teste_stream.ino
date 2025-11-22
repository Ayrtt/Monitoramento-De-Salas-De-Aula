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

  pinMode(LED_PIN, OUTPUT);

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
          
      if((strcmp(stream.dataPath().c_str(),"/ar/temperatura_flag") == 0) && (stream.to<int>() == 1)){
        
        Serial.println(Database.get<int>(aClient, PATH_TEMPERATURA));
        
        
      } 
      else if (strcmp(stream.dataPath().c_str(),"/ar/estado") == 0) {
        Serial.println("ar_estado");
      }
      else if (strcmp(stream.dataPath().c_str(),"/ar/estado") == 1) {
        Serial.println("ar_estado");
      }
      else if (strcmp(stream.dataPath().c_str(),"/luz/estado") == 0) {
        digitalWrite(LED_PIN, HIGH);
      }
      else if (strcmp(stream.dataPath().c_str(),"/luz/estado") == 1) {
        digitalWrite(LED_PIN, LOW);
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
