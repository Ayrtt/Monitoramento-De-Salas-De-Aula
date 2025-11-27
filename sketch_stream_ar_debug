#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h> 
#include <FirebaseJson.h>
#include <string.h>
#include "time.h"

#include <DHT.h>
#include <DHT_U.h>
#include <IRremote.hpp>

#include "config.h"
#include "codigos_ar.h" 

#if !defined(RAW_BUFFER_LENGTH)
#define RAW_BUFFER_LENGTH 1024
#endif

// --- Protótipos ---
void processData(AsyncResult &aResult);
void setup_wifi();
void setup_ssl();
void setup_stream_ssl();
void setup_firebase();
int getTemperaturaDHT();
String getTimeDate();
void sendCodeAr(int temperatura);
void sendCodeArOFF();
void sendRegisterAr(bool estado, int temperatura);

// --- Objetos Globais ---
FirebaseApp app;
WiFiClientSecure ssl_client, stream_ssl_client; 
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
AsyncClient streamClient(stream_ssl_client); 
RealtimeDatabase Database;

UserAuth user_auth(FIREBASE_API_KEY, FIREBASE_USER_EMAIL, FIREBASE_USER_PASSWORD);

DHT_Unified dht(DHT_PIN, DHTTYPE);

// --- Variáveis Globais ---
bool estado = false; 
unsigned long ultima_coleta = 0;
int temperatura_alvo = 24; // Valor padrão

void setup() {
  Serial.begin(115200); 
  Serial.println("Setup: Inicializando...");

  setup_wifi();
  
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

  setup_ssl(); 
  setup_stream_ssl();
  setup_firebase();
  dht.begin();

  // Inicializa o IR no pino LED_PIN (27)
  IrSender.begin(LED_PIN); 

  Serial.println("Aguardando autenticação...");
  while (!app.ready()) {
      app.loop();
      delay(100);
  }
  Serial.println("Firebase pronto!");

  // Inicia o Stream. O estado inicial virá automaticamente.
  Database.get(streamClient, PATH_CONTROLE, processData, true /* SSE mode */, "streamControlTask");
}

void loop() {
  app.loop();
  
  if (app.ready()) {
      if (millis() - ultima_coleta > DHT_READ_DELAY) {
          int temp = getTemperaturaDHT();

          if (temp > -100) {
              Serial.printf("Telemetria: %d°C (Estado AR: %s)\n", temp, estado ? "ON" : "OFF");
              sendRegisterAr(estado, temp);
              ultima_coleta = millis();
          }
      }
  }
  
  delay(10);
}

void processData(AsyncResult &aResult) {
  if (!aResult.isResult()) return;
    
  if (aResult.isEvent())
    Firebase.printf("[STATUS] Task: %s, Evento: %s, Code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());

  if (aResult.isError())
    Firebase.printf("[FIREBASE ERRO] Task: %s, Msg: %s, Code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
    
  if (aResult.available()) {
    RealtimeDatabaseResult &stream = aResult.to<RealtimeDatabaseResult>();
      
    if (stream.isStream()) {
      if (stream.event() == "put" || stream.event() == "patch") {
          
          if (stream.dataPath().startsWith("/registros")) return;

          Serial.println("=========================================");
          Firebase.printf("[STREAM EVENTO] Evento: %s\n", stream.event().c_str());
          Firebase.printf("[STREAM DADOS] Caminho: %s\n", stream.dataPath().c_str());
          
          // Sincroniza temperatura alvo
          if (stream.dataPath().equals("/ar/temperatura")) {
              temperatura_alvo = stream.to<int>();
              Serial.printf("[SYNC] Temperatura alvo: %d\n", temperatura_alvo);
          }
          
          // Lógica de Flag de Temperatura
          else if(stream.dataPath().equals("/ar/temperatura_flag") && stream.to<bool>() == true){        
            Serial.println("[FLAG] Comando de Temperatura recebido.");
            
            // Dispara GET assíncrono para garantir o valor mais recente
            Database.get(aClient, PATH_TEMPERATURA, processData, false, "getTempTask");
            
            // Usa o valor atual enquanto o GET não chega (para resposta rápida)
            if (temperatura_alvo >= 16 && temperatura_alvo <= 31) {
                sendCodeAr(temperatura_alvo);
                sendRegisterAr(true, temperatura_alvo);
            }
            Database.set<bool>(aClient, PATH_TEMPERATURA_FLAG, false);
          } 
          
          // Lógica de Estado ON/OFF
          else if (stream.dataPath().equals("/ar/estado")) {
            bool novo_estado = stream.to<bool>();
            Serial.printf("[STREAM] Novo estado: %s\n", novo_estado ? "ON" : "OFF");
            
            if (novo_estado != estado) {
                if (novo_estado) {
                    sendCodeAr(temperatura_alvo);
                } else {
                    sendCodeArOFF();
                }
                estado = novo_estado;
                // O registro será enviado no próximo ciclo de telemetria ou aqui se preferir
            }
          }
      }
    }
    // Callback do GET assíncrono de temperatura
    else if (aResult.uid() == "getTempTask") {
        RealtimeDatabaseResult &result = aResult.to<RealtimeDatabaseResult>();
        int t = result.to<int>();
        Serial.printf("[GET TASK] Temperatura confirmada: %d\n", t);
        
        // Se for diferente da que usamos, corrige
        if (t != temperatura_alvo && t >= 16 && t <= 31) {
            temperatura_alvo = t;
            sendCodeAr(temperatura_alvo);
            sendRegisterAr(true, temperatura_alvo);
        }
    }
    else if (aResult.uid() != "streamControlTask") {
      Firebase.printf("[PAYLOAD] Task: %s, Payload: %s\n", aResult.uid().c_str(), aResult.c_str());
    }
  }
}

void setup_wifi(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
  }
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

int getTemperaturaDHT() {
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) return -999; 
  return (int)event.temperature;
}

String getTimeDate() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo, 50)) return "00/00/0000, 00:00:00";
  char timeStringBuff[50];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%d/%m/%Y, %H:%M:%S", &timeinfo);
  return String(timeStringBuff);
}

void sendRegisterAr(bool novo_estado, int temperatura) {
  estado = novo_estado;
  if (temperatura == -999) temperatura = 0;

  FirebaseJson json;
  String dataHoraStr = getTimeDate();
  
  json.set("dataHora", dataHoraStr);
  json.set("estado", estado ? 1 : 0);
  json.set("temperatura", temperatura);

  String jsonString;
  json.toString(jsonString);

  Database.push<object_t>(aClient, PATH_AR_REGISTROS, object_t(jsonString), processData, "pushRegistroTask");
}

void sendCodeAr(int temperatura){
  Serial.printf("[IR] Enviando comando LIGAR/TEMP %d°C no pino %d\n", temperatura, LED_PIN);
  estado = true;
  
  int index = temperatura - 16;
  
  if (index >= 0 && index < 16) {
      const uint16_t* dados = arr_temperaturas_on[index];
      size_t tamanho = arr_temperaturas_tamanhos[index] / sizeof(uint16_t); 
      IrSender.sendRaw(dados, tamanho, 38);
  } else {
      Serial.printf("[ERRO IR] Temperatura %d fora da faixa.\n", temperatura);
  }
}

void sendCodeArOFF() {
  Serial.println("[IR] Enviando comando DESLIGAR");
  estado = false;
  IrSender.sendRaw(desligar, desligar_LEN / sizeof(uint16_t), 38);
}
