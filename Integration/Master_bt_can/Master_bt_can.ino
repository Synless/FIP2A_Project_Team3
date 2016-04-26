
#include <SPI.h>
#include <Oleduino.h>
#include <SPIdma.h>
DMA dma;
#include "mcp_can.h"
#include "fip_sprite.h"
#include "fip_hexSprites.h"

const int SPI_CS_PIN = 7;

MCP_CAN CAN(SPI_CS_PIN);    // Set CS pin

Oleduino console;

String strBuffer[16];
uint8_t dataBuffer[16][9];

uint32_t dispTimer = 0;
bool connection = false;

bool debug = true;

SpriteInst numbers[304];
uint32_t dispTimer2 = 0;


uint8_t idCntBuffer[16][3] = // ID,COUNT,counter
{
  {0x01, 1, 0},
  {0x12, 2, 0},
  {0x23, 3, 0},
  {0x34, 4, 0},
  {0x45, 5, 0},
  {0x56, 6, 0},
  {0x67, 7, 0},
  {0x78, 8, 0},
  {0x89, 9, 0},
  {0x9A, 10, 0},
  {0xAB, 11, 0},
  {0xBC, 12, 0},
  {0xCD, 13, 0},
  {0xDE, 14, 0},
  {0xEF, 15, 0},
  {0xF8, 15, 0}
};

