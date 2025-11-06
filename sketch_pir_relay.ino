#include <stdio.h>
#include <string.h>
#include <Arduino.h>

const int RELAY_PIN = 26;
const int PIR_PIN = 27;
bool flag = 0;
int pirState = 0;
unsigned long start_time;

void state_machine_task(void *pvParameter) {
  unsigned long lastDetectionTime = 0;
  bool isLedOn = false;

  while (1) {
    int pirReading = digitalRead(PIR_PIN);

    if (pirReading == HIGH) { 
      lastDetectionTime = millis();

      if (!isLedOn) {
        isLedOn = true;
        pirState = 1;
        digitalWrite(RELAY_PIN, LOW);
        Serial.println("Sensor ativado.");
      }
    }

    if (isLedOn && (millis() - lastDetectionTime > 5000)) {
      isLedOn = false;
      pirState = 0;
      digitalWrite(RELAY_PIN, HIGH);
      Serial.println("Sensor desativado.");
    }

    delay(10);
  }
}

void sensor_isr_handler() {
  pirState = 1;
}

void setup() {
  Serial.begin(9600);
  Serial.println("Setup.");
  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  xTaskCreate(&state_machine_task, "state_machine_task", 2048, NULL, 5, NULL);
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), sensor_isr_handler, RISING);
}

void loop() {
  delay(1000);
}
