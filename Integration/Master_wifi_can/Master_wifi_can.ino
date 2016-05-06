#include <Oleduino.h>
#include <SPIdma.h>
#include "mcp_can.h"
#include "fip_sprite.h"
#include "fip_hexSprites.h"

#define WIFI_SERIAL Serial
#define MSG_SIZE 12

DMA dma;
MCP_CAN CAN(7);    // Set CS pin
Oleduino console;

String strBuffer[16];
uint8_t dataBuffer[16][8] = {0x00};
uint8_t canSendBuffer[8]; //to send data on the can bus
uint8_t receiveBuffer[3]; //to get data from Wifi

uint32_t dispTimer = 0;
bool connection = true;

bool debug = false; //if debug is true, you MUST keep serial port open for normal operation

bool rxState = 0, txState = 0;

SpriteInst numbers[272];  //all sprites' numbers : 2 for ID, 1 for ':', 14 for data = 17 per line, 16 lines => 17*16 = 272;

uint32_t dispTimer2 = 0;
uint32_t conTimer = 0, ackTimer = 0;

char inMsg[12] = {0};

//known robot's peripherals ID
uint8_t idCntBuffer[16][3] = // ID (0..0xFF),COUNT (0x01..0xFF),counter (init at 0, value is dynamic)
{
  {0x01, 6, 1},
  {0x12, 4, 1},
  {0x23, 3, 1},
  {0x34, 6, 1},
  {0x45, 5, 1},
  {0x56, 3, 1},
  {0x67, 2, 1},
  {0x78, 3, 1},
  {0x89, 7, 1},
  {0x9A, 5, 1},
  {0xAB, 10, 1},
  {0xBC, 4, 1},
  {0xCD, 5, 1},
  {0xDE, 1, 1},
  {0xEF, 1, 1},
  {0xF8, 1, 1}
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

  startWifiServer();

  while (CAN_OK != CAN.begin(CAN_250KBPS))              // init can bus : baudrate = 500k
  {
    if (debug)
    {
      SerialUSB.println("CAN BUS Shield init fail");
      SerialUSB.println(" Init CAN BUS Shield again");
    }
    delay(100);
  }
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

          // 2. send to wifi module if connection is OK
          if (connection)
          {
            serverSend(msg);
            idCntBuffer[k][2] = idCntBuffer[k][1]; //reset counter

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
            if (debug)
            {
              SerialUSB.print(" 0x");
              SerialUSB.print(msg[i], HEX);
            }
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

  //if (Serial.available())
  //  SerialUSB.write(char(Serial.read()));

  //update connection and get data from remote device, and send the data
  if (WIFI_SERIAL.available())
  {
    //if we receive data => connection is OK
    char ch = WIFI_SERIAL.read();
    if (ch == '+')
    {
      while (!WIFI_SERIAL.available());
      ch = WIFI_SERIAL.read();
      if (ch == 'I')
      {
        while (!WIFI_SERIAL.available());
        ch = WIFI_SERIAL.read();
        if (ch == 'P')
        {
          while (!WIFI_SERIAL.available());
          ch = WIFI_SERIAL.read();
          if (ch == 'D')
          {
            while (!WIFI_SERIAL.available());
            ch = WIFI_SERIAL.read();
            if (ch == ',')
            {
              while (!WIFI_SERIAL.available());
              ch = WIFI_SERIAL.read();
              if (ch == '0')
              {
                while (!WIFI_SERIAL.available());
                ch = WIFI_SERIAL.read();
                if (ch == ',')
                {
                  int ms = WIFI_SERIAL.parseInt();

                  while (!WIFI_SERIAL.available());
                  ch = WIFI_SERIAL.read();
                  if (ch == ':')
                  {
                    while (!WIFI_SERIAL.available());
                    ch = WIFI_SERIAL.read();
                    if (ch == '!')
                    {
                      //here we can assume that we receive pseudo valid data from slave : the connection is alive
                      connection = true;
                      conTimer = millis();

                      for (int i = 0; i < 3; i++)
                      {
                        while (!WIFI_SERIAL.available());
                        ch = WIFI_SERIAL.read();
                        receiveBuffer[i] = ch;
                        if (debug)SerialUSB.print(ch);
                      }
                      while (!WIFI_SERIAL.available());
                      ch = WIFI_SERIAL.read();
                      if (ch == '#') //valid data has been received
                      {
                        rxState = !rxState;
                        digitalWrite(PIN_LED_RXL, rxState);
                        if (debug)
                        {
                          SerialUSB.print("\n\nGOT VALID DATA FROM SLAVE : ");
                          SerialUSB.print(receiveBuffer[0], BIN);
                          SerialUSB.print(" ");
                          SerialUSB.print(receiveBuffer[1], DEC);
                          SerialUSB.print(" ");
                          SerialUSB.println(receiveBuffer[2], DEC);
                        }

                        //cmd processing :
                        if (receiveBuffer[0] & 0x04)
                        {
                          //is button A is pressed

                          if (debug)SerialUSB.println("BUTTON A IS PRESSED ON REMOTE");
                          // send data:  id = 0x00, standrad frame, data len = 8, stmp: data buf
                          canSendBuffer[0] = 0x01;
                          canSendBuffer[1] = 0x0A;
                          CAN.sendMsgBuf(0x07A4, 0, 2, canSendBuffer);
                        }
                        if (receiveBuffer[0] & 0x02)
                        {
                          //is button B is pressed
                          if (debug)SerialUSB.println("BUTTON B IS PRESSED ON REMOTE");
                          // send data:  id = 0x00, standrad frame, data len = 8, stmp: data buf
                          canSendBuffer[0] = 0x01;
                          canSendBuffer[1] = 0x0B;

                          CAN.sendMsgBuf(0x07A4, 0, 2, canSendBuffer);
                        }
                        if (receiveBuffer[0] & 0x01)
                        {
                          //is button C is pressed
                          if (debug)SerialUSB.println("BUTTON C IS PRESSED ON REMOTE");
                          canSendBuffer[0] = 0x01;
                          canSendBuffer[1] = 0x0C;

                          CAN.sendMsgBuf(0x07A4, 0, 2, canSendBuffer);
                        }

                        //receiveBuffer[1] = X joystick value, 128 is center, approx. 0 is left, 128 right
                        if (receiveBuffer[1] > 128 + 0x0F)
                        {
                          if (debug)SerialUSB.println("JOYSTICK RIGHT : " + String(receiveBuffer[1]));
                          canSendBuffer[0] = 0x01;
                          canSendBuffer[1] = 0xF0;
                          CAN.sendMsgBuf(0x07A4, 0, 2, canSendBuffer);
                        }
                        else if (receiveBuffer[1] < 128 - 0x0F)
                        {
                          if (debug)SerialUSB.println("JOYSTICK LEFT : " + String(receiveBuffer[1]));
                          canSendBuffer[0] = 0x01;
                          canSendBuffer[1] = 0xF1;
                          CAN.sendMsgBuf(0x07A4, 0, 2, canSendBuffer);
                        }

                        //receiveBuffer[2] = Y joystick value, 128 is center, approx. 0 is up, 128 down
                        if (receiveBuffer[2] > 128 + 0x0F)
                        {
                          if (debug)SerialUSB.println("JOYSTICK DOWN : " + String(receiveBuffer[2]));
                          canSendBuffer[0] = 0x01;
                          canSendBuffer[1] = 0xF2;
                          CAN.sendMsgBuf(0x07A4, 0, 2, canSendBuffer);
                        }
                        else if (receiveBuffer[2] < 128 - 0x0F)
                        {
                          if (debug)SerialUSB.println("JOYSTICK UP : " +  String(receiveBuffer[2]));
                          canSendBuffer[0] = 0x01;
                          canSendBuffer[1] = 0xF3;
                          CAN.sendMsgBuf(0x07A4, 0, 2, canSendBuffer);
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }




  //Wifi conncetion timeout management
  if (millis() - conTimer > 3000)
  {
    conTimer = millis();
    if (debug)SerialUSB.println("CONNECTION CLOSED");
    connection = false;
    digitalWrite(PIN_LED_RXL, HIGH);  //in case they stay ON, turn them OFF
    digitalWrite(PIN_LED_TXL, HIGH);
  }
} // end loop


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


void startWifiServer()
{
  bool succ = false;
  String successTrack = "";
  String command;

  pinMode(WIFI_ENABLE, OUTPUT);
  digitalWrite(WIFI_ENABLE, HIGH);
  digitalWrite(WIFI_ENABLE, LOW);
  delay(50);
  digitalWrite(WIFI_ENABLE, HIGH);
  WIFI_SERIAL.begin(115200);
  WIFI_SERIAL.setTimeout(300);
  if (debug)SerialUSB.println("***********************************");
  if (debug)SerialUSB.println("WIFI >>> CONFIGURATION : START");

  wifiFlush(1000);

  if (wifiSendWaitAck("AT", "OK", 1000, 5))
  {
    digitalWrite(13, HIGH);
    if (debug)SerialUSB.println("WIFI >>> module OK");
  }
  else
  {
    digitalWrite(13, LOW);
    if (debug)SerialUSB.println("WIFI >>> ERROR : module FAIL");
  }

  wifiFlush(2000); //discard the message


  /* SETUP THE ACCESS POINT
     SSID : WIFI_MASTER
     PASSWD : wifiserver
  */
  succ = wifiSendWaitAck("AT+CWMODE=2", "OK", 5000);
  successTrack = succ ? "OK" : "FAIL";
  if (debug)SerialUSB.println("WIFI >>> AP mode " + successTrack);

  succ |= wifiSendWaitAck("AT+CWSAP=\"WIFI_MASTER\",\"wifiserver\",5,3", "OK", 2000, 5);
  succ |= wifiSendWaitAck("AT+CIPAP=\"192.168.111.1\"", "OK", 100);
  succ |= wifiSendWaitAck("AT+CIPMUX=1", "OK", 1000);
  succ |= wifiSendWaitAck("AT+CIPSERVER=1,234", "OK", 1000);
  succ |= wifiSendWaitAck("AT+CIPSTO=7200", "OK", 1000);

  successTrack = succ ? "OK" : "FAIL";

  if (debug)SerialUSB.println("WIFI >>> Server config " + successTrack);
  if (succ)
  {
    if (debug)SerialUSB.print("WIFI >>> Server IP : ");
    if (debug)SerialUSB.println(wifiGetLocalIP());
    wifiFlush(1000);
  }
  //fail = wifiSendWaitAck("AT+CIPSTATUS", "OK", 1000);
  //successTrack = fail ? "OK" : "OK";
  //SerialUSB.println("WIFI >>> Server config " + successTrack);

  if (succ)
  {
    if (debug)SerialUSB.println("WIFI >>> CONFIGURATION SERVER: SUCCESS");
  }
  else
  {
    if (debug)SerialUSB.println("WIFI >>> CONFIGURATION SERVER: FAIL");
    if (debug)SerialUSB.println("***********************************");
  }

}

bool wifiSendWaitAck(char * cmd, char * validation, int timeout)
{
  wifiFlush(100);
  long timer = millis();
  WIFI_SERIAL.println(cmd);
  while (millis() - timer < timeout)
  {
    if (WIFI_SERIAL.find(validation))
      return true;
  }
  return false;
}

bool wifiSendWaitAck(char * cmd, char * validation, int timeout, int retry)
{
  for (int i = 0; i < retry; i++)
  {
    wifiFlush(100);
    long timer = millis();
    WIFI_SERIAL.println(cmd);
    while (millis() - timer < timeout)
    {
      if (WIFI_SERIAL.find(validation))
        return true;
    }
  }
  return false;
}

void serverSend(String msg)
{
  WIFI_SERIAL.print("AT+CIPSEND=0,"); //assume connection ID is 0
  WIFI_SERIAL.println(msg.length());
  delay(50);
  WIFI_SERIAL.print(msg);
  //delay(5);
}

void wifiFlush(int d)
{
  delay(d);
  while (WIFI_SERIAL.available())
    WIFI_SERIAL.read();
}

String wifiGetLocalIP()
{
  WIFI_SERIAL.println("AT+CIFSR");
  delay(10);
  String s = WIFI_SERIAL.readStringUntil('"');
  s = WIFI_SERIAL.readStringUntil('"');
  return s;
}

const char ipd[5] = {'+', 'I', 'P', 'D', ','};

bool processIncomingServerData()
{
  char c;
  if (WIFI_SERIAL.available())
  {
    c = WIFI_SERIAL.read();
    for (int i = 0; i < 5; i++)
    {
      if (c != ipd[i])
      {
        return false; //invalid data from server, give up
      }
    }
  }
  else
    return false; //no data available

  //if we go up here, it means we got a message from server
  int msgSize = WIFI_SERIAL.parseInt();

  if (msgSize != MSG_SIZE)
    return false;

  for (int i = 0; i < MSG_SIZE; i++)
  {
    while (!WIFI_SERIAL.available());
    inMsg[i] = WIFI_SERIAL.read();
  }
  if (inMsg[0] != '!' || inMsg[11] != '#')
    return false;
  else
    return true;
}
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
