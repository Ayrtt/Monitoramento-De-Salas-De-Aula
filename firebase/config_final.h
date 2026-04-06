// config.h

#ifndef CONFIG_H
#define CONFIG_H

// --- Configurações de Rede ---
#define WIFI_SSID 
#define WIFI_PASSWORD 

// --- Configurações do Firebase ---
#define FIREBASE_API_KEY 
#define FIREBASE_DATABASE_URL 
#define FIREBASE_USER_EMAIL 
#define FIREBASE_USER_PASSWORD 

// ID do dispositivo para facilitar a troca no futuro
#define DEVICE_ID "/0001"

// Caminho para nó principal
#define PATH_CONTROLE "/Dispositivos" DEVICE_ID

// Caminhos específicos para o controle do ar-condicionado
#define PATH_AR_ESTADO PATH_CONTROLE "/ar/estado"
#define PATH_AR_REGISTROS PATH_CONTROLE "/registros/ar"
#define PATH_TEMPERATURA PATH_CONTROLE "/ar/temperatura"
#define PATH_TEMPERATURA_FLAG PATH_CONTROLE "/ar/temperatura_flag"

#define PATH_LUZ_ESTADO PATH_CONTROLE "/luz/estado"
#define PATH_LUZ_REGISTROS PATH_CONTROLE "/registros/luz"

// --- Configurações do Hardware (Pinos) ---
#define LED_PIN 27
#define DHT_PIN 14
#define DHTTYPE DHT11

#define RELAY_PIN 32
#define PIR_PIN 34

#define DHT_READ_DELAY 10000
#define INACTIVITY_TIMEOUT_MS 5000

// --- Configurações do Servidor de Tempo (NTP) ---
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC -10800      // Offset para GMT-3 (Brasil)
#define DAYLIGHT_OFFSET_SEC 0

#endif
