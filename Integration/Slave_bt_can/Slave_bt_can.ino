//#define CONF
#define MASTER 0
#define SLAVE  1
#define MODE SLAVE
#define DELAY1 500
#define KEY A4
#define RS_HC 12
#define DEBUG 0

#include <Oleduino.h>
#include <SPIdma.h>
#include "fip_sprite.h"
#include "fip_hexSprites.h"

DMA dma;
Oleduino console;

//adresse de l'esclave :
//AT+ADDR
//+ADDR:2015:12:148184

//adresse du maitre :
//+ADDR:2016:2:164200

bool state = false;
uint32_t error = 0;

uint8_t dataBuffer[16][8] = {0};
SpriteInst numbers[272];

uint32_t ti = 0;
uint32_t cmpt = 0;
bool connection_OK = false;
uint32_t connection_timeout = 0;
bool already_connected = false;

String strBuffer[16];
uint32_t dispTimer = 0;
uint32_t dispTimer2 = 0;
uint32_t buttonTimer = 0;

uint8_t idDisplayBuffer[16][2] = // ID,not use
{
  {0x01, 1},
  {0x12, 2},
  {0x23, 3},
  {0x34, 4},
  {0x45, 5},
  {0x56, 6},
  {0x67, 7},
  {0x78, 8},
  {0x89, 9},
  {0x9A, 10},
  {0xAB, 11},
  {0xBC, 12},
  {0xCD, 13},
  {0xDE, 14},
  {0xEF, 15},
  {0xF8, 15}
};

void setup()
{
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  /*
    la broche KEY du module HC05 est connecté à la broche KEY,
    utilisée en sortie numerique
  */
  pinMode(KEY, OUTPUT);

  /*
    si KEY=0 au démarage, mode transmission
    si KEY=VCC au démarrage, mode AT cmd (config) , à 38400bauds
  */
  digitalWrite(KEY, HIGH);


  /*
     La broche 12 est connecté au RESET du HC05 : on reset le module pour passer d'un mode à l'autre
     car la broche KEY est testé au démarrage du module uniquement 'à verifier)
  */
  pinMode(RS_HC, OUTPUT);

  /*
     on effectue un reset en mettant la broche RESET du HC05 à GND
  */

  console.init();
  dma.init();

  initHexSprites();
  renderScreen(numbers, 272, RED);

  digitalWrite(RS_HC, LOW);
  delay(100);
  digitalWrite(RS_HC, HIGH);

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

  Serial1.print('A'); // Commande envoyee au master ACKNOWLEDGE
  //on attends que le moniteur série coté PC soit ouver
  //while (!SerialUSB);



}


