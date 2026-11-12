#define DT 3
#define SCK 2

long OFFSET = 3210753.74; //LEITURA SEM PRESSÃO
long SCALE = 5177853; // LEITURAS PRESSÃO CONHECIDA - SEM PRESSÃO

void setup() {
  Serial.begin(9600);
  pinMode(SCK, OUTPUT);
  pinMode(DT, INPUT);
}

long lerSensor() {

  while(digitalRead(DT));

  long count = 0;

  for(int i = 0; i < 24; i++) {
    digitalWrite(SCK, HIGH);
    count = count << 1;
    digitalWrite(SCK, LOW);

    if(digitalRead(DT))
      count++;
  }

  digitalWrite(SCK, HIGH);
  digitalWrite(SCK, LOW);

  return count;
}

void loop() {

  long leitura = lerSensor();

  float psi = ((float)(leitura - OFFSET) / SCALE) * 5.8;

  Serial.print("Pressao: ");
  Serial.print(psi);
  Serial.println(" PSI");

  delay(500);
}