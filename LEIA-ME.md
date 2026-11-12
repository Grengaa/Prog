# Pulmão Mecânico — firmware para ensaios de bancada

Esta versão usa Arduino Uno R3 (ATmega328P), o módulo MPS20N0040D-D com conversor HX710B identificado na foto e um driver L298N. A faixa de 0–40 kPa é a declarada no anúncio do sensor, não uma faixa de operação escolhida para o protótipo. Não se destina a pessoas ou animais.

## O que o programa faz

1. Ao ligar, mantém o motor desabilitado e mostra as leituras no computador.
2. Depois de configurado, o comando `S` inicia a compressão.
3. Quando uma leitura válida atinge o limiar ajustado, desabilita o acionamento, aguarda o intervalo de inversão e aciona o retorno.
4. O retorno dura o tempo ajustado, seguido de uma pausa e do próximo ciclo.
5. Falha de comunicação, leitura desatualizada/inválida ou tempo máximo de compressão interrompem o acionamento e deixam uma falha registrada. Não há retorno automático nessas falhas. Reconheça com `X`, confira a montagem e só depois dê novo `S`. Falha de protocolo exige reiniciar a placa após corrigir a causa.

É um controle por limiar com retorno temporizado, não PID nem regulação contínua de pressão. O PWM é constante em cada movimento. A pressão decide quando termina a compressão. O limiar é um gatilho: atraso do sensor e inércia podem produzir pressão superior a ele.

O retorno NÃO mede nem garante posição inicial. Se a compressão terminar mais cedo, o mesmo tempo de retorno pode soltar linha demais. Verifique a posição inicial manualmente antes de cada partida e observe a linha nos ensaios. O programa não detecta todos os vazamentos, travamentos, rompimentos da linha ou falhas do sensor. Ao desligar o motor, a mecânica pode continuar comprimindo a bolsa; desabilitar o acionamento não comprova alívio de pressão.

## Arquivos

Extraia o ZIP e abra `Pulmao_Bancada/Pulmao_Bancada.ino`. Deixe `Controle.h` na mesma pasta. Não é necessário instalar biblioteca HX711/HX710: o protocolo do HX710B está implementado no sketch. Selecione Arduino Uno na IDE.

## Ligações

Faça as ligações com as fontes desligadas. Siga os NOMES impressos na placa, pois a ordem física depende de como ela está virada.

| Componente | Ligação no Uno |
|---|---|
| HX710B VCC | 5 V regulados do Uno, para o módulo mostrado |
| HX710B GND | GND |
| HX710B OUT | D2 |
| HX710B SCK | D3 |
| L298N ENA | D9, retirando o jumper ENA que o mantém habilitado |
| L298N IN1 | D8 |
| L298N IN2 | D7 |
| L298N GND | GND comum com Uno e fonte do motor |
| Botão de parada normalmente aberto | Entre D4 e GND, usa INPUT_PULLUP |

Ligue o motor em OUT1/OUT2 do L298N. Alimente o motor com fonte externa adequada à tensão e à corrente reais do motor. Não alimente o motor pelo 5 V do Uno. A corrente de partida/travamento precisa ser conferida: 7.400 rpm não determina corrente, torque nem compatibilidade com o driver. O datasheet do L298 especifica 2 A DC por canal como limite absoluto, sujeito às condições térmicas; isso não garante que um módulo comercial suporte essa corrente continuamente.

O jumper **ENA** e o jumper do regulador **5V-EN** são diferentes. A configuração de alimentação da lógica do L298N depende do módulo. Não una sua saída de regulador de 5 V ao 5 V USB do Arduino sem conferir o esquema do módulo. Use um resistor de aproximadamente 10 kΩ entre ENA e GND para manter o driver desabilitado durante reset, quando o pino do Uno fica em alta impedância. Disponibilize corte físico da alimentação do motor: o botão D4 e o comando serial dependem do software.

O sensor mede por sua conexão pneumática, ligada ao ponto de medição do circuito com uma derivação. Ele não precisa receber todo o fluxo. Evite líquido/condensação no sensor: a descrição comercial não comprova compatibilidade com água. Confira a resposta com pressão conhecida antes de acionar o motor.

## Primeira etapa — somente leitura

O arquivo vem com PWM, tempos de movimento e limiar iguais a zero e `CALIBRACAO_CONFERIDA=false`. Assim ele compila e lê o sensor, mas bloqueia a partida do motor. Isso evita atribuir valores fictícios à montagem.

1. Mantenha a alimentação do motor desligada e conecte o Uno por USB.
2. Abra o Monitor Serial em **115200 baud**. Aguarde a inicialização.
3. Observe `sensor_ok:1` e `raw`, que é a contagem digital sem unidade de pressão. `pressao_kPa:nan` significa que ainda não existe pressão válida/calibrada.
4. A cada 40 amostras, o programa imprime `# media_raw_40=...`, útil para observar o zero e coletar pontos de calibração. Não interprete `raw=0` com `sensor_ok:0` como pressão zero.
5. Se não houver dados, confira alimentação, GND, OUT e SCK. Não troque OUT por uma entrada analógica nem use I2C: o HX710B tem protocolo próprio de dados e clock.