void loop()
{
  //ce code est un "pass through" entre ce qui est envoyé/reçu sur le moniteur série et le HC05
  if (DEBUG)
  {
    if (SerialUSB.available())
    {
      char c = SerialUSB.read();
      if (c == '>') //go to cmd mode
      {
        digitalWrite(KEY, HIGH);
        digitalWrite(RS_HC, LOW);
        delay(100);
        digitalWrite(RS_HC, HIGH);
        Serial1.begin(38400);
        SerialUSB.println("\nCMD MODE");
      }
      else if (c == '<') //back to transmit mode
      {
        digitalWrite(KEY, LOW);
        digitalWrite(RS_HC, LOW);
        delay(100);
        digitalWrite(RS_HC, HIGH);
        Serial1.begin(115200);
        SerialUSB.println("\nTX/RX MODE");
      }
      else
      {
        Serial1.write(c);
      }

    }
  }
  if (Serial1.available())
  {

    char b = Serial1.read();
    String res = "";

    already_connected = true;

    if (b == '?')
    {
      Serial1.print('A'); // Commande envoyee au master ACKNOWLEDGE
      if (DEBUG)
        SerialUSB.println("A send");
      b = 0;
      connection_OK = true;
      connection_timeout = millis();
      //write_screen_at(1, "STATUS: CONNECTED");

    }

    if (b == '!')
    {
      cmpt++;
      while (!Serial1.available());
      //char CAN_id
      res += char(Serial1.read());
      while (!Serial1.available());
      //char len =
      res += char(Serial1.read());
      /*
            SerialUSB.print("ID:0x");
            SerialUSB.print(CAN_id, HEX);
            SerialUSB.print(" len:0x");
            SerialUSB.print(len, HEX);
      */
      uint8_t data[8];
      uint8_t recv_data;
      for (int i = 0; i < 8; i++)
      {
        while (!Serial1.available());

        //data[i] = Serial1.read();      // read CAN data
        //res += char(Serial1.read());
        recv_data = Serial1.read();
        data[i]= recv_data;
        res += char(recv_data);

        if (DEBUG)
          SerialUSB.print(" ");
        if (DEBUG)
          SerialUSB.print(data[i], HEX);
      }

      for (int k = 0; k < 16; k++) {
        if (idDisplayBuffer[k][0] == data[0]) {
          dataBuffer[k][0] = data[0];
          dataBuffer[k][1] = data[1];
          dataBuffer[k][2] = data[2];
          dataBuffer[k][3] = data[3];
          dataBuffer[k][4] = data[4];
          dataBuffer[k][5] = data[5];
          dataBuffer[k][6] = data[6];
          dataBuffer[k][7] = data[7];
        }
      }





      while (!Serial1.available());
      char end_char = Serial1.read();
      res += end_char;


      if (end_char == '#')
      {
        if (DEBUG)
          SerialUSB.println("VALID");
        if (DEBUG)
          SerialUSB.println(res);
        state = !state;
        digitalWrite(13, state);


        //write_screen_top(res);

      }
      else
      {
        if (DEBUG)
          SerialUSB.println("ERROR !!!");
        error++;

        //write_screen_top("ERROR");

        digitalWrite(13, LOW);
      }
      if (DEBUG)
        SerialUSB.println();
    }


  }

  if (millis() - dispTimer2 > 500)
  {
    dispTimer2 = millis();
    setNumbers();
    renderScreen(numbers, 272, BLACK);

    Serial1.print('A'); // Commande envoyee au master ACKNOWLEDGE
    if (DEBUG)
      SerialUSB.println("A send");
    connection_OK = true;
    connection_timeout = millis();
  }


  if (millis() - buttonTimer > 500)
  {
    buttonTimer = millis();

    read_button();
    if (DEBUG)
      SerialUSB.println("A send");
  }

  if (millis() - dispTimer > 1000)
  {
    dispTimer = millis();
    if (DEBUG)
      SerialUSB.println("\nDISPLAY NOW\n");
    //console.display.fillScreen(0);
    //console.display.setCursor(0, 0);



    for (int k = 0; k < 16 ; k++) {
      //console.display.println(strBuffer[k]);
    }

  }

  if ((millis() - connection_timeout > 3000) && already_connected)           // Check imeout du bluetooth toutes les 3 secondes
  {
    connection_timeout = millis() + 1500;

    if (DEBUG)
      SerialUSB.println("\nBT TIMEOUT, RESET NOW\n");

    //write_screen_at(1, "STATUS: DISCONNECTED");

    configBluetoothSlave();

    /*digitalWrite(KEY, LOW);
      digitalWrite(RS_HC, LOW);
      delay(100);
      digitalWrite(RS_HC, HIGH);*/
  }

}

void write_screen_top( String data) {

  for (int k = 15; k > 4; k--)
  {
    strBuffer[k] = strBuffer[k - 1];
  }
  strBuffer[4] = data;
}

void write_screen_at(int at , String data) {
  strBuffer[at] = data;
}

