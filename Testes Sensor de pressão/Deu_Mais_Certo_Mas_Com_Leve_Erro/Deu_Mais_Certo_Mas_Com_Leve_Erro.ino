#define DT 3
#define SCK 2

// Alterado para float para manter a precisão das casas decimais
float OFFSET = 3194200.0; 
float SCALE = 5177853.0; // Ajuste este valor conforme a calibração

void setup() {
  Serial.begin(9600);
  pinMode(SCK, OUTPUT);
  pinMode(DT, INPUT);
}

long lerSensor() {
  while(digitalRead(DT)); // Espera o sensor ficar pronto

  unsigned long count = 0; // Usamos unsigned para a leitura bruta

  for(int i = 0; i < 24; i++) {
    digitalWrite(SCK, HIGH);
    count = count << 1;
    digitalWrite(SCK, LOW);
    if(digitalRead(DT)) count++;
  }

  // 25º pulso para definir o ganho (padrão 128)
  digitalWrite(SCK, HIGH);
  digitalWrite(SCK, LOW);

  // --- TRATAMENTO DE SINAL (24-bit para 32-bit) ---
  // Se o bit 23 (o 24º bit) for 1, o número é negativo
  if (count & 0x800000) {
    count |= 0xFF000000;
  }

  return (long)count;
}

void loop() {
  long leitura = lerSensor();

  // Cálculo da pressão
  // (Leitura Atual - Zero do Sensor) / Fator de Escala
  float psi = ((float)leitura - OFFSET) / SCALE;
  
  float valorFinal = psi * 5.8;

  if (valorFinal < 0) {
    valorFinal = 0.0;
  }

  Serial.print(leitura);

  delay(500);
}