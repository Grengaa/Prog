# Arduino Plotter em C++

Aplicativo desktop para Windows com Qt 6: lê contagens RAW do HX710B enviadas pelo Arduino e desenha pressão em mmHg ao longo do tempo, usando calibração por dois pontos. Também pode ser compilado em Linux com Qt 6.

## Compilar no Windows

1. Instale o Qt 6 pelo instalador oficial: https://www.qt.io/download-qt-installer-oss.
2. Selecione **Qt 6 para MinGW 64-bit**, o módulo **Qt Serial Port**, **Qt Creator**, **CMake** e o compilador **MinGW** correspondente ao kit. Widgets faz parte do Qt Base. Use componentes da mesma versão e arquitetura.
3. Abra `CMakeLists.txt` no Qt Creator, selecione o kit Desktop Qt 6 MinGW e clique em **Run**. A interface é criada diretamente em C++; não há arquivo `.ui`.

Alternativa no terminal do kit Qt (com CMake, Ninja e o compilador no PATH):

```powershell
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/mingw_64"
cmake --build build
ctest --test-dir build --output-on-failure
.\build\ArduinoPlotter.exe
```

Substitua o caminho pela versão instalada. Para distribuir o executável, execute `windeployqt build/ArduinoPlotter.exe` no terminal do mesmo kit e distribua a pasta com as DLLs geradas.

## Sensor de pressão com HX710B

A aplicação recebe **um inteiro RAW por linha**, terminado por `\n` ou `\r\n`, a 9600 baud. O Arduino precisa ler o HX710B com a biblioteca e a ligação correspondentes ao seu módulo e enviar apenas `Serial.println(leituraRaw);`. Não use `analogRead()` para ler a saída digital do HX710B. Textos, unidades, cabeçalhos e múltiplas colunas são rejeitados no modo pressão.

O exemplo `examples/ArduinoCsv` é o exemplo antigo de entradas analógicas: **não serve para o HX710B**. O sketch específico depende dos pinos, da biblioteca e do código que você usa na placa.

### Calibrar em mmHg

1. Feche o Monitor Serial da IDE Arduino, selecione a COM no aplicativo e conecte a 9600 baud (ou à velocidade usada no sketch).
2. Observe o campo **RAW**, que funciona mesmo sem calibração.
3. Com uma pressão de referência conhecida e estável, anote RAW 1 e a pressão correspondente em mmHg 1. Repita em outra pressão conhecida para RAW 2/mmHg 2. Use referências diferentes e suficientemente separadas na faixa de trabalho.
4. Preencha os quatro campos e clique em **Aplicar calibração**. O gráfico e a leitura atual passam a mostrar mmHg.

A conversão linear é:

```text
pressão = P1 + (RAW − RAW1) × (P2 − P1) / (RAW2 − RAW1)
```

Não há coeficiente universal RAW → mmHg para o HX710B: ele depende da ponte sensora, do ganho e do módulo. A faixa anunciada de **0–40 kPa equivale a aproximadamente 0–300,025 mmHg**, mas não informa quais contagens RAW representam esses extremos. Para referências em kPa, use `mmHg = kPa × 7,50061683`.

Se o sensor mede pressão relativa/diferencial, o resultado calibrado também será relativo/diferencial; a aplicação não soma automaticamente a pressão atmosférica. Confirme esse tipo de pressão no datasheet do módulo. Não conclua que ele mede pressão absoluta apenas pelo nome comercial “barometric”. A validade da aproximação linear deve ser conferida com referências intermediárias.

A calibração vale para a sessão atual; aplique-a novamente ao reabrir o programa. Alterar qualquer referência invalida a conversão e limpa o gráfico até aplicar uma nova calibração. Alterações de sensor, ganho ou formato RAW exigem nova calibração.

### Gráfico e controles

- Gráfico de **pressão em mmHg × tempo em segundos**, com as últimas 1000 amostras e uma única curva.
- A escala vertical é **fixa**, inicialmente 0–300,025 mmHg. Altere mínima/máxima e clique em **Aplicar escala** para usar outros limites, inclusive negativos.
- Leituras fora da escala não são limitadas artificialmente: a leitura numérica permanece e aparece um aviso; a curva é recortada nos limites do gráfico.
- **Pausar** congela o gráfico e a leitura de pressão; a serial continua sendo lida, e RAW continua atualizado. Os dados durante a pausa são descartados da visualização.
- **Limpar** zera o gráfico, o tempo e os contadores, preservando a calibração aplicada.
- **Demonstração** desconecta a serial e gera uma pressão fictícia em torno de 150 mmHg, identificada como SIMULAÇÃO. Não utiliza nem calibra o sensor.

O tempo é registrado pelo computador no recebimento, não pelo Arduino. Feche outros programas que usam a mesma COM antes de conectar. A placa pode reiniciar ao abrir a porta.

## Verificação

O teste `parser_test` cobre números, decimais, espaços, CRLF, notação científica e rejeição de dados inválidos. Também pode ser executado sem Qt:

```sh
g++ -std=c++17 -Wall -Wextra -pedantic tests/parser_test.cpp -o /tmp/arduino-parser-test
/tmp/arduino-parser-test
```

Para validar a aplicação completa: execute a demonstração, confira a curva de pressão simulada e os botões de pausa e limpeza; depois conecte a placa e confira o RAW e as pressões contra o instrumento de referência. A compilação Windows e a conexão com hardware precisam ser verificadas em uma máquina Windows com Qt e Arduino disponíveis.

## Organização orientada a objetos

- `MainWindow` (`src/mainwindow.hpp/.cpp`): representa a janela e coordena os controles, a porta serial, os temporizadores e os contadores. Seu estado é privado.
- `Plot` (`src/plot.hpp`): representa o gráfico, guarda até 1000 amostras em um contêiner privado e oferece `add()`, `clear()` e consulta somente de leitura `data()`.
- `SerialCsvBuffer` (`src/serialcsvbuffer.hpp`): reconstrói linhas recebidas em fragmentos e encaminha amostras válidas ou erros para quem o utiliza. Não depende da interface gráfica.
- `PressureCalibration` (`src/pressurecalibration.hpp`): valida dois pontos de referência e converte RAW em mmHg sem depender do Qt.
- `main.cpp`: cria a aplicação e a janela.

A **composição** aparece em `MainWindow`, que contém um `Plot` e um `SerialCsvBuffer`. A **herança** aparece em `MainWindow` e `Plot`, que derivam de `QWidget`. O método `Plot::paintEvent()` usa **polimorfismo**, sobrescrevendo o método virtual de desenho do Qt. Os atributos privados demonstram **encapsulamento**.

Além do parser, `serialcsvbuffer_test` verifica linhas fragmentadas, várias linhas no mesmo bloco, recuperação após excesso de tamanho e limpeza do estado. O teste `pressurecalibration_test` verifica os pontos de referência, interpolação, inclinação invertida e rejeição de calibrações inválidas. Os três testes são registrados no CTest.
