#include <mcp_can.h>
#include <SPI.h>


/*Arduino board + SeedStudio Can Shield are necessary to simulate the robot behaviour*/

const int SPI_CS_PIN = 9;

MCP_CAN CAN(SPI_CS_PIN);       // Set CS pin

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
}

unsigned char stmp[8] = {'h', 'e', 'l', 'l', 'o', 41, 42, 43};
uint8_t rxBuf[8];
byte i = 0;
byte len;
void loop()
{
  i++;
  if (i == 0)
    i = 1;
  for (int a = 0; a < 7; a++)
    stmp[a] = i;
  stmp[7] = random(0xFF);

  // send data:  ID=i, standrad frame, data len = 8, stmp: data buf
  CAN.sendMsgBuf(i, 0, 8, stmp);

  if (CAN_MSGAVAIL == CAN.checkReceive())  // check if CAN data available
  {
    CAN.readMsgBuf(&len, rxBuf);    // read data,  len: data length, buf: data buf
    int dest = CAN.getCanId();
    Serial.print("CAN Receive ! destination is : 0x");
    Serial.println(dest, HEX);

    if (dest == 0x07A4 && rxBuf[0] == 0x02) // this two value means that the brain (Robot ID 0) send 2 bytes (0x[ID][SIZE]) to the motor board (ID 0x7A4)
    {
      if (rxBuf[1] == 0xFF)
      {
        Serial.println("Button B is pressed on remote");
      }
    }
  }
  delay(3);
}