void read_button() {

  uint8_t data[5] = {0}, buttonValue = 0, Xjoy = 0, Yjoy = 0;
  bool buttonA = false, buttonB = false, buttonC = false;

  buttonA = console.A.isPressed();    // lecture de l'etat des bouttons
  buttonB = console.B.isPressed();
  buttonC = console.C.isPressed();

  console.joystick.read();            // lecture de la valeur des joysticks
  Xjoy = map(console.joystick.getRawX(), 0, 4095, 0, 255);
  Yjoy = map(console.joystick.getRawY(), 0, 4095, 0, 255);;

  if (buttonA) {
    buttonValue = buttonValue | 0b00000100;
    //data[0] = '!';
    //data[1] = 0x02;
    //data[2] = 0xFF;
    //data[3] = Ox41;
    //data[4] = '#';

  }
  if (buttonB)
    buttonValue = buttonValue | 0b00000010;
  if (buttonC)
    buttonValue = buttonValue | 0b00000001;

  data[0] = '!';
  data[1] = buttonValue;
  data[2] = Xjoy;
  data[3] = Yjoy;
  data[4] = '#';

  if (DEBUG) {
    SerialUSB.print("Read button: ");
    SerialUSB.print(data[1], DEC);
    SerialUSB.print(" ");
    SerialUSB.print(data[2], DEC);
    SerialUSB.print(" ");
    SerialUSB.print(data[3], DEC);
    SerialUSB.print("\n");
  }

  Serial1.write(data[0]);
  Serial1.write(data[1]);
  Serial1.write(data[2]);
  Serial1.write(data[3]);
  Serial1.write(data[4]);


}

void configBluetoothMaster()
{
  if (DEBUG)
    SerialUSB.print("Configuring master...");

  digitalWrite(13, HIGH);
  Serial1.println("AT+ORGL");
  delay(DELAY1);
  Serial1.println("AT+RMAAD");
  delay(DELAY1);
  Serial1.println("AT+NAME=BT_MASTER");
  delay(DELAY1);
  Serial1.println("AT+PSWD=0000");
  delay(DELAY1);
  Serial1.println("AT+ROLE=1");
  delay(DELAY1);
  Serial1.println("AT+CMODE=1");
  delay(DELAY1);
  Serial1.println("AT+INIT");
  delay(DELAY1);
  Serial1.println("AT+INQ");
  delay(DELAY1);
  Serial1.println("AT+LINK=2016,2,164200"); ////+ADDR:2016:2:164200    //2015,12,148184
  delay(DELAY1);
  Serial1.println("AT+UART=115200,1,0");
  delay(DELAY1);

  while (Serial1.find("FAIL"))
  {

    if (DEBUG)
      SerialUSB.println("Can't connect to slave : AT+LINK return FAIL");
  }
  while (Serial1.available())
    Serial1.read();

  digitalWrite(KEY, LOW);
  digitalWrite(RS_HC, LOW);
  delay(100);
  digitalWrite(RS_HC, HIGH);
  digitalWrite(13, LOW);
  if (DEBUG)
    SerialUSB.println("OK");
}

void configBluetoothSlave()
{

  //write_screen_at(0, "MODE: SLAVE");

  if (already_connected) {                          // mecanisme permettant de determiner si la fonction est utilisée pour la config initiale ou la reconnection au robot
    //write_screen_at(1, "STATUS: TRY RECONNECT..."); // reconnection
    already_connected = false;
  } else {
    //write_screen_at(1, "STATUS: CONNECTING...");    //connection initiale
  }

  if (DEBUG)
    SerialUSB.print("Configuring slave...");
  digitalWrite(13, HIGH);
  Serial1.println("AT+ORGL");
  delay(DELAY1);
  Serial1.println("AT+NAME=BT_SLAVE");
  delay(DELAY1);
  Serial1.println("AT+PSWD=0000");
  delay(DELAY1);
  Serial1.println("AT+ROLE=0");
  delay(DELAY1);
  Serial1.println("AT+UART=115200,1,0");
  delay(DELAY1);

  while (Serial1.available())
    Serial1.read();

  digitalWrite(KEY, LOW);
  digitalWrite(RS_HC, LOW);
  delay(100);
  digitalWrite(RS_HC, HIGH);
  digitalWrite(13, LOW);
  if (DEBUG)
    SerialUSB.println("OK");
}

void initHexSprites()
{
  for (int i = 0; i < 272; i++)
  {
    if (i % 17 == 2)
      numbers[i].sprite = &s_Oxdot;
    else
      numbers[i].sprite = &s_OxF;
    numbers[i].enabled = true;;
    numbers[i].x = (i % 17) * 6;
    numbers[i].y = i / 17 * 8;
  }
}

