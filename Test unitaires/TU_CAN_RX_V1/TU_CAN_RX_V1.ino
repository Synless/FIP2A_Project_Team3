// demo: CAN-BUS Shield, receive data with check mode
// send data coming to fast, such as less than 10ms, you can use this way
// loovee, 2014-6-13


#include <SPI.h>
#include "mcp_can.h"


// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 7;

MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin

void setup()
{
    SerialUSB.begin(115200);
    while(!SerialUSB);
    
    while (CAN_OK != CAN.begin(CAN_200KBPS))              // init can bus : baudrate = 500k
    {
        SerialUSB.println("CAN BUS Shield init fail");
        SerialUSB.println(" Init CAN BUS Shield again");
        delay(100);
    }
    SerialUSB.println("CAN BUS Shield init ok!");
}


void loop()
{
    unsigned char len = 0;
    unsigned char buf[8];

    if(CAN_MSGAVAIL == CAN.checkReceive())            // check if data coming
    {
        CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf

        unsigned char canId = CAN.getCanId();
        
        SerialUSB.println("-----------------------------");
        SerialUSB.print("Get data from ID: ");
        SerialUSB.println(canId, HEX);

        for(int i = 0; i<len; i++)    // print the data
        {
            SerialUSB.print(buf[i], HEX);
            SerialUSB.print("\t");
        }
        SerialUSB.println();
    }
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
