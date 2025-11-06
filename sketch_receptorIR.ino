/*
 * Sketch "Sniffer" de Sinais IR (Versão Simplificada)
 * * Biblioteca: IRremote.hpp (v3.0+ por Armin Joachimsmeyer)
 * Objetivo: Capturar e imprimir o array rawData de um controle de Ar Condicionado.
 * * Hardware:
 * - ESP32
 * - Receptor IR (ex: VS1838B)
 * * Circuito:
 * - Pino VCC do Receptor -> Pino 3V3 do ESP32
 * - Pino GND do Receptor -> Pino GND do ESP32
 * - Pino Data/Out do Receptor -> Pino GPIO 14 (definido em kRecvPin)
 */

#include <Arduino.h>

/*
 * Para controles de Ar Condicionado, um buffer maior é essencial.
 * O padrão (100) é muito pequeno. O seu exemplo usa 700, o que é excelente.
 */
#if !defined(RAW_BUFFER_LENGTH)
#define RAW_BUFFER_LENGTH 700
#endif

// Defina o pino de recepção
const int kRecvPin = 14;

#include <IRremote.hpp>

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(50); // Espera o Serial Monitor
    }

    // Inicia o receptor IR
    IrReceiver.begin(kRecvPin, ENABLE_LED_FEEDBACK); // ENABLE_LED_FEEDBACK piscará o LED_BUILTIN

    Serial.print(F("Pronto para receber sinais IR no pino: "));
    Serial.println(kRecvPin);
    Serial.println(F("Aponte o controle e aperte um botão..."));
}

void loop() {
    // Verifica se um código foi recebido
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