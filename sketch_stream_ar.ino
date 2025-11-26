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

#if !defined(RAW_BUFFER_LENGTH)
#define RAW_BUFFER_LENGTH 1024
#endif

void processData(AsyncResult &aResult);

void setup_wifi();
void setup_ssl();
void setup_stream_ssl();
void setup_firebase();

int getTemperaturaDHT();
String getTimeDate();

void sendCodeAr(int temperatura);
void sendRegisterAr(bool estado, int temperatura);

FirebaseApp app;
WiFiClientSecure ssl_client, stream_ssl_client; 
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
AsyncClient streamClient(stream_ssl_client); 
RealtimeDatabase Database;

UserAuth user_auth(FIREBASE_API_KEY, FIREBASE_USER_EMAIL, FIREBASE_USER_PASSWORD);

DHT_Unified dht(DHT_PIN, DHTTYPE);

bool estado = false;
unsigned long ultima_coleta = 0;

void setup() {
  Serial.begin(115200); // Taxa de Baud padrão
  Serial.println("Setup: Inicializando...");

  setup_wifi();
  setup_ssl(); 
  setup_stream_ssl();
  setup_firebase();
  dht.begin();

  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

  IrSender.begin(LED_PIN, false); 

  Serial.println("Aguardando autenticação...");
  while (!app.ready()) {
      app.loop();
      delay(100);
  }
  Serial.println("Firebase pronto!");

  // Usando Database.get no modo SSE (true) para iniciar o Stream.
  // processData é usado para receber tanto os erros/eventos QUANTO os dados de Stream.
  Database.get(streamClient, PATH_CONTROLE, processData, true /* SSE mode */, "streamControlTask");
}

void loop() {
  app.loop();
  delay(10);
  int temp = getTemperaturaDHT();

  if (app.ready()) {
    int temp = getTemperaturaDHT();

    // Só processa se a leitura for válida (> -100, por exemplo)
    if (temp > -100) {
      if (millis() - ultima_coleta > DHT_READ_DELAY){ 
            Serial.printf("Telemetria periódica: %d°C\n", temp);
            sendRegisterAr(estado, temp);
            ultima_coleta = millis();
      }
      else if (temp != Database.get<int>(aClient, PATH_TEMPERATURA)){
        sendRegisterAr(estado, temp);
      }
    }   
  }  
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
        if((strcmp(stream.dataPath().c_str(),"/ar/temperatura_flag") == 0) && (stream.to<int>() == 1)){        
          int temperatura = Database.get<int>(aClient, PATH_TEMPERATURA) - 16;
          sendCodeAr(temperatura);
          sendRegisterAr(estado, temperatura);
        } 
        else if (strcmp(stream.dataPath().c_str(),"/ar/estado") == 0) {
          bool novo_estado = stream.to<bool>();
          if (novo_estado == false && estado == true) {
            sendCodeArOFF();
            sendRegisterAr(estado, getTemperaturaDHT());     
          }   
          else if (novo_estado == true && estado == false) {
            sendCodeAr(22);
            sendRegisterAr(estado, 22);
          }
        }
      }
      
      //Database.set<int>(aClient, PATH_LUZ_ESTADO, digitalRead(LED_PIN));
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

int getTemperaturaDHT() {
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    // Retorna valor sentinela de erro
    Serial.println("Erro ao capturar temperatura.");
    return -999; 
  }
  return (int)event.temperature;
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

void sendRegisterAr(bool estado, int temperatura) {
  object_t jsonData, dataHoraObj, estadoObj, temperaturaObj;
  JsonWriter writer;

  if (temperatura = -999){temperatura = 0;}

  writer.create(dataHoraObj, "dataHora", string_t(getTimeDate()));
  writer.create(estadoObj, "estado", estado ? 1 : 0);
  writer.create(temperaturaObj, "temperatura", temperatura);
  writer.join(jsonData, 3, dataHoraObj, estadoObj, temperatura);

  // Imprime o JSON que será enviado para fins de depuração.
  Serial.print("Enviando objeto JSON para: ");
  Serial.println(PATH_AR_REGISTROS);
  Serial.println(jsonData);

  if (estado != Database.get<bool>(aClient, PATH_AR_ESTADO)){
    Database.set<int>(aClient, PATH_AR_ESTADO, estado);
  }
  
  Database.push<object_t>(aClient, PATH_AR_REGISTROS, jsonData, processData, "pushRegistroTask");
}

void sendCodeAr(int temperatura){
  if (!estado){estado = true;}
  
  const uint8_t* dados = arr_temperaturas_on[temperatura];
  uint16_t tamanho = arr_temperaturas_tamanhos[temperatura];

  IrSender.sendRaw(dados, tamanho, 38);
}

void sendCodeArOFF() {
  if (estado){estado = false;}

  IrSender.sendRaw(desligar, desligar_LEN, 38);
}
