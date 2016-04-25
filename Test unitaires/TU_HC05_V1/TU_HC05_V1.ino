bool conf = 0;

void setup() {

    pinMode(13, OUTPUT);
    digitalWrite(13,LOW);


  /*
    la broche KEY du module HC05 est connecté à la broche A4,
    utilisée en sortie digitale
  */
  pinMode(A4, OUTPUT);

  /*
    si KEY=0 au démarage, mode transmission
    si KEY=VCC au démarrage, mode AT cmd (config) , à 38400bauds
  */
  digitalWrite(A4, conf?HIGH:LOW);

  /*
     La broche 12 est connecté au RESET du HC05 : on reset le module pour passer d'un mode à l'autre
     car la broche KEY est testé au démarrage du module uniquement 'à verifier)
  */
  pinMode(12, OUTPUT);

  /*
     on effectue un reset en mettant la broche RESET du HC05 à GND
  */
  digitalWrite(12, LOW);
  delay(100);
  digitalWrite(12, HIGH);

  /*
     si on est en mode AT, le baudrate est de 38400
  */
  Serial1.begin(38400);

  //on attends que le moniteur série coté PC soit ouver
  while (!SerialUSB);

}

void loop() {
  //ce code est un "pass through" entre ce qui est envoyé/reçu sur le moniteur série et le HC05
  
  if (SerialUSB.available()) {
    char c = SerialUSB.read();
    Serial1.write(c);
  }
  if (Serial1.available()) {
    char b = Serial1.read();
    SerialUSB.write(b);
  }
}
