/*
 * Sketch "Clone de TV" (Recebe 1x, Envia Infinitamente)
 * Biblioteca: IRremote.hpp (v3.0+ por Armin Joachimsmeyer)
 * Objetivo: Capturar o primeiro comando IR (ex: "Vol +") e
 * retransmiti-lo a cada 2 segundos.
 *
 * --- Hardware ---
 * * RECEPTOR (VS1838B):
 * - Pino VCC  -> ESP32 3V3
 * - Pino GND  -> ESP32 GND
 * - Pino Data -> Resistor 10kΩ -> ESP32 GPIO 14 (definido em kRecvPin)
 * * TRANSMISSOR (LED IR):
 * - ESP32 GPIO 4 -> Resistor 220Ω -> Perna LONGA (Anodo) do LED
 * - Perna CURTA (Catodo) do LED   -> ESP32 GND
 */

#include <Arduino.h>
#include <IRremote.hpp>

#if !defined(RAW_BUFFER_LENGTH)
#define RAW_BUFFER_LENGTH 700
#endif

const int kRecvPin = 14;
const int kSendPin = 27;

bool g_codeHasBeenCaptured = false;      // Flag para controlar o estado
IRData g_capturedIRData;                 // Armazena dados de protocolos conhecidos
uint8_t g_capturedRawData[RAW_BUFFER_LENGTH]; // Armazena dados RAW (se desconhecido)
uint8_t g_capturedRawLength;             // Tamanho dos dados RAW

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(50); 
    }

    // Inicia o RECEPTOR
    IrReceiver.begin(kRecvPin, ENABLE_LED_FEEDBACK);

    // Inicia o TRANSMISSOR
    // O false desabilita o feedback no LED_BUILTIN para envio,
    // para sabermos que o feedback é só da recepção.
    IrSender.begin(kSendPin, false); 

    Serial.println(F("Receptor pronto. Execute o comando no controle."));
}

void loop() {
    if (!g_codeHasBeenCaptured) {
        if (IrReceiver.decode()) {
            Serial.println();
            Serial.println(F("--- CÓDIGO CAPTURADO! ---"));
            
            g_capturedIRData = IrReceiver.decodedIRData;

            //Verifica se é um protocolo desconhecido (RAW)
            if (g_capturedIRData.protocol == UNKNOWN) {
                Serial.println(F("Protocolo desconhecido. Armazenando como RAW."));                
                g_capturedRawLength = IrReceiver.irparams.rawlen - 1;
                IrReceiver.compensateAndStoreIRResultInArray(g_capturedRawData);
                IrReceiver.printIRResultRawFormatted(&Serial, true);
            } else {
                Serial.println(F("Protocolo conhecido. Armazenando dados decodificados."));
                IrReceiver.printIRResultShort(&Serial);
            }

            //Muda a flag
            g_codeHasBeenCaptured = true;
            IrReceiver.stop();

            Serial.println(F("----------------------------------"));
            Serial.println(F("Receptor DESLIGADO."));
            Serial.println(F("Iniciando retransmissão em 2 segundos..."));
            
            delay(2000);
        }
    }
    else {
        Serial.print(F("Enviando código... "));
        if (g_capturedIRData.protocol == UNKNOWN) {
            Serial.println(F("(Modo RAW)"));
            IrSender.sendRaw(g_capturedRawData, g_capturedRawLength, 38); // 38 KHz é padrão
        } else {
            Serial.print(F("(Protocolo: "));
            Serial.print(getProtocolString(g_capturedIRData.protocol));
            Serial.println(F(")"));
            IrSender.write(&g_capturedIRData, NO_REPEATS);
        }
        delay(2000);
    }
}