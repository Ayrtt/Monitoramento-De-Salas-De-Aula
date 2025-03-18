#include <stdio.h>
#include <string.h>
#include <Arduino.h>
/*
  #include <WiFi.h>
  #include <PubSubClient.h>

  const char* ssid = "Wokwi-GUEST";
  const char* password = "";

  const char* mqttServer = "200.129.71.138";
  const int mqttPort = 1883;
  const char* mqttUser = "ayrtton_silva:ff3908";
  const char* mqttTopic_PUBLISH_PIR = "ayrtton_silva:ff3908/attrs";
  const char* mqttTopic_SUBSCRIBE_RELAY = "ayrtton_silva:ff3908/config";
  // id dispositivo relay: 2a7071
  // id dispositivo pir: ff3908
  // dojot link: http://200.129.71.138:8000/#/device/list
  // ia: https://colab.research.google.com/drive/1mUc0Ru4_MC_ujn_-ultLWTTA8JthbC_P?authuser=3&pli=1#scrollTo=2DgkzosW7xLE


  WiFiClient espClient;
  PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao broker MQTT... ");
    if (client.connect("ESP32Client", mqttUser, "")) {
      Serial.println("conectado");
      client.subscribe(mqttTopic_SUBSCRIBE_RELAY);
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(". Tentando novamente em 5 segundos.");
      delay(5000);
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  String msg;

  for (int i = 0; i < length; i++) {
    char c = (char)payload[i];

    msg += c;
  }

  Serial.printf("Chegou a seguinte string via MQTT: %s do topico: %s\n", msg, topic);

  if (msg.equals("1")) {
    digitalWrite(RELAY_PIN, HIGH);
    Serial.print("Relé ligado por comando MQTT");
  }
  if (msg.equals("0")) {
    digitalWrite(RELAY_PIN, LOW);
    Serial.print("Relé desligado por comando MQTT");
  }
}

void publish_PIR(int pirState){
  char payload[32];
  sprintf(payload, "{\"pirState\": %d}", pirState);

  while (!client.publish(mqttTopic_PUBLISH_PIR, payload)) {
    Serial.println("Falha ao publicar a mensagem. Tentando novamente...");
    delay(500);
  }
  Serial.println("Mensagem publicada.");
  
  memset(payload, 0, sizeof(payload));
}
*/

const int PIR_PIN = 27;
const int RELAY_PIN = 26;

bool flag = 0;
int pirState = 0;
unsigned long start_time;



void state_machine_task(void *pvParameter) {
  while(1){
    int pir_pin_read = digitalRead(PIR_PIN);
    if((pir_pin_read==LOW)&&(flag==0)){
      flag = 1;
      pirState = 0;
      publish_PIR(pirState);
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("Sensor desativado.");
    }

    if((pir_pin_read==HIGH)&&(flag==1)){
      flag = 0;
      pirState = 1;
      publish_PIR(pirState);
      digitalWrite(RELAY_PIN, HIGH);
      Serial.println("Sensor ativado.");

      start_time = millis();

      while(millis() - start_time <5000){
        if (digitalRead(PIR_PIN) == LOW){
          break;
        }
         delay(10);
      }
    }
  }
}

/*void state_machine_task(void *pvParameter) {
  unsigned long lastDetectionTime = 0; // Marca o tempo da última detecção
  bool isLedOn = false; // Estado do LED

  while (1) {
    int pirReading = digitalRead(PIR_PIN);

    if (pirReading == HIGH) { 
      // Movimento detectado
      lastDetectionTime = millis(); // Atualiza o tempo da última detecção

      if (!isLedOn) {
        // Liga o LED e publica a mensagem apenas se ainda não estiver ligado
        isLedOn = true;
        pirState = 1;
        publish_PIR(pirState);
        digitalWrite(RELAY_PIN, HIGH);
        Serial.println("Sensor ativado.");
      }
    }

    // Verifica se o tempo de retenção passou e o LED está ligado
    if (isLedOn && (millis() - lastDetectionTime > 5000)) {
      // Desliga o LED após o timeout
      isLedOn = false;
      pirState = 0;
      publish_PIR(pirState);
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("Sensor desativado.");
    }

    delay(10); // Pequeno atraso para evitar consumo excessivo de CPU
  }
}*/

void sensor_isr_handler() {
  pirState = 1;
}

void setup() {
  Serial.begin(9600);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);

  //attachInterrupt(digitalPinToInterrupt(PIR_PIN), sensor_isr_handler, RISING);

  //setup_wifi();
  //client.setServer(mqttServer, mqttPort);
  //client.setCallback(callback);

  xTaskCreate(&state_machine_task, "state_machine_task", 2048, NULL, 5, NULL);
}

void loop() {
  /*
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  */ 
  Serial.println("Loop\n\n");
  delay(1000);
}