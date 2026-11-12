#define DATA 3
#define CLOCK 2

long readHX710B() {
  while (digitalRead(DATA)); 
  long value = 0;
  for (int i = 0; i < 24; i++) {
    digitalWrite(CLOCK, HIGH);
    value = value << 1;
    digitalWrite(CLOCK, LOW);
    if (digitalRead(DATA)) value++;
  }
  digitalWrite(CLOCK, HIGH);
  digitalWrite(CLOCK, LOW);
  if (value & 0x800000) value |= 0xFF000000;
  return value;
}

void setup() {
  pinMode(DATA, INPUT);
  pinMode(CLOCK, OUTPUT);
  
  Serial.begin(9600); // PLX-DAQ costuma funcionar melhor em 9600 ou 128000

  // COMANDOS PLX-DAQ:
  Serial.println("CLEARDATA"); // Limpa a planilha se houver dados antigos
  Serial.println("LABEL,Hora,Leitura Pressao"); // Define o cabeçalho das colunas
}

void loop() {
  long pressure = readHX710B();

  // COMANDO PARA ENVIAR DADOS:
  // "DATA" avisa que vem informação, "TIME" coloca a hora do PC automaticamente
  Serial.print("DATA,TIME,"); 
  Serial.println(pressure); 

  delay(500); // Aumentei um pouco para não travar o Excel com excesso de dados
}