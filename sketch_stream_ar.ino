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

#define RECORD_GAP_MICROS 100000 //Necessário para enviar os dois pacotes do comando
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

DHT_Unified dht(DHTPIN, DHTTYPE);

bool estado = Database.get<bool>(aClient, PATH_AR_ESTADO);;
unsigned long ultima_coleta = 0;

void setup() {
  Serial.begin(115200); // Taxa de Baud padrão
  Serial.println("Setup: Inicializando...");

  setup_wifi();
  setup_ssl(); 
  setup_stream_ssl();
  setup_firebase();
  dht.begin();

  IrSender.begin(kSendPin, false); 
  // Usando Database.get no modo SSE (true) para iniciar o Stream.
  // processData é usado para receber tanto os erros/eventos QUANTO os dados de Stream.
  Database.get(streamClient, PATH_CONTROLE, processData, true /* SSE mode */, "streamControlTask");
}

void loop() {
  app.loop();
  delay(10);
  int temp = getTemperaturaDHT();

  if (millis() - ultima_coleta > 5000){
    sendRegisterAr(estado, temp);
    ultima_coleta = millis();
  }
  else if (temp != Database.get<int>(aClient, PATH_TEMPERATURA)){
    sendRegisterAr(estado, temp);
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
          sendRegistroAr(estado, temperatura);
        } 
        else if (strcmp(stream.dataPath().c_str(),"/ar/estado") == 0) {
          bool novo_estado = stream.to<bool>();
          if (novo_estado == false && estado == true) {
            desligar();
            sendRegistroAr(estado, temperatura);     
          }   
          else if (novo_estado == true && estado == false) {
            sendCodeAr(22);
            sendRegistroAr(estado, 22);
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
  
  Database.push<object_t>(aClient, PATH_LUZ_REGISTROS, jsonData, processData, "pushRegistroTask");
}

void sendCodeAr(int temperatura){
  if (!estado){estado = true;}
  
  const uint8_t* dados = arr_temperaturas_on[temperatura];
  uint16_t tamanho = arr_temperatura_tamanhos[temperatura];

  IrSender.sendRaw(dados, tamanho, 38);
}

void sendCodeArOFF() {
  if (estado){estado = false;}

  int tamanhoDesligar = sizeof(desligar) /  sizeof(desligar[0];)

  IrSender.sendRaw(desligar, tamanho, 38);
}

/* config.h
// config.h

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
#define DEVICE_ID "/0001"

// Caminho para nó principal
#define PATH_CONTROLE "/Dispositivos" DEVICE_ID

// Caminhos específicos para o controle do ar-condicionado
#define PATH_AR_ESTADO PATH_CONTROLE "/ar/estado"
#define PATH_AR_REGISTROS PATH_CONTROLE "/registros/ar"
#define PATH_TEMPERATURA PATH_CONTROLE "/ar/temperatura"
#define PATH_TEMPERATURA_FLAG PATH_CONTROLE "/ar/temperatura_flag"

// --- Configurações do Hardware (Pinos) ---
#define LED_PIN 27
#define DHT_PIN 14
#define DHTTYPE DHT11

// --- Configurações do Servidor de Tempo (NTP) ---
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC -10800      // Offset para GMT-3 (Brasil)
#define DAYLIGHT_OFFSET_SEC 0

#include <Arduino.h>

const uint8_t ligar_16[] = { /* ... cole os ticks do 16 aqui ...  };
const uint8_t ligar_17[] = { /* ... cole os ticks do 17 aqui ...  };
const uint8_t ligar_18[] = { /* ... cole os ticks do 18 aqui ... / };
const uint8_t ligar_19[] = { /* ... cole os ticks do 19 aqui ... / };
const uint8_t ligar_20[] = { /* ... cole os ticks do 20 aqui ... / };
const uint8_t ligar_21[] = { /* ... cole os ticks do 21 aqui ... / };
const uint8_t ligar_22[] = { /* ... cole os ticks do 22 aqui ... / };
const uint8_t ligar_23[] = { /* ... cole os ticks do 23 aqui ... / };
const uint8_t ligar_24[] = { /* ... cole os ticks do 24 aqui ... / };
const uint8_t ligar_25[] = { /* ... cole os ticks do 25 aqui ... / };
const uint8_t ligar_26[] = { /* ... cole os ticks do 26 aqui ... / };
const uint8_t ligar_27[] = { /* ... cole os ticks do 27 aqui ... / };
const uint8_t ligar_28[] = { /* ... cole os ticks do 28 aqui ... / };
const uint8_t ligar_29[] = { /* ... cole os ticks do 29 aqui ... / };
const uint8_t ligar_30[] = { /* ... cole os ticks do 30 aqui ... / };
const uint8_t ligar_31[] = { /* ... cole os ticks do 31 aqui ... / };

const uint8_t desligar_16[] = { /* ... cole os ticks do 16 aqui ... / };
const uint8_t desligar_17[] = { /* ... cole os ticks do 17 aqui ... / };
const uint8_t desligar_18[] = { /* ... cole os ticks do 18 aqui ... / };
const uint8_t desligar_19[] = { /* ... cole os ticks do 19 aqui ... / };
const uint8_t desligar_20[] = { /* ... cole os ticks do 20 aqui ... / };
const uint8_t desligar_21[] = { /* ... cole os ticks do 21 aqui ... / };
const uint8_t desligar_22[] = { /* ... cole os ticks do 22 aqui ... / };
const uint8_t desligar_23[] = { /* ... cole os ticks do 23 aqui ... / };
const uint8_t desligar_24[] = { /* ... cole os ticks do 24 aqui ... / };
const uint8_t desligar_25[] = { /* ... cole os ticks do 25 aqui ... / };
const uint8_t desligar_26[] = { /* ... cole os ticks do 26 aqui ... / };
const uint8_t desligar_27[] = { /* ... cole os ticks do 27 aqui ... / };
const uint8_t desligar_28[] = { /* ... cole os ticks do 28 aqui ... / };
const uint8_t desligar_29[] = { /* ... cole os ticks do 29 aqui ... / };
const uint8_t desligar_30[] = { /* ... cole os ticks do 30 aqui ... / };
const uint8_t desligar_31[] = { /* ... cole os ticks do 31 aqui ... / };

const uint8_t* const arr_temperaturas_on[] = {
  ligar_16, ligar_17, ligar_18, ligar_19, 
  ligar_20, ligar_21, ligar_22, ligar_23, 
  ligar_24, ligar_25, ligar_26, ligar_27, 
  ligar_28, ligar_29, ligar_30, ligar_31
};

const uint8_t* const arr_temperaturas_off[] = {
  desligar_16, desligar_17, desligar_18, desligar_19, desligar_20, 
  desligar_21, desligar_22, desligar_23, desligar_24, desligar_25, 
  desligar_26, desligar_27, desligar_28, desligar_29, desligar_30, 
  desligar_31
};

const uint16_t arr_temperaturas_TAMANHOS[] = {
  sizeof(ligar_16), sizeof(ligar_17), sizeof(ligar_18), sizeof(ligar_19),
  sizeof(ligar_20), sizeof(ligar_21), sizeof(ligar_22), sizeof(ligar_23),
  sizeof(ligar_24), sizeof(ligar_25), sizeof(ligar_26), sizeof(ligar_27),
  sizeof(ligar_28), sizeof(ligar_29), sizeof(ligar_30), sizeof(ligar_31)
};

const uint16_t ligar_OFF_LEN = sizeof(ligar_OFF);


#endif
*/