void setNumbers()
{
  for (int line = 0; line < 16; line++)
  {
    for (int column = 0; column < 8; column++)
    {
      uint8_t LB = dataBuffer[line][column] & 0x0F;
      uint8_t HB = (dataBuffer[line][column] >> 4) & 0x0F;
      //uint8_t col = column > 0 ? 2*(column + 1) : 2*column;
      uint8_t col;
      if (column > 0)
        col = 2 * column + 1;
      else
        col = 2 * column;
      switch (HB)
      {
        case 0x0:
          numbers[col + line * 17].sprite = &s_Ox0;
          break;
        case 0x1:
          numbers[col + line * 17].sprite = &s_Ox1;
          break;
        case 0x2:
          numbers[col + line * 17].sprite = &s_Ox2;
          break;
        case 0x3:
          numbers[col + line * 17].sprite = &s_Ox3;
          break;
        case 0x4:
          numbers[col + line * 17].sprite = &s_Ox4;
          break;
        case 0x5:
          numbers[col + line * 17].sprite = &s_Ox5;
          break;
        case 0x6:
          numbers[col + line * 17].sprite = &s_Ox6;
          break;
        case 0x7:
          numbers[col + line * 17].sprite = &s_Ox7;
          break;
        case 0x8:
          numbers[col + line * 17].sprite = &s_Ox8;
          break;
        case 0x9:
          numbers[col + line * 17].sprite = &s_Ox9;
          break;
        case 0xA:
          numbers[col + line * 17].sprite = &s_OxA;
          break;
        case 0xB:
          numbers[col + line * 17].sprite = &s_OxB;
          break;
        case 0xC:
          numbers[col + line * 17].sprite = &s_OxC;
          break;
        case 0xD:
          numbers[col + line * 17].sprite = &s_OxD;
          break;
        case 0xE:
          numbers[col + line * 17].sprite = &s_OxE;
          break;
        case 0xF:
          numbers[col + line * 17].sprite = &s_OxF;
          break;
        default:
          numbers[col + line * 17].sprite = &s_Oxdot;
          break;
      }
      switch (LB)
      {
        case 0x0:
          numbers[col + (line * 17) + 1].sprite = &s_Ox0;
          break;
        case 0x1:
          numbers[col + (line * 17) + 1].sprite = &s_Ox1;
          break;
        case 0x2:
          numbers[col + line * 17 + 1].sprite = &s_Ox2;
          break;
        case 0x3:
          numbers[col + line * 17 + 1].sprite = &s_Ox3;
          break;
        case 0x4:
          numbers[col + line * 17 + 1].sprite = &s_Ox4;
          break;
        case 0x5:
          numbers[col + line * 17 + 1].sprite = &s_Ox5;
          break;
        case 0x6:
          numbers[col + line * 17 + 1].sprite = &s_Ox6;
          break;
        case 0x7:
          numbers[col + line * 17 + 1].sprite = &s_Ox7;
          break;
        case 0x8:
          numbers[col + line * 17 + 1].sprite = &s_Ox8;
          break;
        case 0x9:
          numbers[col + line * 17 + 1].sprite = &s_Ox9;
          break;
        case 0xA:
          numbers[col + line * 17 + 1].sprite = &s_OxA;
          break;
        case 0xB:
          numbers[col + line * 17 + 1].sprite = &s_OxB;
          break;
        case 0xC:
          numbers[col + line * 17 + 1].sprite = &s_OxC;
          break;
        case 0xD:
          numbers[col + line * 17 + 1].sprite = &s_OxD;
          break;
        case 0xE:
          numbers[col + line * 17 + 1].sprite = &s_OxE;
          break;
        case 0xF:
          numbers[col + line * 17 + 1].sprite = &s_OxF;
          break;
        default:
          numbers[col + line * 17 + 1].sprite = &s_Oxdot;
          break;
      }
    }
  }
}