## Calibração antes de usar pressão

Use um manômetro de referência e uma fonte de pressão controlada de bancada. Não use o motor desconhecido para gerar a primeira referência de calibração.

- Com diferença de pressão zero, registre várias médias estáveis e preencha `RAW_ZERO`.
- Aplique uma pressão conhecida dentro da faixa relevante ao ensaio, igual ou superior ao limiar que será usado. A partida bloqueia um limiar superior a essa referência para evitar extrapolação. Registre a leitura do manômetro em kPa em `PRESSAO_REFERENCIA_KPA` e a média digital correspondente em `RAW_REFERENCIA`.
- O programa calcula `p = (raw − RAW_ZERO) × PRESSAO_REFERENCIA_KPA / (RAW_REFERENCIA − RAW_ZERO)`.
- Confira outros pontos, subindo e descendo a pressão, e a repetibilidade. Dois pontos apenas definem uma reta; não comprovam a exatidão do conjunto.
- Só depois defina `CALIBRACAO_CONFERIDA=true` e reenvie o sketch. Os parâmetros ficam no código; não há salvamento automático em EEPROM.

Não use `map(raw, 0, 16777215, 0, 40)`: os extremos digitais não são automaticamente zero e 40 kPa. Os 24 bits do conversor também não equivalem à exatidão de pressão do módulo.

## Ajustes de movimento

Todos estão no início de `Pulmao_Bancada.ino`, no bloco `AJ`. A ordem é:

1. Limiar de pressão, em kPa.
2. Tempo máximo de compressão, em ms.
3. Tempo de retorno, em ms.
4. Pausa entre ciclos, em ms.
5. Intervalo sem acionamento antes de inverter, em ms.
6. Idade máxima da leitura, em ms.
7. PWM para comprimir, de 1 a 255.
8. PWM para retornar, de 1 a 255.

A pausa de 1.000 ms e a espera de inversão de 200 ms são valores iniciais de organização do programa, não parâmetros validados. O tempo de desaceleração depende do motor e da carga. Ajuste tempos e PWM na montagem com o orientador. Primeiro confira os sentidos sem a linha presa; `INVERTER_SENTIDO` troca a associação entre polaridade e compressão. PWM baixo não garante movimento; o motor pode não vencer o atrito. Não há medição da rotação real.

A implementação limita os tempos configuráveis de movimento a 10 s para evitar erros grosseiros de unidade; esse teto não significa que 10 s seja apropriado para a montagem. Não use o tempo máximo como substituto de limite mecânico.

## Comandos e tela

- `S`: inicia somente se a calibração e os ajustes forem válidos, houver leitura recente, pressão abaixo do limiar e nenhuma falha pendente.
- `X`: interrompe e reconhece falha. Não reinicia os ciclos.
- Botão D4 pressionado: para; ao soltar, não reinicia sozinho.

Os dados são exibidos no Monitor Serial e em formato compatível com o Plotter Serial. No Plotter, oculte `raw`, `estado`, `falha` e `sensor_ok` para comparar apenas `pressao_kPa` com `limite_kPa`. As linhas começadas com `#` são mensagens de diagnóstico; se a versão do Plotter não as ignorar, use inicialmente o Monitor Serial. Um display LCD/OLED físico ainda não foi escolhido e não faz parte desta versão.

Estados: 0 parado; 1 comprimindo; 2 intervalo de inversão; 3 retornando; 4 pausa; 5 falha. Falhas: 0 nenhuma; 1 sensor; 2 compressão excedeu o tempo; 3 pressão fora da faixa de verificação ou alta antes de reiniciar o ciclo.

A leitura usa o modo diferencial nominal de 40 amostras/s, com 27 pulsos por transferência. Descarta amostras de estabilização ao iniciar. O programa verifica saturação digital e atraso de leitura, mas não consegue identificar todo sensor defeituoso que continue enviando números plausíveis. Há watchdog de 250 ms; após reset o sistema permanece parado e exige nova partida. Seu comportamento físico depende também do bootloader e do circuito de desabilitação.

## Referências técnicas

- AVIA Semiconductor. HX710(A/B), datasheet, especialmente tabelas 2 e 3 e figura 2. https://bafnadevices.com/wp-content/uploads/2024/04/HX710B-SMD-Datasheet.pdf
- STMicroelectronics. L298, DS0218, revisão 5, 2023. https://www.st.com/resource/en/datasheet/l298.pdf
- Arduino. Uno Rev3. https://docs.arduino.cc/hardware/uno-rev3/
- Abo-Bakr Emara. Ventilator-, código original com motor temporizado. https://github.com/BakrEmara/Ventilator-

O código desta entrega é uma implementação nova; preserva os números de pinos do acionamento do projeto-base. Não foi executado no seu hardware. Veja VALIDACAO.txt para os testes realizados.
