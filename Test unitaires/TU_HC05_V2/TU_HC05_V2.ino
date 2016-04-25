//#define CONF
#define MASTER 0
#define SLAVE  1
#define MODE MASTER

//adresse de l'esclave :
//AT+ADDR
//+ADDR:2015:12:148184

//adresse du maitre :
//+ADDR:2016:2:164200

void setup() {

  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);


  /*
    la broche KEY du module HC05 est connecté à la broche A4,
    utilisée en sortie digitale
  */
  pinMode(A4, OUTPUT);

  /*
    si KEY=0 au démarage, mode transmission
    si KEY=VCC au démarrage, mode AT cmd (config) , à 38400bauds
  */
  digitalWrite(A4, HIGH);


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

#if MODE //slave
  configBluetoothSlave();
#else
  configBluetoothMaster();
#endif
  Serial1.begin(115200);

  //on attends que le moniteur série coté PC soit ouver
  while (!SerialUSB);

}

void loop() {
  //ce code est un "pass through" entre ce qui est envoyé/reçu sur le moniteur série et le HC05

  if (SerialUSB.available()) {
    char c = SerialUSB.read();
    if (c == '>') //go to cmd mode
    {
      digitalWrite(A4, HIGH);
      digitalWrite(12, LOW);
      delay(100);
      digitalWrite(12, HIGH);
      Serial1.begin(38400);
      SerialUSB.println("\nCMD MODE");
    }
    else if (c == '<') //back to transmit mode
    {
      digitalWrite(A4, LOW);
      digitalWrite(12, LOW);
      delay(100);
      digitalWrite(12, HIGH);
      Serial1.begin(115200);
      SerialUSB.println("\nTX/RX MODE");
    }
    else
      Serial1.write(c);
  }
  if (Serial1.available()) {
    char b = Serial1.read();
    SerialUSB.write(b);
  }
}


void configBluetoothMaster()
{
  SerialUSB.print("Configuring master...");

  digitalWrite(13, HIGH);
  Serial1.println("AT+ORGL");
  delay(500);
  Serial1.println("AT+RMAAD");
  delay(500);
  Serial1.println("AT+NAME=BT_MASTER");
  delay(500);
  Serial1.println("AT+PSWD=0000");
  delay(500);
  Serial1.println("AT+ROLE=1");
  delay(500);
  Serial1.println("AT+CMODE=1");
  delay(500);
  Serial1.println("AT+INIT");
  delay(500);
  Serial1.println("AT+INQ");
  delay(500);
  Serial1.println("AT+LINK=2016,2,164200"); ////+ADDR:2016:2:164200    //2015,12,148184
  delay(500);
  Serial1.println("AT+UART=115200,1,0");
  delay(500);

  while (Serial1.find("FAIL"))
  {
    SerialUSB.println("Can't connect to slave : AT+LINK return FAIL");
  }
  while (Serial1.available())
    Serial1.read();

  digitalWrite(A4, LOW);
  digitalWrite(12, LOW);
  delay(100);
  digitalWrite(12, HIGH);
  digitalWrite(13, LOW);

  SerialUSB.println("OK");
}

void configBluetoothSlave()
{
  SerialUSB.print("Configuring slave...");
  digitalWrite(13, HIGH);
  Serial1.println("AT+ORGL");
  delay(500);
  Serial1.println("AT+NAME=BT_SLAVE");
  delay(500);
  Serial1.println("AT+PSWD=0000");
  delay(500);
  Serial1.println("AT+ROLE=0");
  delay(500);
  Serial1.println("AT+UART=115200,1,0");
  delay(500);

  while (Serial1.available())
    Serial1.read();

  digitalWrite(A4, LOW);
  digitalWrite(12, LOW);
  delay(100);
  digitalWrite(12, HIGH);
  digitalWrite(13, LOW);

  SerialUSB.println("OK");

}

