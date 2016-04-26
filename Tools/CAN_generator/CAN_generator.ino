// demo: CAN-BUS Shield, send data
#include <mcp_can.h>
#include <SPI.h>

// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 9;

MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin

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
byte i = 0;

void loop()
{
  i++;
  if (i == 0)
    i = 1;
  for (int a = 0; a < 7; a++)
    stmp[a] = i;
  stmp[7] = random(0xFF);

  // send data:  id = 0x00, standrad frame, data len = 8, stmp: data buf
  CAN.sendMsgBuf(i, 0, 8, stmp);
  delay(3);                       // send data per 100ms
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
