#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

// Network
const char* WIFI_SSID = "";
const char* WIFI_PASSWORD= "";
const char* serverName = "";
const int serverPort =;

const char* deviceID = "";

// --- Configurações do Hardware (Pinos) ---
#define LED_PIN 27
#define DHT_PIN 14
#define DHTTYPE DHT11

#define RELAY_PIN 32
#define PIR_PIN 34

#define RELAY_ON LOW  
#define RELAY_OFF HIGH 

#if !defined(RAW_BUFFER_LENGTH) // IR remote buffer
#define RAW_BUFFER_LENGTH 1024
#endif

// --- Configurações dos delays ---
#define DHT_READ_DELAY 20000
#define INACTIVITY_TIMEOUT_MS 5000

// --- Configurações do Servidor de Tempo (NTP) ---
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC -10800      // Offset para GMT-3 (Brasil)
#define DAYLIGHT_OFFSET_SEC 0

#endif
