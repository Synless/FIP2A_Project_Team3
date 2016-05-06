#include <Oleduino.h>
Oleduino console;
#include "MAX17043.h"

MAX17043 batteryMonitor;

void setup() {

  console.init();

  batteryMonitor.reset();
  batteryMonitor.quickStart();
}

void loop() {
  float cellVoltage = batteryMonitor.getVCell();
  SerialUSB.print("Voltage:\t\t");
  SerialUSB.print(cellVoltage, 4);
  SerialUSB.println("V");

  float stateOfCharge = batteryMonitor.getSoC();
  SerialUSB.print("State of charge:\t");
  SerialUSB.print(stateOfCharge);
  SerialUSB.println("%");

  console.display.setCursor(0, 0);
  console.display.fillScreen(0);
  console.display.print("Bat. voltage : "); console.display.print(cellVoltage);
  console.display.print(" VBat. SoC : "); console.display.print(constrain(stateOfCharge,0,100));
  console.display.print(" %\nTemp. : "); console.display.print(console.bme.readTemperature());
  console.display.print(" `T\nHum. : "); console.display.print(console.bme.readHumidity());
  console.display.print(" %\nPres. : "); console.display.print(console.bme.readPressure()/100.0);
  console.display.print(" hPa");
  delay(1000);
}
