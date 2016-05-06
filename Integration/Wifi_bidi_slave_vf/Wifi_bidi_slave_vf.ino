#include <SPI.h>
#include <Wire.h>
#include <Oleduino.h>
#include <SPIdma.h>
#include "fip_sprite.h"
#include "fip_hexSprites.h"

//not used
String MASTER_SSID =     "WIFI_MASTER";
String MASTER_PASSWD =   "wifiserver";
String MASTER_IP =       "192.168.111.1";
String SLAVE_IP =        "192.168.111.2";
String SERVER_PORT =     "234";

#define WIFI_ROLE_SLAVE  0
#define WIFI_ROLE_MASTER 1
#define WIFI_ROLE WIFI_ROLE_SLAVE
#define DEBUG 0
#define fps 2
#define WIFI_SERIAL Serial

Oleduino console;
DMA dma;

boolean toggleRX = true, toggleTX = true;
uint32_t Timer1 = 0;
bool b = 0;
uint8_t dataBuffer[12];
uint8_t dataBuffer_dma[16][8] = {0};
SpriteInst numbers[272];

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
  // put your setup code here, to run once:
  SerialUSB.begin(115200);

  // init console and DMA functions
  console.init();
  dma.init();
  initHexSprites();

  // update screen  and fill with 'FF'
  renderScreen(numbers, 272, RED);

  // wait for button A
  //while (!console.A.isPressed());


  if (DEBUG)
  {
    SerialUSB.println("Start program\n");
  }

#if WIFI_ROLE
  startWifiServer();
#else
  startWifiClient();
#endif

}

//main loop function, not used in this application
void loop()
{
  
}


/*WIFI CLIENT
   - the client is the Station, must connect to server : SSID = "WIFI_MASTER" PASSWD="wifiserver"
   - the client connects on webserver at 192.168.111.1 on port 234
*/

void startWifiClient()
{
  bool succ = false;
  String successTrack = "";

  pinMode(WIFI_ENABLE, OUTPUT);
  digitalWrite(WIFI_ENABLE, HIGH);
  digitalWrite(WIFI_ENABLE, LOW);
  delay(50);
  digitalWrite(WIFI_ENABLE, HIGH);
  WIFI_SERIAL.begin(115200);
  WIFI_SERIAL.setTimeout(300);

  if (DEBUG)
  {
    SerialUSB.println("***********************************");
    SerialUSB.println("WIFI >>> CONFIGURATION CLIENT: START");
  }

  wifiFlush(1000);

  if (wifiSendWaitAck("AT", "OK", 1000, 5))
  {
    digitalWrite(13, HIGH);
    if (DEBUG) {
      SerialUSB.println("WIFI >>> module OK");
    }
  }
  else
  {
    digitalWrite(13, LOW);
    if (DEBUG) {
      SerialUSB.println("WIFI >>> ERROR : module FAIL");
    }
  }

  wifiFlush(2000); //discard the message


  /* SETUP THE ACCESS POINT
     SSID : WIFI_MASTER
     PASSWD : wifiserver
  */
  succ = wifiSendWaitAck("AT+CWMODE=1", "OK", 5000);

  successTrack = succ ? "OK" : "FAIL";
  if (DEBUG) {
    SerialUSB.println("WIFI >>> Station mode " + successTrack);
  }

  succ = wifiSendWaitAck("AT+CWJAP=\"WIFI_MASTER\",\"wifiserver\"", "OK", 30000);  //some ESP send "got IP" after a few second

  successTrack = succ ? "OK" : "FAIL";
  if (DEBUG) {
    SerialUSB.println("WIFI >>> Join AP " + successTrack);
  }

  if (succ)
  {
    if (DEBUG)
    {
      SerialUSB.print("WIFI >>> Client IP : ");
      SerialUSB.println(wifiGetLocalIP());
    }
    wifiFlush(1000);
  }


  succ = wifiSendWaitAck("AT+CIPSTART=\"TCP\",\"192.168.111.1\",234", "OK", 5000); //some ESP send back "OK", others send back "Linked"
  successTrack = succ ? "OK" : "FAIL";
  if (DEBUG) {
    SerialUSB.println("WIFI >>> Client linked to server : " + successTrack);
  }

  /*
    succ |= wifiSendWaitAck("AT+CWSAP=\"WIFI_MASTER\",\"wifiserver\",5,3", "OK", 2000, 5);
    succ |= wifiSendWaitAck("AT+CIPAP=\"192.168.111.1\"", "OK", 300, 5);
    succ |= wifiSendWaitAck("AT+CIPMUX=1", "OK", 1000);
    succ |= wifiSendWaitAck("AT+CIPSERVER=1,234", "OK", 1000);
    succ |= wifiSendWaitAck("AT+CIPSTO=7200", "OK", 1000);
  */

  if (succ)
  {
    if (DEBUG)
    {
      SerialUSB.println("WIFI >>> CONFIGURATION : SUCCESS");
    }
  }
  else
  {
    if (DEBUG)
    {
      SerialUSB.println("WIFI >>> CONFIGURATION : FAIL");
      SerialUSB.println("***********************************");
    }
  }

  //console.display.println("Client config ok");
  uint32_t blinktimer = 0;

  setNumbers();
  renderScreen(numbers, 272, BLACK);

  while (1)
  {
    wifiDirectSerialMode();

    //proof that the code is running
    if (millis() - blinktimer > (int)(1000 / fps))
    {
      blinktimer = millis();
      setNumbers();
      renderScreen(numbers, 272, BLACK);
    }

    if (millis() - Timer1 > 100)
    {
      Timer1 = millis();
      read_button();
    }
  }
}



