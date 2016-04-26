void cls()
{
  fastfill(0);
}
void fastfill(uint16_t color) {

  uint8_t buffer[512]; // Buffer two full rows at a time - 512 bytes.  This is the same the size of an SD card block.

  for (int i = 0; i < 511; i += 2) {
    buffer[i] = color >> 8;
    buffer[i + 1] = color;
  }
  // Specify size of region to be drawn.

  console.display.writeCommand(SSD1351_CMD_SETCOLUMN);
  console.display.writeData(0);
  console.display.writeData(127);

  console.display.writeCommand(SSD1351_CMD_SETROW);
  console.display.writeData(0);
  console.display.writeData(127);

  console.display.writeCommand(SSD1351_CMD_WRITERAM); // Tell display we're going to send it image data in a moment. (Not sure if necessary.)
  digitalWrite(DISPLAY_DC_PIN, HIGH); // Set DATA/COMMAND pin to DATA.

  for (byte row = 0; row < 128; row += 2) { // 2.79FPS without SPI_FULL_SPEED in SPI.begin, 3.75FPS with it.
    digitalWrite(DISPLAY_CS_PIN, LOW); // Tell display to pay attention to the incoming data.
    //SPI.beginTransaction(SPISettings(12000000, MSBFIRST, SPI_MODE0)); // Adding this boosts speed to over 9FPS when not reading from SD card.  So reading from SD card is 2x as slow as this?

    dma.spi_write(buffer, 512);
    //SPI.transfer(buffer, 512);

    //SPI.endTransaction();
    digitalWrite(DISPLAY_CS_PIN, HIGH); // Tell display we're done talking to it for now, so the next SD read doesn't corrupt the screen.
  }
}

void clearBand(int y, int h, uint16_t color) {

  uint8_t buffer[256]; // Buffer two full rows at a time - 512 bytes.  This is the same the size of an SD card block.

  for (int i = 0; i < 255; i += 2) {
    buffer[i] = color >> 8;
    buffer[i + 1] = color;
  }
  // Specify size of region to be drawn.

  console.display.writeCommand(SSD1351_CMD_SETCOLUMN);
  console.display.writeData(0);
  console.display.writeData(127);

  console.display.writeCommand(SSD1351_CMD_SETROW);
  console.display.writeData(y);
  console.display.writeData(y+h-1);

  console.display.writeCommand(SSD1351_CMD_WRITERAM); // Tell display we're going to send it image data in a moment. (Not sure if necessary.)
  digitalWrite(DISPLAY_DC_PIN, HIGH); // Set DATA/COMMAND pin to DATA.

  for (byte row = 0; row < h; row ++) { // 2.79FPS without SPI_FULL_SPEED in SPI.begin, 3.75FPS with it.
    digitalWrite(DISPLAY_CS_PIN, LOW); // Tell display to pay attention to the incoming data.
    //SPI.beginTransaction(SPISettings(12000000, MSBFIRST, SPI_MODE0)); // Adding this boosts speed to over 9FPS when not reading from SD card.  So reading from SD card is 2x as slow as this?

    dma.spi_write(buffer, 256);

    //SPI.endTransaction();
    digitalWrite(DISPLAY_CS_PIN, HIGH); // Tell display we're done talking to it for now, so the next SD read doesn't corrupt the screen.
  }
}

