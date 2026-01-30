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

#define RELAY_ON LOW  
#define RELAY_OFF HIGH 

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
void sendRegisterPIR(bool presenca);
void pir_manager_task(void *pvParameter);

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
int temperatura_alvo = 24;
SemaphoreHandle_t pirSemaphore;
bool luz_estado_atual = false; 

void pir_manager_task(void *pvParameter) {
  unsigned long lastActivityTime = 0;
  bool isPresenceActive = false;

  while (1) {
    if (xSemaphoreTake(pirSemaphore, 0) == pdTRUE) {
      lastActivityTime = millis(); 

      if (!isPresenceActive) {
        isPresenceActive = true;
        Serial.println("[PIR] Movimento detectado! Ligando luz.");
        sendRegisterPIR(true);
      }
    }

    if (isPresenceActive && (millis() - lastActivityTime > INACTIVITY_TIMEOUT_MS)) {
      isPresenceActive = false;
      Serial.println("[PIR] Inatividade. Desligando luz.");
      sendRegisterPIR(false);
    }

    vTaskDelay(pdMS_TO_TICKS(50)); 
  }
}

void IRAM_ATTR sensor_isr_handler() {
  xSemaphoreGiveFromISR(pirSemaphore, NULL);
}

void setup() {
  Serial.begin(115200); 
  Serial.println("Setup: Inicializando...");

  digitalWrite(RELAY_PIN, RELAY_OFF); 
  pinMode(RELAY_PIN, OUTPUT); 
  pinMode(PIR_PIN, INPUT);

  setup_wifi();
  
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

  setup_ssl(); 
  setup_stream_ssl();
  setup_firebase();
  dht.begin();

  IrSender.begin(LED_PIN);

  pirSemaphore = xSemaphoreCreateBinary();
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), sensor_isr_handler, RISING);
  
  xTaskCreate(&pir_manager_task, "pir_manager_task", 12288, NULL, 5, NULL);

  Serial.println("Aguardando autenticação...");
  while (!app.ready()) {
      app.loop();
      delay(100);
  }
  Serial.println("Firebase pronto!");

  Database.get(streamClient, PATH_CONTROLE, processData, true /* SSE mode */, "streamControlTask");
}

void loop() {
  app.loop();
  
  if (app.ready()) {
      if (millis() - ultima_coleta > DHT_READ_DELAY) {
          int temp = getTemperaturaDHT();

          if (temp > -100) {
              Serial.printf("Temperatura: %d°C (Estado AR: %s)\n", temp, estado ? "ON" : "OFF");
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
          if (stream.dataPath().equals("/")) return;

          Serial.println("=========================================");
          Firebase.printf("[STREAM EVENTO] Evento: %s\n", stream.event().c_str());
          Firebase.printf("[STREAM DADOS] Caminho: %s\n", stream.dataPath().c_str());
          Serial.println("=========================================");
          
          if (stream.dataPath().equals("/ar/temperatura")) {
              temperatura_alvo = stream.to<int>();
              Serial.printf("[SYNC] Temperatura alvo: %d\n", temperatura_alvo);
          }
          else if(stream.dataPath().equals("/ar/temperatura_flag") && stream.to<bool>() == true){        
            Serial.println("[FLAG] Comando de Temperatura recebido.");
            Database.get(aClient, PATH_TEMPERATURA, processData, false, "getTempTask");
            
            if (temperatura_alvo >= 16 && temperatura_alvo <= 31) {
                sendCodeAr(temperatura_alvo);
                estado = true;
                Database.set<bool>(aClient, PATH_AR_ESTADO, true);
            }
            Database.set<bool>(aClient, PATH_TEMPERATURA_FLAG, false);
          } 
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
            }
          }

          else if (stream.dataPath().equals("/luz/estado")) {
            bool novo_estado = stream.to<bool>();
            
            if (novo_estado != luz_estado_atual) {
                Serial.printf("[STREAM] Comando manual recebido: %s\n", novo_estado ? "LIGAR" : "DESLIGAR");
                // Usa as constantes RELAY_ON/OFF para garantir a lógica correta
                digitalWrite(RELAY_PIN, novo_estado ? RELAY_ON : RELAY_OFF);
                luz_estado_atual = novo_estado;
            }
          }
      }
    }
    else if (aResult.uid() == "getTempTask") {
        RealtimeDatabaseResult &result = aResult.to<RealtimeDatabaseResult>();
        int t = result.to<int>();
        Serial.printf("[GET TASK] Temperatura confirmada: %d\n", t);
        
        if (t != temperatura_alvo && t >= 16 && t <= 31) {
            temperatura_alvo = t;
            sendCodeAr(temperatura_alvo);
            estado = true;
            Database.set<bool>(aClient, PATH_AR_ESTADO, true);
        }
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
  json.set("estado", estado ? true : false);
  json.set("temperatura", temperatura);

  String jsonString;
  json.toString(jsonString);

  Database.push<object_t>(aClient, PATH_AR_REGISTROS, object_t(jsonString), processData, "pushRegistroTask");
  
  Database.set<int>(aClient, PATH_TEMPERATURA, temperatura);
  Database.set<bool>(aClient, PATH_AR_ESTADO, estado);
}

