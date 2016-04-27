#include <SPI.h>
#include <Wire.h>
#include <Oleduino.h>

//not used
String MASTER_SSID =     "WIFI_MASTER";
String MASTER_PASSWD =   "wifiserver";
String MASTER_IP =       "192.168.111.1";
String SLAVE_IP =        "192.168.111.2";
String SERVER_PORT =     "234";

#define WIFI_ROLE_SLAVE  0
#define WIFI_ROLE_MASTER 1
#define WIFI_ROLE WIFI_ROLE_MASTER

#define WIFI_SERIAL Serial

Oleduino console;

boolean toggleRX = true, toggleTX = true;

void setup() {
  // put your setup code here, to run once:
  SerialUSB.begin(115200);
  console.init();
  //while(!SerialUSB);
  console.display.println("press A to start");

  while (!console.A.isPressed());
  //while (!SerialUSB);
  console.display.println("Start config");

  SerialUSB.println("Start program\n");

#if WIFI_ROLE
  startWifiServer();
#else
  startWifiClient();
#endif



}

void loop() {
  // put your main code here, to run repeatedly:

}



/*WIFI SERVER
   - the master is the AP : SSID = "WIFI_MASTER" PASSWD="wifiserver"
   - the master set up a webserver on port 234
*/
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
  SerialUSB.println("***********************************");
  SerialUSB.println("WIFI >>> CONFIGURATION : START");

  wifiFlush(1000);

  if (wifiSendWaitAck("AT", "OK", 1000, 5))
  {
    digitalWrite(13, HIGH);
    SerialUSB.println("WIFI >>> module OK");
  }
  else
  {
    digitalWrite(13, LOW);
    SerialUSB.println("WIFI >>> ERROR : module FAIL");
  }

  wifiFlush(2000); //discard the message


  /* SETUP THE ACCESS POINT
     SSID : WIFI_MASTER
     PASSWD : wifiserver
  */
  succ = wifiSendWaitAck("AT+CWMODE=2", "OK", 5000);
  successTrack = succ ? "OK" : "FAIL";
  SerialUSB.println("WIFI >>> AP mode " + successTrack);

  succ |= wifiSendWaitAck("AT+CWSAP=\"WIFI_MASTER\",\"wifiserver\",5,3", "OK", 2000, 5);
  succ |= wifiSendWaitAck("AT+CIPAP=\"192.168.111.1\"", "OK", 100);
  succ |= wifiSendWaitAck("AT+CIPMUX=1", "OK", 1000);
  succ |= wifiSendWaitAck("AT+CIPSERVER=1,234", "OK", 1000);
  succ |= wifiSendWaitAck("AT+CIPSTO=7200", "OK", 1000);

  successTrack = succ ? "OK" : "FAIL";
  SerialUSB.println("WIFI >>> Server config " + successTrack);
  if (succ)
  {
    SerialUSB.print("WIFI >>> Server IP : ");
    SerialUSB.println(wifiGetLocalIP());
    wifiFlush(1000);
  }
  //fail = wifiSendWaitAck("AT+CIPSTATUS", "OK", 1000);
  //successTrack = fail ? "OK" : "OK";
  //SerialUSB.println("WIFI >>> Server config " + successTrack);

  if (succ)
    SerialUSB.println("WIFI >>> CONFIGURATION SERVER: SUCCESS");
  else
    SerialUSB.println("WIFI >>> CONFIGURATION SERVER: FAIL");
  SerialUSB.println("***********************************");

  console.display.println("Server config ok");


  uint32_t blinktimer = 0;
  bool b = 0;

  while (1)
  {
    wifiDirectSerialMode();
    if (console.C.isPressed() && console.C.justPressed())
    {
      WIFI_SERIAL.println("AT+PING=\"192.168.111.2\"");
      console.display.fillScreen(0);
      console.display.setCursor(0, 0);
      console.display.print("Ping : ");
      while (!WIFI_SERIAL.available());
      uint32_t timer = millis();
      while (WIFI_SERIAL.read() != '+' && millis() - timer < 500);
      timer = millis();
      while (WIFI_SERIAL.read() != '+' && millis() - timer < 500);

      int integer = WIFI_SERIAL.parseInt();
      if (integer > 0)
        console.display.println(integer);
      else
        console.display.println("Timeout");


    }

    //send "hello world" to other device if button B is pressed.
    if (console.B.isPressed() && console.B.justPressed())
      serverSend("hello world");


    //proof that the code is running
    if (millis() - blinktimer > 500)
    {
      blinktimer = millis();
      b = !b;
      digitalWrite(13, b);
    }


  }
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
  SerialUSB.println("***********************************");
  SerialUSB.println("WIFI >>> CONFIGURATION CLIENT: START");

  wifiFlush(1000);

  if (wifiSendWaitAck("AT", "OK", 1000, 5))
  {
    digitalWrite(13, HIGH);
    SerialUSB.println("WIFI >>> module OK");
  }
  else
  {
    digitalWrite(13, LOW);
    SerialUSB.println("WIFI >>> ERROR : module FAIL");
  }

  wifiFlush(2000); //discard the message


  /* SETUP THE ACCESS POINT
     SSID : WIFI_MASTER
     PASSWD : wifiserver
  */
  succ = wifiSendWaitAck("AT+CWMODE=1", "OK", 5000);
  successTrack = succ ? "OK" : "FAIL";
  SerialUSB.println("WIFI >>> Station mode " + successTrack);

  succ = wifiSendWaitAck("AT+CWJAP=\"WIFI_MASTER\",\"wifiserver\"", "OK", 30000);  //some ESP send "got IP" after a few second
  successTrack = succ ? "OK" : "FAIL";
  SerialUSB.println("WIFI >>> Join AP " + successTrack);
  if (succ)
  {
    SerialUSB.print("WIFI >>> Client IP : ");
    SerialUSB.println(wifiGetLocalIP());
    wifiFlush(1000);
  }


  succ = wifiSendWaitAck("AT+CIPSTART=\"TCP\",\"192.168.111.1\",234", "OK", 5000); //some ESP send back "OK", others send back "Linked"
  successTrack = succ ? "OK" : "FAIL";
  SerialUSB.println("WIFI >>> Client linked to server : " + successTrack);


  /*
    succ |= wifiSendWaitAck("AT+CWSAP=\"WIFI_MASTER\",\"wifiserver\",5,3", "OK", 2000, 5);
    succ |= wifiSendWaitAck("AT+CIPAP=\"192.168.111.1\"", "OK", 300, 5);
    succ |= wifiSendWaitAck("AT+CIPMUX=1", "OK", 1000);
    succ |= wifiSendWaitAck("AT+CIPSERVER=1,234", "OK", 1000);
    succ |= wifiSendWaitAck("AT+CIPSTO=7200", "OK", 1000);
  */

  if (succ)
    SerialUSB.println("WIFI >>> CONFIGURATION : SUCCESS");
  else
    SerialUSB.println("WIFI >>> CONFIGURATION : FAIL");
  SerialUSB.println("***********************************");

  console.display.println("Client config ok");

  uint32_t blinktimer = 0;
  bool b = 0;

  while (1)
  {
    wifiDirectSerialMode();

    //proof that the code is running
    if (millis() - blinktimer > 500)
    {
      blinktimer = millis();
      b = !b;
      digitalWrite(13, b);
    }
  }
}



