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
#define DEVICE_ID "0001"

// Caminhos para os registros históricos
#define PATH_LUZ_REGISTROS "/Dispositivos/" DEVICE_ID "/registros/luz"
#define PATH_AR_REGISTROS "/Dispositivos/" DEVICE_ID "/registros/ar"


// --- Configurações do Hardware (Pinos) ---
#define PIR_PIN 34 // Pino do sensor PIR

// --- Configurações de Lógica e Temporizadores ---
#define INACTIVITY_TIMEOUT_MS 5000 // 5 segundos para timeout de inatividade

// --- Configurações do Servidor de Tempo (NTP) ---
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC -10800      // Offset para GMT-3 (Brasil)
#define DAYLIGHT_OFFSET_SEC 0

#endif