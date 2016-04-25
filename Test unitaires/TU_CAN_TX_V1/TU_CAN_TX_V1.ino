// demo: CAN-BUS Shield, send data
#include <mcp_can.h>
#include <SPI.h>

// CS pin for CAN is digital 7 on the console
const int SPI_CS_PIN = 7;

MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin

void setup()
{
  while (!SerialUSB);

  while (CAN_OK != CAN.begin(CAN_500KBPS))              // init can bus : baudrate = 500k
  {
    SerialUSB.println("CAN BUS Shield init fail");
    SerialUSB.println(" Init CAN BUS Shield again");
    delay(100);
  }
  SerialUSB.println("CAN BUS Shield init ok!");
}

unsigned char stmp[8] = {0, 1, 2, 3, 4, 5, 6, 7};
void loop()
{
  // send data:  id = 0x00, standrad frame, data len = 8, stmp: data buf
  if (SerialUSB.available())
  {
    while (SerialUSB.available())
      SerialUSB.read();
    SerialUSB.println("send");
    CAN.sendMsgBuf(0x69, 0, 8, stmp);
    delay(1000);
  }// send data per 100ms
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
