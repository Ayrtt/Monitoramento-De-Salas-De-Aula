# Monitoramento de Salas de Aula

Projeto de conclusão do curso de Engenharia de Computação pelo IFPB. Trata-se de um sistema para monitoramento e controle automatizado de iluminação e climatização em salas de aula ou laboratórios.

## Sumário
- Materiais Utilizados;
- Arquivos do repositório;
- Funcionamento do projeto

## Materiais utilizados
- Microcontrolador ESP32;
- Sensor de movimento PIR; 
- Relé eletromecânico de 5V;
- Fonte de alimentação 5V;
- Servidor remoto para armazenamento de dados;
- Interface de monitoramento online;
- Sensor de temperatura DHT11;
- Emissor LED Infravermelho;
- Transistor BC 547 NPN;
- Resistores de 220Ω e 10kΩ.


## Arquivos do repositório

- Pasta Firebase
	- Arquivos para implementação do projeto utilizando a base de dados Firebase Realtime Database, da Google. Aqui estão os arquivos pertencentes à configuração final e modularização das funções para testes de funcionamento da conexão do hardware com essa base de dados.
- Pasta MongoDB
	- Arquivos para implementação do projeto utilizando a base de dados MongoDB. Aqui estão os arquivos finalizados do projeto para implementação do MVP.
		- Na pasta "front" estão os arquivos pertinentes à configuração da api e da interface web utilizadas.
- Os demais são arquivos de testes de sensores e funções.

## Funcionamento do projeto

Após conectar o sistema à energia, o ESP inicializará e se conectará ao banco de dados. Feita a conexão, o loop começa, lendo os sensores e enviando os primeiros registros. 

O sensor DHT está programado para ler periodicamente. Caso a temperatura seja igual à leitura anterior, nada acontecerá. Caso seja diferente, um novo registro será enviado.

Quando for detectada presença por parte do sensor PIR, o relé será acionado, iniciando o temporizador e enviando o registro devido, informando sua ativação. Então, retorna-se ao loop à espera de novas ativações ou do fim do temporizador. Quando não houver mais sinais de presença e o temporizador chegar ao fim, será enviado um novo registro indicando o desligamento da luz.

Já na interface Web, temos o botão "Power Luz", responsável pela ativação e desativação remota do relé que controla o LED, o "Power Ar" que controla a ativação e desativação remota do aparelho ar-condicionado, e o botão "Definir Temperatura", para quando quiser alterar a temperatura.

Os códigos infravermelhos do aparelho ar-condicionado foram previamente adquiridos por meio de um receptor infravermelho universal VS1838B,  armazenados e configurados no ESP. Cada botão desses envia o comando correspondente para que o LED emissor possa enviá-lo. 
