/*
 * Sketch de Teste de envio de códigos do Ar Condicionado (TCL)
 */
#include <Arduino.h>
#include <IRremote.hpp>

#include "codigos.h"

const int kSendPin = 27;

int idx = 15;

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(50); }
    IrSender.begin(kSendPin, false);
}

void loop() {
    Serial.println(F("Enviando comando RAW"));

    const uint16_t* dados = arr_comandos_on[idx].rawData;
    size_t tamanho = arr_comandos_on[idx].length;

    //enviarRAWConvertido(dados, tamanho);
    //IrSender.sendRaw(dados, tamanho, 38);
    //com problema: 25,31
    const uint16_t* dados_off = arr_comando_off.rawData;
    size_t tamanho_off = arr_comando_off.length;
    enviarRAWConvertido(dados_off, tamanho_off);

    Serial.println(F("Comando enviado."));
    Serial.println(F("A enviar novamente em 5 segundos..."));
    delay(5000);
}

void enviarRAWConvertido(const uint16_t* dados_ticks, size_t tamanho) {
    uint16_t* dados_micros = (uint16_t*)malloc(tamanho * sizeof(uint16_t));
    if (dados_micros == NULL) {
        Serial.println("[ERRO IR] Falha ao alocar memória para conversão!");
        return;
    }

    // Converte: Valor * 50 (fator de conversão comum para códigos RAW curtos)
    for (size_t i = 0; i < tamanho; i++) {
        dados_micros[i] = dados_ticks[i] * 50; 
    }
    
    for(int i = 0; i<3; i++){
        Serial.println("[IR] Enviando sinal convertido..."); 
        IrSender.sendRaw(dados_micros, tamanho, 38);
      delay(1000);
    }
    free(dados_micros);
}

/*
const uint16_t TCL_22_LEN = sizeof(TCL_22_GRAUS) / sizeof(TCL_22_GRAUS[0]);
    IrSender.sendRaw(TCL_22_GRAUS, TCL_22_LEN, 38); //Variável com o nome do código, tamanho, frequência
*/
