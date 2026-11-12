#pragma once
#include <stdint.h>
#include <math.h>

// Logica de bancada: limiar de pressao + retorno temporizado, sem posicao medida.
struct Ajustes {
  float limiteKPa;
  uint32_t compressaoMaxMs, retornoMs, pausaMs, inversaoMs, leituraMaxMs;
  uint8_t pwmComprimir, pwmRetornar;
};
enum class Estado : uint8_t { PARADO, COMPRIMINDO, INVERTENDO, RETORNANDO, PAUSA, FALHA };
enum class Falha : uint8_t { NENHUMA, SENSOR, TEMPO_COMPRESSAO, PRESSAO_INVALIDA };
struct Controle {
  Estado estado = Estado::PARADO;
  Falha falha = Falha::NENHUMA;
  uint32_t desde = 0;
  bool configurado(const Ajustes &a) const {
    return isfinite(a.limiteKPa) && a.limiteKPa > 0 && a.limiteKPa < 40 &&
      a.compressaoMaxMs > 0 && a.compressaoMaxMs <= 10000 &&
      a.retornoMs > 0 && a.retornoMs <= 10000 &&
      a.pausaMs >= 100 && a.pausaMs <= 60000 &&
      a.inversaoMs >= 50 && a.inversaoMs <= 2000 &&
      a.leituraMaxMs > 0 && a.leituraMaxMs <= 250 &&
      a.pwmComprimir > 0 && a.pwmRetornar > 0;
  }
  bool ativo() const { return estado != Estado::PARADO && estado != Estado::FALHA; }
  void parar() { estado=Estado::PARADO; falha=Falha::NENHUMA; }
  void erro(Falha f) { estado=Estado::FALHA; falha=f; }
  bool iniciar(uint32_t agora, const Ajustes &a, bool sensorOk, float p) {
    if (estado != Estado::PARADO || !configurado(a) || !sensorOk ||
        !isfinite(p) || p < -0.5f || p >= a.limiteKPa) return false;
    estado=Estado::COMPRIMINDO; desde=agora; return true;
  }
  void atualizar(uint32_t agora, const Ajustes &a, bool sensorOk, float p) {
    if (!ativo()) return;
    if (!sensorOk) { erro(Falha::SENSOR); return; }
    if (!isfinite(p) || p < -0.5f || p >= 40) { erro(Falha::PRESSAO_INVALIDA); return; }
    uint32_t dt=agora-desde; // subtracao sem sinal suporta a volta de millis().
    switch (estado) {
      case Estado::COMPRIMINDO:
        if (p >= a.limiteKPa) { estado=Estado::INVERTENDO; desde=agora; }
        else if (dt >= a.compressaoMaxMs) erro(Falha::TEMPO_COMPRESSAO);
        break;
      case Estado::INVERTENDO:
        if (dt >= a.inversaoMs) { estado=Estado::RETORNANDO; desde=agora; }
        break;
      case Estado::RETORNANDO:
        if (dt >= a.retornoMs) { estado=Estado::PAUSA; desde=agora; }
        break;
      case Estado::PAUSA:
        if (dt >= a.pausaMs) {
          if (p >= a.limiteKPa) erro(Falha::PRESSAO_INVALIDA);
          else { estado=Estado::COMPRIMINDO; desde=agora; }
        }
        break;
      default: break;
    }
  }
  int comando(const Ajustes &a) const {
    if (estado==Estado::COMPRIMINDO) return a.pwmComprimir;
    if (estado==Estado::RETORNANDO) return -int(a.pwmRetornar);
    return 0;
  }
};
