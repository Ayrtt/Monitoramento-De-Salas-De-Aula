/*
 * Sketch "Sniffer" de Sinais IR (Versão Simplificada)
 * * Biblioteca: IRremote.hpp (v3.0+ por Armin Joachimsmeyer)
 * Objetivo: Capturar e imprimir o array rawData de um controle Infravermelho.
 * --- Hardware ---
 * * MÓDULO RECEPTOR (VS1838B):
 * - Pino VCC  -> ESP32 5V
 * - Pino GND  -> ESP32 GND
 * - Pino Data -> ESP32 GPIO 14 (definido em kRecvPin)
 */

#include <Arduino.h>
#include <IRremote.hpp>

#if !defined(RAW_BUFFER_LENGTH)
#define RAW_BUFFER_LENGTH 700
#endif

const int kRecvPin = 14;

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(50);
    }

    // Inicia o RECEPTOR
    IrReceiver.begin(kRecvPin, ENABLE_LED_FEEDBACK); // ENABLE_LED_FEEDBACK piscará o LED_BUILTIN

    Serial.print(F("Pronto para receber sinais IR no pino: "));
    Serial.println(kRecvPin);
    Serial.println(F("Aponte o controle e aperte um botão..."));
}

void loop() {
    if (IrReceiver.decode()) {
        Serial.println();
        Serial.println(F("--- NOVO CÓDIGO CAPTURADO ---"));

        /*
         * Imprime o resultado como um array C++ (rawData)
         * O 'true' no final força o formato de array (uint16_t rawData[])
         * mesmo que o protocolo seja conhecido.
         */
        IrReceiver.printIRResultRawFormatted(&Serial, true);

        // Imprime também um resumo (útil para depuração)
        Serial.println(F("--- Resumo do Protocolo ---"));
        IrReceiver.printIRResultShort(&Serial);
        Serial.println(F("----------------------------"));
        Serial.println();

        // Prepara o receptor para o próximo código
        IrReceiver.resume();
    }
}
