#define S1 16
#define S2 17
#define boton 21

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(boton, INPUT_PULLUP);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (!digitalRead(boton)) Serial.println("ON");
  digitalWrite(S1,HIGH);
  digitalWrite(S2,HIGH);
  delay(250);
  digitalWrite(S2,LOW);
  delay(250);
  digitalWrite(S1,LOW);
  digitalWrite(S2,HIGH);
  delay(250);
  digitalWrite(S2,LOW);
  delay(250);
}
