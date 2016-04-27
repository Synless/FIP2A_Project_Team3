#include <Oleduino.h>
#include <SPIdma.h>
#include "mcp_can.h"
#include "fip_sprite.h"
#include "fip_hexSprites.h"

DMA dma;
MCP_CAN CAN(7);    // Set CS pin
Oleduino console;

String strBuffer[16];
uint8_t dataBuffer[16][8] = {0x00};
uint8_t canSendBuffer[8]; //to send data on the can bus
uint8_t receiveBuffer[3]; //to get data from BT

uint32_t dispTimer = 0;
bool connection = false;

bool debug = true; //if debug is true, you MUST keep serial port open for normal operation

bool rxState = 0, txState = 0;

SpriteInst numbers[272];  //all sprites' numbers : 2 for ID, 1 for ':', 14 for data = 17 per line, 16 lines => 17*16 = 272;

uint32_t dispTimer2 = 0;
uint32_t conTimer = 0, ackTimer = 0;

uint8_t idCntBuffer[16][3] = // ID (0..0xFF),COUNT (0x01..0xFF),counter (init at 0, value is dynamic)
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
  pinMode(PIN_LED_TXL, OUTPUT);
  digitalWrite(PIN_LED_RXL, HIGH);
  digitalWrite(PIN_LED_TXL, HIGH);

  console.init();
  dma.init();

  initHexSprites();
  renderScreen(numbers, 272, RED);


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
            dataBuffer[k][i] = buf[i];

          }
          msg[11] = '#';

          // 2. send to bluetooth module if connection is OK
          if (connection)
          {
            Serial1.print(msg);
            txState = !txState;
            digitalWrite(PIN_LED_TXL, txState);
          }

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
        digitalWrite(13, LOW);

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
    renderScreen(numbers, 272, BLACK);


    
  }



  //update connection and get data from remote device, and send the data
  if (Serial1.available())
  {
    //if we receive data => connection is OK
    connection = true;
    rxState = !rxState;
    digitalWrite(PIN_LED_RXL, rxState);

    char ch = Serial1.read();
    if (ch == '!')
    {
      //if (debug)SerialUSB.print("Receive : ");
      for (int i = 0; i < 3; i++)
      {
        while (!Serial1.available());
        receiveBuffer[i] = Serial1.read();
      }
      while (!Serial1.available());
      ch = Serial1.read();
      if (ch == '#')
      {
        //message is valid, process data and send it on CAN bus
        if (receiveBuffer[0] & 0x04)
        {
          //is button A is pressed
          if (debug)SerialUSB.println("BUTTON A IS PRESSED ON REMOTE");
          if (debug)SerialUSB.println(receiveBuffer[0], BIN);
          
          // send data:  id = 0x00, standrad frame, data len = 8, stmp: data buf
          canSendBuffer[0]=0x02;
          canSendBuffer[1]=(receiveBuffer[0] & 0x02)?0xFF:0x00;
          canSendBuffer[2]=0x41;
          CAN.sendMsgBuf(0x07A4, 0, 3, canSendBuffer);



        }
        if (receiveBuffer[0] & 0x02)
        {
          //is button B is pressed
          if (debug)SerialUSB.println("BUTTON B IS PRESSED ON REMOTE");
        }
        if (receiveBuffer[0] & 0x01)
        {
          //is button C is pressed
          if (debug)SerialUSB.println("BUTTON C IS PRESSED ON REMOTE");
        }

        //receiveBuffer[1] = X joystick value, 128 is center, approx. 0 is left, 128 right
        if (receiveBuffer[1] > 128 + 0x0F)
        {
          if (debug)SerialUSB.println("JOYSTICK RIGHT : " + String(receiveBuffer[1]));
        }
        else if (receiveBuffer[1] < 128 - 0x0F)
        {
          if (debug)SerialUSB.println("JOYSTICK LEFT : " + String(receiveBuffer[1]));
        }

        //receiveBuffer[2] = Y joystick value, 128 is center, approx. 0 is up, 128 down
        if (receiveBuffer[2] > 128 + 0x0F)
        {
          if (debug)SerialUSB.println("JOYSTICK DOWN : " + String(receiveBuffer[2]));
        }
        else if (receiveBuffer[2] < 128 - 0x0F)
        {
          if (debug)SerialUSB.println("JOYSTICK UP : " +  String(receiveBuffer[2]));
        }
      }
    }

    //rearm bt connection timeout timer
    conTimer = millis();
  }

  //BT conncetion timeout management
  if (millis() - conTimer > 1500)
  {
    conTimer = millis();
    if (debug)SerialUSB.println("CONNECTION CLOSED");
    connection = false;
    digitalWrite(PIN_LED_RXL, HIGH);  //in case they stay ON, turn them OFF
    digitalWrite(PIN_LED_TXL, HIGH);
  }


  //update display
  if (millis() - ackTimer > 1000)
  {
    ackTimer = millis();
    Serial1.write('?'); //ask for connection status
    if (debug)SerialUSB.println('?');
  }


} // end loop



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
  {
    Serial1.read();
    if (debug)SerialUSB.print(".");
  }

  digitalWrite(A4, LOW);
  digitalWrite(12, LOW);
  delay(100);
  digitalWrite(12, HIGH);
  digitalWrite(13, LOW);
  if (debug)SerialUSB.print(".");
  if (debug)SerialUSB.println("OK");
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

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
