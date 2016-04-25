#include <SPI.h>
#include <Wire.h>
#include <Oleduino.h>

String MASTER_SSID =     "WIFI_MASTER";
String MASTER_PASSWD =   "wifiserver";
String MASTER_IP =       "192.168.111.1";
String SLAVE_IP =        "192.168.111.2";
String SERVER_PORT =     "234";

#define WIFI_SERIAL Serial

Oleduino c;

boolean toggleRX = true, toggleTX = true;

void setup() {
  // put your setup code here, to run once:
  SerialUSB.begin(115200);
  while (!SerialUSB);
  c.init();

  SerialUSB.println("Start program\n");

  startWifiServer();
  //startWifiClient();



}

void loop() {
  // put your main code here, to run repeatedly:

}


#define WIFI_ROLE_SLAVE  0
#define WIFI_ROLE_MASTER 1
#define WIFI_ROLE WIFI_ROLE_MASTER

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


  while (1)
  {
    wifiDirectSerialMode();
    if (c.C.isPressed() && c.C.justPressed())
    {
      WIFI_SERIAL.println("AT+PING=\"192.168.111.2\"");
      c.display.fillScreen(0);
      c.display.setCursor(0, 0);
      delay(100);
      while (WIFI_SERIAL.read() != '+');
      while (WIFI_SERIAL.read() != '+');

      int integer = WIFI_SERIAL.parseInt();
      if (integer != 0)
        c.display.print(integer);
      else
        c.display.print("Timeout");

      serverSend("hello world");
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

  while (1)
  {
    wifiDirectSerialMode();
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
  WIFI_SERIAL.print("AT+CIPSEND=0,"); //assume connection ID is 0
  WIFI_SERIAL.println(msg.length());
  while (WIFI_SERIAL.read() != '>'); //wait for prompt
  WIFI_SERIAL.println(msg);
}

void wifiDirectSerialMode()
{
  //get data from wifi module, send to computer
  if (WIFI_SERIAL.available())
    SerialUSB.write(WIFI_SERIAL.read());
  //send data from computer to wifi module
  if (SerialUSB.available())
    WIFI_SERIAL.write(SerialUSB.read());
}

String wifiGetLocalIP()
{
  WIFI_SERIAL.println("AT+CIFSR");
  delay(10);
  String s = WIFI_SERIAL.readStringUntil('"');
  s = WIFI_SERIAL.readStringUntil('"');
  return s;
}

