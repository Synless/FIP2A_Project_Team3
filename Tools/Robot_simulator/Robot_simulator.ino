#include <mcp_can.h>
#include <SPI.h>


/*Arduino board + SeedStudio Can Shield are necessary to simulate the robot behaviour*/

const int SPI_CS_PIN = 9;

MCP_CAN CAN(SPI_CS_PIN);       // Set CS pin

bool astate = 0, bstate = 0, cstate = 0;

void setup()
{
  Serial.begin(9600);

  while (CAN_OK != CAN.begin(CAN_250KBPS))              // init can bus : baudrate = 500k
  {
    Serial.println("CAN BUS Shield init fail");
    Serial.println(" Init CAN BUS Shield again");
    delay(100);
  }
  Serial.println("CAN BUS Shield init ok!");


  pinMode(4, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(7, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);

}


byte knownID[16] = {0x01, 0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78, 0x89, 0x9A, 0xAB, 0xBC, 0xCD, 0xDE, 0xEF, 0xF8};

unsigned char stmp[8] = {'h', 'e', 'l', 'l', 'o', 41, 42, 43};
uint8_t rxBuf[8];
byte i = 0, cnt = 0;
byte len;
void loop()
{
  i++;
  if (i == 0)
    i = 1;
  for (int a = 0; a < 7; a++)
    stmp[a] = i;

  stmp[0] = knownID[cnt++];
  
  switch (stmp[0])
  {
    case 0xF8:
      stmp[7] = constrain(analogRead(A15) >> 2, 1,255);
      break;
    case 0xEF:
      stmp[7] = digitalRead(4) ? 0xFF : 0x11;
      break;
    case 0xDE:
      stmp[7] = digitalRead(3) ? 0xFF : 0x11;
      break;
    default:
      stmp[7] = random(0xFF);
      break;
  }


  if (cnt >= 16)
    cnt = 0;
  // send data:  ID=i, standrad frame, data len = 8, stmp: data buf
  CAN.sendMsgBuf(i, 0, 8, stmp);

  if (CAN_MSGAVAIL == CAN.checkReceive())  // check if CAN data available
  {
    CAN.readMsgBuf(&len, rxBuf);    // read data,  len: data length, buf: data buf
    int dest = CAN.getCanId();
    Serial.print("CAN Receive ! destination is : 0x");
    Serial.println(dest, HEX);

    if (dest == 0x07A4 && rxBuf[0] == 0x01)
    {
      switch (rxBuf[1])
      {
        case 0x0A: //button A
          astate = !astate;
          digitalWrite(7, astate);
          break;
        case 0x0B: //button B
          bstate = !bstate;
          digitalWrite(5, bstate);
          break;
        case 0x0C: //button C
          cstate = !cstate;
          digitalWrite(6, cstate);
          break;
        case 0xF0: //right
          Serial.println("RIGHT");
          break;
        case 0xF1: //left
          Serial.println("LEFT");
          break;
        case 0xF2: //down
          Serial.println("DOWN");
          break;
        case 0xF3: //up
          Serial.println("UP");
          break;
      }
    }
  }
  delay(3);
}