void wifiFlush(int d)
{
  delay(d);
  while (WIFI_SERIAL.available())
    WIFI_SERIAL.read();
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
  SerialUSB.println("Sending : " + msg);
  SerialUSB.print("String length : ");
  SerialUSB.println(msg.length());

  WIFI_SERIAL.print("AT+CIPSEND=0,"); //assume connection ID is 0
  WIFI_SERIAL.println(msg.length());
  while (WIFI_SERIAL.read() != '>'); //wait for prompt
  WIFI_SERIAL.println(msg);
}

void wifiDirectSerialMode()
{
  //get data from wifi module, send to computer
  //processIncomingData();
  if (WIFI_SERIAL.available())
    SerialUSB.write(WIFI_SERIAL.read());
  //send data from computer to wifi module
  if (SerialUSB.available())
    WIFI_SERIAL.write(SerialUSB.read());
}

void processIncomingData()
{
  char c;
  if (WIFI_SERIAL.available()>3)
  {
    c = WIFI_SERIAL.read();
    if (c == '+')
    {
      if (WIFI_SERIAL.available())
      {
        c = WIFI_SERIAL.read();
        if (c == 'I')
        {
          if (WIFI_SERIAL.available())
          {
            c = WIFI_SERIAL.read();
            if (c == 'P')
            {
              if (WIFI_SERIAL.available())
              {
                c = WIFI_SERIAL.read();
                if (c == 'D')
                {
                  if (WIFI_SERIAL.available())
                  {
                    c = WIFI_SERIAL.read();
                    if (c == ',')
                    {
                      int msgSize = WIFI_SERIAL.parseInt();
                      WIFI_SERIAL.read(); //discard ':'
                      SerialUSB.println("Received a message of " + String(msgSize) + " bytes");
                      String s = WIFI_SERIAL.readString();
                      SerialUSB.println(s);
                      console.display.println(s);
                    }
                    else
                      SerialUSB.print(c);
                  }
                }
                else
                  SerialUSB.print(c);
              }
            }
            else
              SerialUSB.print(c);
          }
        }
        else
          SerialUSB.print(c);
      }
    }
    else
      SerialUSB.print(c);
  }
}

String wifiGetLocalIP()
{
  WIFI_SERIAL.println("AT+CIFSR");
  delay(10);
  String s = WIFI_SERIAL.readStringUntil('"');
  s = WIFI_SERIAL.readStringUntil('"');
  return s;
}


