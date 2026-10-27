// Selecione a placa e a porta corretas na IDE Arduino antes de carregar.
// Três colunas, uma linha por amostra, a 20 amostras por segundo.
void setup() {
  Serial.begin(9600);
}

void loop() {
  Serial.print(analogRead(A0));
  Serial.print(',');
  Serial.print(analogRead(A1));
  Serial.print(',');
  Serial.println(analogRead(A2));
  delay(50);
}
