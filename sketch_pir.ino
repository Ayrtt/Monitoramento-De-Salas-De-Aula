#include <stdio.h>
#include <string.h>
#include <Arduino.h>

const int PIR_PIN = 27; // Pino do sensor
int pirState = 0; // Estado do sensor

void state_machine_task(void *pvParameter) {
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
        Serial.println("Sensor ativado.");
      }
    }

    // Verifica se o tempo de retenção passou e o LED está ligado
    if (isLedOn && (millis() - lastDetectionTime > 5000)) {
      // Desliga o LED após o timeout
      isLedOn = false;
      pirState = 0;
      Serial.println("Sensor desativado.");
    }

    delay(10); // Pequeno atraso para evitar consumo excessivo de CPU
  }
}

/*
bool flag = 0; // Flag para controle do estado do sensor
unsigned long start_time; // Variável para armazenar o instante de início do temporizador

void state_machine_task(void *pvParameter) {
  while(1){
    int pir_pin_read = digitalRead(PIR_PIN);
    if((pir_pin_read==LOW)&&(flag==0)){ // Se o sensor for desativado e a flag estiver abaixada
      flag = 1; // Flag sobe para prevenir prints infinitos
      pirState = 0; // Estado atualizado
      Serial.println("Sensor desativado.");
    }

    if((pir_pin_read==HIGH)&&(flag==1)){ // Se o sensor for ativado e a flag estivar levantada
      flag = 0; // Flag abaixa para prevenir prints infinitos
      pirState = 1; // Estado atualizado
      Serial.println("Sensor ativado.");

      start_time = millis(); // Início do temporizador

      while(millis() - start_time <5000){ // Enquanto a diferença entre o instante atual e o início do temporizador for menor que cinco segundos
        Serial.println("While.");
        if (digitalRead(PIR_PIN) == LOW){ // Se o sensor desativar antes do tempo acabar
          Serial.println("While LOW.");
          break;
        }
         delay(10);
      }
    }
  }
}*/

void sensor_isr_handler() {
  pirState = 1; // Interrupção para alterar o estado do sensor
}

void setup() {
  Serial.begin(9600);
  Serial.println("Setup.");
  pinMode(PIR_PIN, INPUT);
  xTaskCreate(&state_machine_task, "state_machine_task", 2048, NULL, 5, NULL);
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), sensor_isr_handler, RISING);
}

void loop() {
  delay(1000);
}