void sendRegisterPIR(bool presenca) {
  luz_estado_atual = presenca;
  
  // Usa as constantes RELAY_ON/OFF para garantir a lógica correta
  digitalWrite(RELAY_PIN, presenca ? RELAY_ON : RELAY_OFF);

  FirebaseJson json;
  String dataHoraStr = getTimeDate();

  json.set("dataHora", dataHoraStr);
  json.set("estado", presenca ? true : false);

  String jsonString;
  json.toString(jsonString);

  Serial.print("Enviando registro PIR: ");
  Serial.println(jsonString);

  Database.set<bool>(aClient, PATH_LUZ_ESTADO, presenca);
  Database.push<object_t>(aClient, PATH_LUZ_REGISTROS, object_t(jsonString), processData, "pushRegistroTask");
}

void enviarRAWConvertido(const uint16_t* dados_ticks, size_t tamanho) {
    // Buffer temporário para armazenar os dados em microssegundos
    // IMPORTANTE: Se o tamanho do código for muito grande, isso pode estourar a pilha.
    // Verifique se o tamanho máximo dos seus códigos cabe aqui.
    // Se for muito grande, aloque dinamicamente com malloc() ou envie em chunks (se a lib permitir).
    
    uint16_t* dados_micros = (uint16_t*)malloc(tamanho * sizeof(uint16_t));
    if (dados_micros == NULL) {
        Serial.println("[ERRO IR] Falha ao alocar memória para conversão!");
        return;
    }

    // Converte: Valor * 50 (fator de conversão comum para códigos RAW curtos)
    for (size_t i = 0; i < tamanho; i++) {
        dados_micros[i] = dados_ticks[i] * 50; 
    }

    Serial.println("[IR] Enviando sinal convertido...");
    for(int i = 0; i<3; i++){
      IrSender.sendRaw(dados_micros, tamanho, 38);
      delay(1000);
    }
    
    
    free(dados_micros); // Libera memória
}

void sendCodeAr(int temperatura){
  Serial.printf("[IR] Enviando comando LIGAR/TEMP %d°C no pino %d\n", temperatura, LED_PIN);
  estado = true;
  
  int index = temperatura - 16;
  
  if (index >= 0 && index < 16) {
      const uint16_t* dados = arr_comandos_on[index].rawData;
      size_t tamanho = arr_comandos_on[index].length;
      
      if (tamanho > 0) {
          // Chama a função de conversão antes de enviar
          enviarRAWConvertido(dados, tamanho);
          Serial.println("[IR] Código enviado.");
      } else {
          Serial.println("[AVISO IR] Código vazio. Preencha codigos_ar.h!");
      }
  } else {
      Serial.printf("[ERRO IR] Temperatura %d fora da faixa.\n", temperatura);
  }
}

void sendCodeArOFF() {
  Serial.println("[IR] Enviando comando DESLIGAR");
  estado = false;
  
  const uint16_t* dados = comando_off.rawData;
  size_t tamanho = comando_off.length;
  
  if (tamanho > 0) {
     // Chama a função de conversão antes de enviar
     enviarRAWConvertido(dados, tamanho);
     Serial.println("[IR] Código OFF enviado.");
  } else {
     Serial.println("[AVISO IR] Código OFF vazio no codigos_ar.h. Envio simulado.");
  }
}
