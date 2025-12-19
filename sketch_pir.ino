#include <stdio.h>
#include <Arduino.h>

#define RELAY_ON LOW  
#define RELAY_OFF HIGH 

void pir_manager_task(void *pvParameter);
void IRAM_ATTR sensor_isr_handler();
String getRuntimeFormatted(); // Função de tempo offline


const int RELAY_PIN = 32;


const int PIR_PIN = 34;
SemaphoreHandle_t pirSemaphore = NULL;

// --- Função que simula um relógio baseado no tempo ligado (Offline) ---
String getRuntimeFormatted() {
  unsigned long t = millis();
  unsigned long segundos = t / 1000;
  unsigned long minutos = (segundos / 60) % 60;
  unsigned long horas = (segundos / 3600);
  segundos = segundos % 60;

  char timeStringBuff[20];
  // Formata como HH:MM:SS
  sprintf(timeStringBuff, "%02lu:%02lu:%02lu", horas, minutos, segundos);
  return String(timeStringBuff);
}

void pir_manager_task(void *pvParameter) {
  unsigned long lastActivityTime = 0;
  bool isPresenceActive = false;

  while (1) {
    if (xSemaphoreTake(pirSemaphore, 0) == pdTRUE) {
      lastActivityTime = millis(); 

      if (!isPresenceActive) {
        isPresenceActive = true;
        Serial.print("[PIR] Movimento detectado! Ligando luz. -> ");       
        Serial.println(getRuntimeFormatted());
        digitalWrite(RELAY_PIN, RELAY_ON);
      }
    }

    if (isPresenceActive && (millis() - lastActivityTime > 10000)) {
      isPresenceActive = false;
      Serial.print("[PIR] Inatividade. Desligando luz.      -> ");
      Serial.println(getRuntimeFormatted());
      digitalWrite(RELAY_PIN, RELAY_OFF);
    }

    vTaskDelay(pdMS_TO_TICKS(50)); 
  }
}

void IRAM_ATTR sensor_isr_handler() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(pirSemaphore, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Setup iniciado.");
  
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);

  digitalWrite(RELAY_PIN, RELAY_OFF);

  pirSemaphore = xSemaphoreCreateBinary();
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), sensor_isr_handler, RISING);
  xTaskCreate(&pir_manager_task, "pir_task", 4096, NULL, 5, NULL);
}

void loop() {
  delay(1000);
}
