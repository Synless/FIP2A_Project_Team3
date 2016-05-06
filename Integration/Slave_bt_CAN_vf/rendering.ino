#define BUF_LENGTH 128
uint8_t buffer[2 * BUF_LENGTH];
void renderScreen(SpriteInst* dataSprite, int nbSprites, uint16_t bgCol)
{
  uint16_t ALPHA = 0xF81F;
  uint16_t lineBuffer[128];
  initFullScreenTransfer();
  for (int lines = 0; lines < 128; ++lines)
  {
    for (int i = 0; i < 128; i++)
    {
      lineBuffer[i] = bgCol;
    }
    //show score
    for (int index = 0; index < nbSprites; ++index)
    {
      if (dataSprite[index].enabled)
      {
        const Sprite* spriteType = dataSprite[index].sprite;
        int curLine = lines - dataSprite[index].y;
        if ( curLine >= 0 && curLine < spriteType->height )
        {
          for (int x = 0; x < spriteType->width; ++x)
          {
            int tx = x + dataSprite[index].x;
            if ( tx >= 0 && tx < 128 )
            {
              uint16_t col = dataSprite[index].flip ? spriteType->data[(spriteType->width - (x + 1)) + (curLine * spriteType->width)] : spriteType->data[x + (curLine * spriteType->width)];
              //int offset = dataSprite[index].flip ? (spriteType->width - (x + 1)) + (curLine * spriteType->width) : x + (curLine * spriteType->width);
              //uint16_t col = pgm_read_word_near(spriteType->data + offset);
              if ( col != ALPHA )
              {
                lineBuffer[tx] = col;
              }
            }
          }
        }
      }
    }
    //fastHBuffer(lines, lineBuffer);
    transferLineOfScreen(lineBuffer);
  }
  endFullScreenTransfer();
}
void initFullScreenTransfer() {
  // set location
  console.display.writeCommand(SSD1351_CMD_SETCOLUMN);
  console.display.writeData(0);
  console.display.writeData(127);
  console.display.writeCommand(SSD1351_CMD_SETROW);
  console.display.writeData(0);
  console.display.writeData(127);
  // fill!
  console.display.writeCommand(SSD1351_CMD_WRITERAM);
  //digitalWrite(DISPLAY_DC_PIN, HIGH); // Set DATA/COMMAND pin to DATA.
  PORT->Group[g_APinDescription[DISPLAY_DC_PIN].ulPort].OUTSET.reg = (1ul << g_APinDescription[DISPLAY_DC_PIN].ulPin) ;
  //digitalWrite(DISPLAY_CS_PIN, LOW); // Tell display to pay attention to the incoming data.
  PORT->Group[g_APinDescription[DISPLAY_CS_PIN].ulPort].OUTCLR.reg = (1ul << g_APinDescription[DISPLAY_CS_PIN].ulPin) ;
  SPI.beginTransaction(SPISettings(12000000, MSBFIRST, SPI_MODE0));
}
void transferLineOfScreen(uint16_t buf[BUF_LENGTH]) {
  for (int i = 0, k = 0; i < 2 * BUF_LENGTH; i += 2, k++) {
    buffer[i] = buf[k] >> 8;
    buffer[i + 1] = buf[k];
  }
  dma.spi_write(buffer, 2 * BUF_LENGTH);
}
void endFullScreenTransfer() {
  SPI.endTransaction();
  //digitalWrite(DISPLAY_CS_PIN, HIGH);
  PORT->Group[g_APinDescription[DISPLAY_CS_PIN].ulPort].OUTSET.reg = (1ul << g_APinDescription[DISPLAY_CS_PIN].ulPin) ;
}
