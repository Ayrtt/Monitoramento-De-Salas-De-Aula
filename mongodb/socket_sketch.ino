#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>

#include "config.h"

SocketIOclient socketIO;

// Função obrigatória para manter a biblioteca Socket.IO rodando internamente
void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case sIOtype_DISCONNECT:
            Serial.println("[WebSocket] Desconectado!");
            break;
        case sIOtype_CONNECT:
            Serial.printf("[WebSocket] Conectado! URL: %s\n", payload);
            socketIO.send(sIOtype_CONNECT, "/"); // Confirma a entrada no canal padrão
            break;
        case sIOtype_EVENT:
            // Caso o servidor mande uma resposta de volta (ex: "presence_registered_successfully")
            Serial.printf("[WebSocket] Resposta do Servidor: %s\n", payload);
            break;
    }
}

// ================================================================
// A FUNÇÃO ISOLADA DE ENVIO DE PRESENÇA
// ================================================================
void enviarRegistroPresenca(String deviceId, String tipoEvento) {
    
    // 1. Cria o documento e define que o formato principal é um Array
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();

    // 2. O primeiro item DEVE ser exatamente o nome que o @socketio.on() espera no app.py
    array.add("register_presence");

    // 3. Cria os dados reais (o payload) aninhados na segunda posição do Array
    JsonObject param1 = array.createNestedObject();
    param1["device_id"] = deviceId;
    param1["event"] = tipoEvento;

    // 4. Serializa tudo para texto
    String output;
    serializeJson(doc, output);

    // 5. Dispara para o servidor instantaneamente!
    socketIO.sendEVENT(output);

    Serial.print("Enviado: ");
    Serial.println(output);
}
// ================================================================

void setup() {
    Serial.begin(115200);

    Serial.print("Conectando ao WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Conectado!");

    // Inicia a conexão WebSocket. O parâmetro "/socket.io/?EIO=4" é crucial para o Flask!
    socketIO.begin(serverName, 5000, "/socket.io/?EIO=4");
    
    // Atrela a função de controle aos eventos da biblioteca
    socketIO.onEvent(socketIOEvent);
}

unsigned long ultimoTeste = 0;

void loop() {
    // ESTA LINHA É VITAL: Ela mantém o túnel aberto. Nunca use delay() longo no loop.
    socketIO.loop();

    // --- ROTINA DE TESTE ---
    // Em vez de usar botões ou sensores reais agora, vamos forçar 
    // o envio de um registro a cada 10 segundos para ver se o Flask recebe.
    if(millis() - ultimoTeste > 10000) {
        ultimoTeste = millis();
        
        // Chamamos a função passando os parâmetros desejados
        enviarRegistroPresenca("pir_sala01", "movimento_detectado");
    }
}
