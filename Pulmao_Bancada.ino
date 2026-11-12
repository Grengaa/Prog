/*
  Pulmao_Bancada — Arduino Uno R3 / ATmega328P, HX710B e L298N.
  SOMENTE PROTOTIPO DE BANCADA, SEM USO HUMANO OU ANIMAL.
  Codigo novo, inspirado na organizacao enA=9/in1=8/in2=7 de:
  Abo-Bakr Emara — https://github.com/BakrEmara/Ventilator-
  Leitura HX710B baseada no datasheet AVIA (ver LEIA-ME).
  Nenhum valor de pressao ou tempo abaixo e prescricao clinica.
  O motor inicia desabilitado. Primeiro calibre e ajuste a montagem.
*/
#include <Arduino.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include "Controle.h"

// Evita loop de watchdog depois de um reset (alem do suporte do bootloader).
void desligarWatchdogInicial() __attribute__((naked, section(".init3")));
void desligarWatchdogInicial() { MCUSR=0; wdt_disable(); }

const uint8_t ENA=9, IN1=8, IN2=7, HX_OUT=2, HX_SCK=3;
const uint8_t BOTAO_PARAR=4; // Botao NA entre D4 e GND. Tambem existe comando X.
const bool INVERTER_SENTIDO=false; // Conferir SEM a linha presa antes de operar.

// PREENCHER COM MEDICOES REAIS, consultando LEIA-ME.
const bool CALIBRACAO_CONFERIDA=false;
const int32_t RAW_ZERO=0;          // Media na pressao diferencial zero.
const int32_t RAW_REFERENCIA=0;    // Media sob pressao conhecida, medida por manometro.
const float PRESSAO_REFERENCIA_KPA=0.0f;

const Ajustes AJ = {
  0.0f, // Limite de pressao em kPa: definir para o ensaio de bancada.
  0,    // Tempo maximo de compressao (ms): definir na montagem.
  0,    // Tempo de retorno (ms): estimativa, NAO confirma posicao inicial.
  1000, // Pausa depois do retorno (ms): exemplo ajustavel.
  200,  // Intervalo sem acionamento antes da inversao (ms): ajustar ao motor.
  150,  // Idade maxima da leitura em operacao (ms), para modo nominal 40 SPS.
  0,    // PWM compressao (1..255): iniciar baixo nos ensaios; zero bloqueia partida.
  0     // PWM retorno (1..255).
};

Controle controle;
bool hxConfigurado=false, amostraDisponivel=false, erroProtocolo=false;
uint8_t descartar=4;
uint32_t inicio=0, configuradoEm=0, ultimaAmostra=0, ultimoEnvio=0;
int32_t bruto=0;
float pressao=NAN;
int64_t soma=0;
uint8_t nMedia=0;
int ultimoComando=32767;

bool calibrado() {
  return CALIBRACAO_CONFERIDA && RAW_ZERO != RAW_REFERENCIA &&
    RAW_ZERO > -8388608L && RAW_ZERO < 8388607L &&
    RAW_REFERENCIA > -8388608L && RAW_REFERENCIA < 8388607L &&
    isfinite(PRESSAO_REFERENCIA_KPA) && PRESSAO_REFERENCIA_KPA > 0 &&
    PRESSAO_REFERENCIA_KPA < 40;
}
void aplicarMotor(int cmd) {
  if (cmd==ultimoComando) return;
  analogWrite(ENA,0); // Desabilita ANTES de alterar a polaridade.
  digitalWrite(IN1,LOW); digitalWrite(IN2,LOW);
  if (cmd!=0) {
    bool comprimir=(cmd>0);
    if (INVERTER_SENTIDO) comprimir=!comprimir;
    digitalWrite(IN1,comprimir ? LOW : HIGH);
    digitalWrite(IN2,comprimir ? HIGH : LOW);
    analogWrite(ENA,cmd>0 ? cmd : -cmd);
  }
  ultimoComando=cmd;
}

