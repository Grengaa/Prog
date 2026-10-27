# Arduino Plotter em C++

Aplicativo desktop para Windows com Qt 6: lê a porta serial do Arduino e desenha um gráfico em tempo real, com uma série por coluna CSV. Também pode ser compilado em Linux com Qt 6.

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

## Usar com o Arduino

1. Carregue o exemplo em `examples/ArduinoCsv/ArduinoCsv.ino` na placa.
2. **Feche o Monitor Serial e o Plotter Serial da IDE Arduino**: eles disputam a porta com o aplicativo.
3. Abra o programa, clique em **Atualizar portas**, selecione a COM do Arduino, mantenha **9600** e clique em **Conectar**.
4. Cada coluna aparece como uma série colorida. O eixo horizontal mostra segundos desde a conexão; o vertical ajusta a escala automaticamente. O gráfico mantém as últimas 1000 amostras e redesenha até cerca de 30 vezes por segundo.

Formato esperado (até 16 colunas, sempre na mesma ordem):

```text
512,300,650
515,302,648
520,305,645
```

Cada linha termina em `\n` ou `\r\n`. Use **ponto** para casas decimais (`23.5,40.2`), sem nomes de coluna, unidades ou timestamp. Cada linha representa uma amostra; o tempo é registrado pelo computador quando ela chega, não pelo relógio do Arduino. Dados recebidos em lote podem ter tempos muito próximos. Valores não numéricos, infinitos, campos vazios e linhas maiores que 4096 bytes são descartados e contabilizados. Séries com escalas muito diferentes compartilham o mesmo eixo vertical.

**Pausar** congela a visualização e descarta as novas amostras enquanto a porta continua sendo lida. **Retomar** volta a adicionar dados; **Limpar** zera gráfico, tempo e contadores. **Demonstração** desconecta a porta e gera três ondas locais, permitindo testar a interface sem uma placa. Ela não valida a comunicação USB.

A placa pode reiniciar ao abrir a porta; aguarde alguns segundos. Se a COM não aparecer, confira o cabo USB de dados e o driver da placa. A velocidade deve corresponder a `Serial.begin(9600)` no sketch.

## Verificação

O teste `parser_test` cobre números, decimais, espaços, CRLF, notação científica e rejeição de dados inválidos. Também pode ser executado sem Qt:

```sh
g++ -std=c++17 -Wall -Wextra -pedantic tests/parser_test.cpp -o /tmp/arduino-parser-test
/tmp/arduino-parser-test
```

Para validar a aplicação completa: execute a demonstração, confira as três curvas e os botões de pausa e limpeza; depois conecte a placa e confira as leituras A0, A1 e A2. A compilação Windows e a conexão com hardware precisam ser verificadas em uma máquina Windows com Qt e Arduino disponíveis.

## Organização orientada a objetos

- `MainWindow` (`src/mainwindow.hpp/.cpp`): representa a janela e coordena os controles, a porta serial, os temporizadores e os contadores. Seu estado é privado.
- `Plot` (`src/plot.hpp`): representa o gráfico, guarda até 1000 amostras em um contêiner privado e oferece `add()`, `clear()` e consulta somente de leitura `data()`.
- `SerialCsvBuffer` (`src/serialcsvbuffer.hpp`): reconstrói linhas recebidas em fragmentos e encaminha amostras válidas ou erros para quem o utiliza. Não depende da interface gráfica.
- `main.cpp`: cria a aplicação e a janela.

A **composição** aparece em `MainWindow`, que contém um `Plot` e um `SerialCsvBuffer`. A **herança** aparece em `MainWindow` e `Plot`, que derivam de `QWidget`. O método `Plot::paintEvent()` usa **polimorfismo**, sobrescrevendo o método virtual de desenho do Qt. Os atributos privados demonstram **encapsulamento**.

Além do parser, `serialcsvbuffer_test` verifica linhas fragmentadas, várias linhas no mesmo bloco, recuperação após excesso de tamanho e limpeza do estado. Ambos os testes são registrados no CTest.
