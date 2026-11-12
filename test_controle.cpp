#include "Pulmao_Bancada/Controle.h"
#include <cassert>
#include <iostream>
int main(){
 Ajustes a={2,500,300,1000,200,150,60,50}; Controle c;
 assert(!c.iniciar(0,a,false,0)); assert(!c.iniciar(0,a,true,2));
 assert(c.iniciar(0,a,true,0)); assert(c.comando(a)==60);
 c.atualizar(10,a,true,2); assert(c.estado==Estado::INVERTENDO && c.comando(a)==0);
 c.atualizar(209,a,true,1); assert(c.comando(a)==0);
 c.atualizar(210,a,true,1); assert(c.comando(a)==-50);
 c.atualizar(510,a,true,0); assert(c.estado==Estado::PAUSA && c.comando(a)==0);
 c.atualizar(1510,a,true,0); assert(c.comando(a)==60);
 c.atualizar(1511,a,false,0); assert(c.estado==Estado::FALHA && c.comando(a)==0);
 assert(!c.iniciar(1512,a,true,0)); c.atualizar(3000,a,true,0); assert(c.estado==Estado::FALHA);
 c.parar();assert(c.iniciar(0,a,true,0));c.atualizar(500,a,true,0);assert(c.falha==Falha::TEMPO_COMPRESSAO);
 c.parar();assert(c.iniciar(0,a,true,0));c.atualizar(1,a,true,NAN);assert(c.comando(a)==0);
 c.parar();uint32_t n=0xFFFFFF00UL;assert(c.iniciar(n,a,true,0));c.atualizar(n+500,a,true,0);assert(c.falha==Falha::TEMPO_COMPRESSAO);
 for(auto s:{Estado::COMPRIMINDO,Estado::INVERTENDO,Estado::RETORNANDO,Estado::PAUSA}){
  c.estado=s;c.atualizar(0,a,false,0);assert(c.comando(a)==0 && c.estado==Estado::FALHA);
 }
 a.pwmComprimir=0;assert(!c.configurado(a));a.pwmComprimir=60;a.limiteKPa=NAN;assert(!c.configurado(a));
 std::cout<<"PASS: ciclo, pausa de inversao, falhas, bloqueios, timeout e rollover\n";
}