void wifiFlush(int d)
{
  delay(d);
  while (WIFI_SERIAL.available())
  {
    WIFI_SERIAL.read();
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
  if (DEBUG)
  {
    SerialUSB.println("Sending : " + msg);
    SerialUSB.print("String length : ");
    SerialUSB.println(msg.length());
  }
  WIFI_SERIAL.print("AT+CIPSEND=0,"); //assume connection ID is 0
  WIFI_SERIAL.println(msg.length());
  while (WIFI_SERIAL.read() != '>'); //wait for prompt
  WIFI_SERIAL.println(msg);
}


void wifiDirectSerialMode()
{
  //get data from wifi module, send to computer
  processIncomingData();
  //if (WIFI_SERIAL.available())
  //  SerialUSB.write(WIFI_SERIAL.read());
  //send data from computer to wifi module
  if (SerialUSB.available())
    WIFI_SERIAL.write(SerialUSB.read());
}

// read A, B, C button status and joystick values
// send data tot the server over WIFI_SERIAL
void read_button()
{
  uint8_t data[5] = {0}, buttonValue = 0b00001000 , Xjoy = 0, Yjoy = 0;
  bool buttonA = false, buttonB = false, buttonC = false;

  buttonA = console.A.isPressed();    // lecture de l'etat des bouttons
  buttonB = console.B.isPressed();
  buttonC = console.C.isPressed();

  console.joystick.read();            // lecture de la valeur des joysticks
  Xjoy = map(console.joystick.getRawX(), 0, 4095, 0, 255);
  Yjoy = map(console.joystick.getRawY(), 0, 4095, 0, 255);;

  if (buttonA)
  {
    buttonValue = buttonValue | 0b00000100;
  }
  if (buttonB)
    buttonValue = buttonValue | 0b00000010;
  if (buttonC)
    buttonValue = buttonValue | 0b00000001;

  data[0] = '!';          //transmission begin character
  data[1] = buttonValue;
  data[2] = Xjoy;
  data[3] = Yjoy;
  data[4] = '#';          //transmission end character

  if (DEBUG)
  {
    SerialUSB.print("Read button: ");
    SerialUSB.print(data[1], DEC);
    SerialUSB.print(" ");
    SerialUSB.print(data[2], DEC);
    SerialUSB.print(" ");
    SerialUSB.print(data[3], DEC);
    SerialUSB.print("\n");
  }

  WIFI_SERIAL.println("AT+CIPSEND=5");
  delay(10);
  WIFI_SERIAL.write(data[0]);
  WIFI_SERIAL.write(data[1]);
  WIFI_SERIAL.write(data[2]);
  WIFI_SERIAL.write(data[3]);
  WIFI_SERIAL.write(data[4]);

}

// incomming data parsing function
void processIncomingData()
{
  char c;
  if (WIFI_SERIAL.available() > 3)
  {
    c = WIFI_SERIAL.read();
    //FINDING IF THE INCOMING SERIAL MESSAGE IS RIGHT
    if (c == '+')
    { if (WIFI_SERIAL.available())
      { c = WIFI_SERIAL.read();
        if (c == 'I')
        { if (WIFI_SERIAL.available())
          { c = WIFI_SERIAL.read();
            if (c == 'P')
            { if (WIFI_SERIAL.available())
              { c = WIFI_SERIAL.read();
                if (c == 'D')
                { if (WIFI_SERIAL.available())
                  { c = WIFI_SERIAL.read();
                    if (c == ',')
                    {
                      int msgSize = WIFI_SERIAL.parseInt();
                      WIFI_SERIAL.read(); //discard ':'
                      if (DEBUG) {
                        SerialUSB.println("Received a message of " + String(msgSize) + " bytes");
                      }

                      for (int i = 0; i < 12; i++)
                      {
                        while (!WIFI_SERIAL.available());
                        dataBuffer[i] = WIFI_SERIAL.read();
                        if (DEBUG)
                        {
                          SerialUSB.print(dataBuffer[i]);
                          SerialUSB.print("\t");
                        }
                        //console.display.println(dataBuffer[i]);
                      }

                      if (dataBuffer[0] == '!' & dataBuffer[11] == '#') //if data 1 ='!' and data 11 ='#'
                      {
                        if (DEBUG) {
                          SerialUSB.println("DATA VALID");
                        }
                        for (int k = 0; k < 16; k++)
                        { // update data for rendering
                          //CHECK THE THIRD BYTE OF THE RECEIVED DATA -> CAN ID
                          if (idDisplayBuffer[k][0] == dataBuffer[3])
                          {
                            if (DEBUG) {
                              SerialUSB.println(" ID FOUND!");
                            }
                            for (int n = 0; n < 8; n++)
                            {
                              dataBuffer_dma[k][n] =  dataBuffer[n + 3];      // save incomming data 
                            }
                            /*
                              dataBuffer_dma[k][0] = dataBuffer[3];
                              dataBuffer_dma[k][1] = dataBuffer[4];
                              dataBuffer_dma[k][2] = dataBuffer[5];
                              dataBuffer_dma[k][3] = dataBuffer[6];
                              dataBuffer_dma[k][4] = dataBuffer[7];
                              dataBuffer_dma[k][5] = dataBuffer[8];
                              dataBuffer_dma[k][6] = dataBuffer[9];
                              dataBuffer_dma[k][7] = dataBuffer[10];
                            */
                          }
                        }
                      }
                      else
                      {
                        if (DEBUG) {
                          SerialUSB.println("ERROR");
                        }
                        b = !b;
                        digitalWrite(13, b);
                        //error
                      }
                    }
                    else
                    {
                      if (DEBUG)
                      {
                        SerialUSB.print(c);
                      }
                    }
                  }
                }
                else
                {
                  if (DEBUG)
                  {
                    SerialUSB.print(c);
                  }
                }
              }
            }
            else
            {
              if (DEBUG)
              {
                SerialUSB.print(c);
              }
            }
          }
        }
        else
        {
          if (DEBUG)
          {
            SerialUSB.print(c);
          }
        }
      }
    }
    else if ( c == 'C' )
    {
      c = WIFI_SERIAL.read();

      if ( c == 'L' )
      {
        // RECEIVED "CLOSED", IM GESSING IT FROM THE FIRST TWO CHAR
        while (!wifiSendWaitAck("AT+CIPSTART=\"TCP\",\"192.168.111.1\",234", "OK", 5000))
        {
          //LOOP WHILE NOT RECONNECTED
          //5 TIMES PER SECOND
          delay(200);
        }
      }
    }
    else
    {
      if (DEBUG)
      {
        SerialUSB.print(c);
      }
    }
  }
}

// Srites dma initialisation function
void initHexSprites()
{
  for (int i = 0; i < 272; i++)
  {
    if (i % 17 == 2)
    {
      numbers[i].sprite = &s_Oxdot;
    }
    else
    {
      numbers[i].sprite = &s_OxF;
    }
    numbers[i].enabled = true;;
    numbers[i].x = (i % 17) * 6;
    numbers[i].y = i / 17 * 8;
  }
}

// update value in dataBuffer_dma to print data on the scree.
void setNumbers()
{
  for (int line = 0; line < 16; line++)
  {
    for (int column = 0; column < 8; column++)
    {
      uint8_t LB = dataBuffer_dma[line][column] & 0x0F;
      uint8_t HB = (dataBuffer_dma[line][column] >> 4) & 0x0F;
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

//get module IP address
String wifiGetLocalIP()
{
  WIFI_SERIAL.println("AT+CIFSR");
  delay(10);
  String s = WIFI_SERIAL.readStringUntil('"');
  s = WIFI_SERIAL.readStringUntil('"');
  return s;
}