void setup()
{
  pinMode(A4, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(PIN_LED_RXL, OUTPUT);

  console.init();
  dma.init();

  initHexSprites();
  renderScreen(numbers, 304, RED);


  if (debug)while (!SerialUSB);

  digitalWrite(12, LOW);
  delay(100);
  digitalWrite(12, HIGH);
  Serial1.begin(38400);
  configBluetoothMaster();
  Serial1.begin(115200);

  while (CAN_OK != CAN.begin(CAN_250KBPS))              // init can bus : baudrate = 500k
  {
    if (debug)SerialUSB.println("CAN BUS Shield init fail");
    if (debug)SerialUSB.println(" Init CAN BUS Shield again");
    delay(100);
  }
  if (debug)
    if (debug)SerialUSB.println("CAN BUS Shield init ok!");


}


void loop()
{
  uint8_t len = 0;
  uint8_t buf[8];
  uint8_t dest = 0, ID;
  char msg[12] = {0};

  digitalWrite(13, LOW);

  if (CAN_MSGAVAIL == CAN.checkReceive())   // check if CAN data available
  {
    CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf
    dest = CAN.getCanId();

    ID = buf[0];  //robot peripheral's ID.

    //check if data must be sent or discarded
    for (int k = 0; k < 16; k++)
    {
      if (idCntBuffer[k][0] == ID)
      {
        digitalWrite(13, HIGH);
        if (idCntBuffer[k][2] == 0) //counter is 0 => we need to send
        {
          idCntBuffer[k][2] = idCntBuffer[k][1]; //reset counter

          dataBuffer[k][0] = ID;
          // 1. fill message buffer
          msg[0] = '!';
          msg[1] = dest;
          msg[2] = len;
          for (int i = 0; i < len; i++) // print the data
          {
            msg[3 + i] = buf[i];
            dataBuffer[k][i + 1] = buf[i];

          }
          msg[11] = '#';

          // 2. send to bluetooth module if connection is OK
          if (connection)
            Serial1.print(msg);

          // 3. update displayable values
          for (int k = 15; k > 0; k--)
          {
            strBuffer[k] = strBuffer[k - 1];
          }
          strBuffer[0] = msg;

          //print on Serial port
          for (int i = 0; i < 12; i++)
          {
            if (debug)SerialUSB.print(" 0x");
            if (debug)SerialUSB.print(msg[i], HEX);
          }
          if (debug)SerialUSB.println();
        }
        idCntBuffer[k][2]--;
        break;
      }
    }

    //If we execute following line, it means unknown device ID has been received
    //so we always send this value, in case it is critical
    msg[0] = '!';
    msg[1] = dest;
    msg[2] = len;
    for (int i = 0; i < len; i++) // print the data
    {
      msg[3 + i] = buf[i];
    }
    msg[11] = '#';

    //send to bluetooth
    //Serial1.print(msg);

    //print on serial port
    /*
      for (int i = 0; i < 12; i++)
      {
      if (debug)SerialUSB.print(" 0x");
      if (debug)SerialUSB.print(msg[i], HEX);
      }
      if (debug)SerialUSB.println();
    */
  } //end receive data from can

  if (millis() - dispTimer2 > 500)
  {
    dispTimer2 = millis();
    setNumbers();
    renderScreen(numbers, 304, BLACK);
  }

  //each second, update display, check connection
  if (millis() - dispTimer > 1000)
  {
    dispTimer = millis();
    connection = false;

    //verify connection
    if (Serial1.available())
    {
      digitalWrite(PIN_LED_RXL, LOW);
      char ch = Serial1.read();
      if (ch == 'A') //ack == connection OK
        connection = true;
      while (Serial1.available())
        Serial1.read();
    }
    else
    {
      digitalWrite(PIN_LED_RXL, HIGH);
    }
    Serial1.write('?'); //ask for connection status
    SerialUSB.println('?');


    /*
      console.display.setCursor(0, 0);
      for (int l = 0; l < 16; l++)
      {
      clearBand(l * 8, 8, connection ? BLACK : RED);
      console.display.println(strBuffer[l]);
      }
    */


  }

}



void configBluetoothMaster()
{
  if (debug)SerialUSB.print("Configuring BT master...");
  digitalWrite(13, HIGH);
  Serial1.println("AT+ORGL");
  if (debug)SerialUSB.print(".");
  delay(500);
  Serial1.println("AT+RMAAD");
  if (debug)SerialUSB.print(".");
  delay(500);
  Serial1.println("AT+NAME=BT_MASTER");
  if (debug)SerialUSB.print(".");
  delay(500);
  Serial1.println("AT+PSWD=0000");
  if (debug)SerialUSB.print(".");
  delay(500);
  Serial1.println("AT+ROLE=1");
  if (debug)SerialUSB.print(".");
  delay(500);
  Serial1.println("AT+CMODE=1");
  if (debug)SerialUSB.print(".");
  delay(500);
  Serial1.println("AT+INIT");
  if (debug)SerialUSB.print(".");
  delay(500);
  Serial1.println("AT+INQ");
  if (debug)SerialUSB.print(".");
  delay(500);
  Serial1.println("AT+LINK=2016,2,164200"); ////+ADDR:2016:2:164200    //2015,12,148184
  if (debug)SerialUSB.print(".");
  delay(500);
  if (debug)SerialUSB.print(".");
  Serial1.println("AT+UART=115200,1,0");
  delay(500);

  while (Serial1.find("FAIL"))
  {
    if (debug)SerialUSB.println("Can't connect to slave : AT+LINK return FAIL");
  }
  while (Serial1.available())
    Serial1.read();

  digitalWrite(A4, LOW);
  digitalWrite(12, LOW);
  delay(100);
  digitalWrite(12, HIGH);
  digitalWrite(13, LOW);

  if (debug)SerialUSB.println("OK");
}

void initHexSprites()
{
  for (int i = 0; i < 304; i++)
  {
    if (i % 19 == 2)
      numbers[i].sprite = &s_Oxdot;
    else
      numbers[i].sprite = &s_OxF;

    numbers[i].enabled = true;;
    numbers[i].x = (i % 19) * 6;
    numbers[i].y = i / 19 * 8;
  }
}

void setNumbers()
{

  for (int line = 0; line < 16; line++)
  {
    for (int column = 0; column < 9; column++)
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
          numbers[col + line * 19].sprite = &s_Ox0;
          break;
        case 0x1:
          numbers[col + line * 19].sprite = &s_Ox1;
          break;
        case 0x2:
          numbers[col + line * 19].sprite = &s_Ox2;
          break;
        case 0x3:
          numbers[col + line * 19].sprite = &s_Ox3;
          break;
        case 0x4:
          numbers[col + line * 19].sprite = &s_Ox4;
          break;
        case 0x5:
          numbers[col + line * 19].sprite = &s_Ox5;
          break;
        case 0x6:
          numbers[col + line * 19].sprite = &s_Ox6;
          break;
        case 0x7:
          numbers[col + line * 19].sprite = &s_Ox7;
          break;
        case 0x8:
          numbers[col + line * 19].sprite = &s_Ox8;
          break;
        case 0x9:
          numbers[col + line * 19].sprite = &s_Ox9;
          break;
        case 0xA:
          numbers[col + line * 19].sprite = &s_OxA;
          break;
        case 0xB:
          numbers[col + line * 19].sprite = &s_OxB;
          break;
        case 0xC:
          numbers[col + line * 19].sprite = &s_OxC;
          break;
        case 0xD:
          numbers[col + line * 19].sprite = &s_OxD;
          break;
        case 0xE:
          numbers[col + line * 19].sprite = &s_OxE;
          break;
        case 0xF:
          numbers[col + line * 19].sprite = &s_OxF;
          break;
        default:
          numbers[col + line * 19].sprite = &s_Oxdot;
          break;
      }

      switch (LB)
      {
        case 0x0:
          numbers[col + (line * 19) + 1].sprite = &s_Ox0;
          break;
        case 0x1:
          numbers[col + (line * 19) + 1].sprite = &s_Ox1;
          break;
        case 0x2:
          numbers[col + line * 19 + 1].sprite = &s_Ox2;
          break;
        case 0x3:
          numbers[col + line * 19 + 1].sprite = &s_Ox3;
          break;
        case 0x4:
          numbers[col + line * 19 + 1].sprite = &s_Ox4;
          break;
        case 0x5:
          numbers[col + line * 19 + 1].sprite = &s_Ox5;
          break;
        case 0x6:
          numbers[col + line * 19 + 1].sprite = &s_Ox6;
          break;
        case 0x7:
          numbers[col + line * 19 + 1].sprite = &s_Ox7;
          break;
        case 0x8:
          numbers[col + line * 19 + 1].sprite = &s_Ox8;
          break;
        case 0x9:
          numbers[col + line * 19 + 1].sprite = &s_Ox9;
          break;
        case 0xA:
          numbers[col + line * 19 + 1].sprite = &s_OxA;
          break;
        case 0xB:
          numbers[col + line * 19 + 1].sprite = &s_OxB;
          break;
        case 0xC:
          numbers[col + line * 19 + 1].sprite = &s_OxC;
          break;
        case 0xD:
          numbers[col + line * 19 + 1].sprite = &s_OxD;
          break;
        case 0xE:
          numbers[col + line * 19 + 1].sprite = &s_OxE;
          break;
        case 0xF:
          numbers[col + line * 19 + 1].sprite = &s_OxF;
          break;
        default:
          numbers[col + line * 19 + 1].sprite = &s_Oxdot;
          break;
      }
    }
  }
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