// Retorna sem esperar quando a conversao ainda nao terminou.
// Nao e I2C e nao usa analogRead(). 24 bits + 3 pulsos = diferencial, 40 SPS.
bool lerHX(int32_t &valor) {
  if (digitalRead(HX_OUT)==HIGH) return false;
  uint32_t bits=0;
  uint8_t salvo=SREG; cli();
  delayMicroseconds(1);
  for (uint8_t i=0;i<24;i++) {
    digitalWrite(HX_SCK,HIGH); delayMicroseconds(1);
    bits=(bits<<1) | (digitalRead(HX_OUT) ? 1UL : 0UL);
    digitalWrite(HX_SCK,LOW); delayMicroseconds(1);
  }
  for (uint8_t i=0;i<3;i++) {
    digitalWrite(HX_SCK,HIGH); delayMicroseconds(1);
    digitalWrite(HX_SCK,LOW); delayMicroseconds(1);
  }
  // DOUT deve voltar a HIGH apos o pulso 25: detecta, por ex., linha presa em LOW.
  bool liberou=(digitalRead(HX_OUT)==HIGH);
  SREG=salvo;
  if (!liberou) { erroProtocolo=true; return false; }
  valor=(bits & 0x800000UL) ? (int32_t)(bits | 0xFF000000UL) : (int32_t)bits;
  return true;
}
bool sensorRecente(uint32_t agora) {
  return amostraDisponivel && !erroProtocolo &&
    (uint32_t)(agora-ultimaAmostra)<=AJ.leituraMaxMs &&
    bruto != -8388608L && bruto != 8388607L;
}
void processarSensor(uint32_t agora) {
  if (hxConfigurado && (uint32_t)(agora-configuradoEm)<200) return;
  int32_t leitura;
  if (!lerHX(leitura)) return;
  if (!hxConfigurado) {
    // Datasheet recomenda power-down apos selecionar outro modo.
    digitalWrite(HX_SCK,HIGH); delayMicroseconds(90); digitalWrite(HX_SCK,LOW);
    hxConfigurado=true; configuradoEm=agora; return;
  }
  if (descartar) { --descartar; return; }
  bruto=leitura; ultimaAmostra=agora; amostraDisponivel=true;
  if (calibrado()) {
    pressao=((float)bruto-(float)RAW_ZERO)*PRESSAO_REFERENCIA_KPA /
      ((float)RAW_REFERENCIA-(float)RAW_ZERO);
  } else pressao=NAN;
  soma+=bruto;
  if (++nMedia==40) {
    Serial.print(F("# media_raw_40=")); Serial.println((long)(soma/40));
    soma=0; nMedia=0;
  }
}
void comandos(uint32_t agora) {
  // Limite por iteracao: fluxo serial continuo nao pode impedir leitura/controle.
  for (uint8_t i=0;i<16 && Serial.available();i++) {
    char c=Serial.read();
    if (c=='x'||c=='X') {
      // X tambem reconhece falha; sempre exige novo S, nunca retoma sozinho.
      controle.parar(); aplicarMotor(0);
      Serial.println(F("# PARADO. Reposicione manualmente antes de novo S."));
    } else if (c=='s'||c=='S') {
      bool pronto=calibrado() && AJ.limiteKPa<=PRESSAO_REFERENCIA_KPA && sensorRecente(agora) && digitalRead(BOTAO_PARAR)==HIGH;
      if (!controle.iniciar(agora,AJ,pronto,pressao))
        Serial.println(F("# PARTIDA BLOQUEADA: confira calibracao, ajustes, sensor e estado."));
      else Serial.println(F("# INICIO: posicao inicial deve ter sido conferida manualmente."));
    }
  }
}
void setup() {
  digitalWrite(ENA,LOW); pinMode(ENA,OUTPUT);
  digitalWrite(IN1,LOW); digitalWrite(IN2,LOW);
  pinMode(IN1,OUTPUT); pinMode(IN2,OUTPUT); aplicarMotor(0);
  digitalWrite(HX_SCK,LOW); pinMode(HX_SCK,OUTPUT);
  pinMode(HX_OUT,INPUT_PULLUP); pinMode(BOTAO_PARAR,INPUT_PULLUP);
  Serial.begin(115200); inicio=millis();
  Serial.println(F("# BANCADA: S=iniciar; X=parar/reconhecer falha. Motor bloqueado sem ajustes."));
  Serial.println(F("# estados: 0=parado 1=comprime 2=inverte 3=retorna 4=pausa 5=falha"));
  Serial.println(F("# falhas: 0=nenhuma 1=sensor 2=tempo compressao 3=pressao invalida"));
  wdt_enable(WDTO_250MS);
}
void loop() {
  wdt_reset(); uint32_t agora=millis();
  processarSensor(agora);
  comandos(agora);
  if (digitalRead(BOTAO_PARAR)==LOW) controle.parar();
  controle.atualizar(agora,AJ,calibrado() && sensorRecente(agora),pressao);
  aplicarMotor(controle.comando(AJ));
  if ((uint32_t)(agora-ultimoEnvio)>=100) {
    ultimoEnvio=agora;
    Serial.print(F("raw:")); Serial.print((long)bruto);
    Serial.print(F("\tpressao_kPa:"));
    if (sensorRecente(agora) && calibrado()) Serial.print(pressao,4);
    else Serial.print(F("nan"));
    Serial.print(F("\tlimite_kPa:")); Serial.print(AJ.limiteKPa,4);
    Serial.print(F("\testado:")); Serial.print((int)controle.estado);
    Serial.print(F("\tfalha:")); Serial.print((int)controle.falha);
    Serial.print(F("\tsensor_ok:")); Serial.println(sensorRecente(agora) ? 1 : 0);
    if (!amostraDisponivel && (uint32_t)(agora-inicio)>1500)
      Serial.println(F("# Sem leitura valida: confira VCC/GND/OUT/SCK e reinicie."));
  }
}
