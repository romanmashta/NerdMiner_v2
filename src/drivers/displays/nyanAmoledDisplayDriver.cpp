#include "../drivers.h"

#ifdef NYAN_AMOLED_DISPLAY

#include <rm67162.h>
#include <TFT_eSPI.h>
#include "media/images_536_240_nyan.h"
#include "media/myFonts.h"
#include "media/Free_Fonts.h"
#include "version.h"
#include "monitor.h"
#include "OpenFontRender.h"
#include "media/s_nyan.h"
#include <AnimatedGIF.h>

#define WIDTH 536
#define HEIGHT 240

#define PINK 0xF993

#define G_X 0
#define G_Y 0
#define G_WIDTH 295
#define G_HEIGHT 157

#define G_SIZE (G_WIDTH * G_HEIGHT)

#define GIF_IMAGE s_nyan_gif

#define BUFFER_SIZE G_WIDTH

uint16_t usTemp[G_WIDTH];
uint16_t gifBuf[G_SIZE];

OpenFontRender render;
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite background = TFT_eSprite(&tft);
TFT_eSprite gifSprite = TFT_eSprite(&tft);
AnimatedGIF gif;

void pushImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data){
  memcpy(&gifBuf[x + y * G_WIDTH], data, w * h * sizeof(uint16_t));
}

void GIFDraw(GIFDRAW *pDraw)
{
  uint8_t *s;
  uint16_t *d, *usPalette;
  int x, y, iWidth, iCount;

  // Display bounds check and cropping
  iWidth = pDraw->iWidth;
  if (iWidth + pDraw->iX > WIDTH)
    iWidth = WIDTH - pDraw->iX;
  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y; // current line
  if (y >= HEIGHT || pDraw->iX >= WIDTH || iWidth < 1)
    return;

  // Old image disposal
  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2) // restore to background color
  {
    for (x = 0; x < iWidth; x++)
    {
      if (s[x] == pDraw->ucTransparent)
        s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }

  // Apply the new pixels to the main image
  if (pDraw->ucHasTransparency) // if transparency used
  {
    uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
    pEnd = s + iWidth;
    x = 0;
    iCount = 0; // count non-transparent pixels
    while (x < iWidth)
    {
      c = ucTransparent - 1;
      d = &usTemp[0];
      while (c != ucTransparent && s < pEnd && iCount < BUFFER_SIZE )
      {
        c = *s++;
        if (c == ucTransparent) // done, stop
        {
          s--; // back up to treat it like transparent
        }
        else // opaque
        {
          *d++ = usPalette[c];
          iCount++;
        }
      } // while looking for opaque pixels
      if (iCount) // any opaque pixels?
      {
        // DMA would degrtade performance here due to short line segments
        //background.setAddrWindow(pDraw->iX + x, y, iCount, 1);
        //background.pushPixels(usTemp, iCount);
        //lcd_address_set(pDraw->iX + x, y, iCount, 1);
        //lcd_PushColors(usTemp[0], iCount);
        pushImage(pDraw->iX + x, y, iCount, 1, usTemp);
        x += iCount;
        iCount = 0;
      }
      // no, look for a run of transparent pixels
      c = ucTransparent;
      while (c == ucTransparent && s < pEnd)
      {
        c = *s++;
        if (c == ucTransparent)
          x++;
        else
          s--;
      }
    }
  }
  else
  {
    s = pDraw->pPixels;

    // Unroll the first pass to boost DMA performance
    // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
    if (iWidth <= BUFFER_SIZE)
      for (iCount = 0; iCount < iWidth; iCount++) usTemp[iCount] = usPalette[*s++];
    else
      for (iCount = 0; iCount < BUFFER_SIZE; iCount++) usTemp[iCount] = usPalette[*s++];

    //background.setAddrWindow(pDraw->iX, y, iWidth, 1);
    //background.pushPixels(&usTemp[0][0], iCount);
    //lcd_address_set(pDraw->iX, y, iWidth, 1);
    //lcd_PushColors(usTemp[0], iCount);
    pushImage(pDraw->iX, y, iWidth, 1, usTemp);

    iWidth -= iCount;
    // Loop if pixel buffer smaller than width
    while (iWidth > 0)
    {
      // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
      if (iWidth <= BUFFER_SIZE)
        for (iCount = 0; iCount < iWidth; iCount++) usTemp[iCount] = usPalette[*s++];
      else
        for (iCount = 0; iCount < BUFFER_SIZE; iCount++) usTemp[iCount] = usPalette[*s++];

      //background.pushPixels(&usTemp[0][0], iCount);
      //lcd_PushColors(usTemp[0], iCount);
      pushImage(pDraw->iX + x, y, iCount, 1, usTemp);
      iWidth -= iCount;
    }
  }
} 

void amoledDisplay_Init(void)
{
  rm67162_init();
  lcd_setRotation(3);

  background.createSprite(WIDTH, HEIGHT);
  background.setSwapBytes(true);
  gifSprite.createSprite(G_WIDTH, G_HEIGHT);
  gifSprite.setSwapBytes(true);

  render.setDrawer(background);
  render.setLineSpaceRatio(0.9);

  if (render.loadFont(DigitalNumbers, sizeof(DigitalNumbers)))
  {
    Serial.println("Initialise error");
    return;
  }

  gif.begin(GIF_PALETTE_RGB565_LE);
  gif.open((uint8_t *)GIF_IMAGE, sizeof(GIF_IMAGE), GIFDraw);
}

int screen_state = 1;
void amoledDisplay_AlternateScreenState(void)
{
  screen_state == 1 ? lcd_off() : lcd_on();
  screen_state ^= 1;
}

int screen_rotation = 1;
void amoledDisplay_AlternateRotation(void)
{
  screen_rotation == 1 ? lcd_setRotation(1) : lcd_setRotation(3);
  screen_rotation ^= 1;
}

void amoledDisplay_MinerScreen(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);

  // Print background screen
  background.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Hashrate
  render.setFontSize(49);
  render.rdrawString(data.currentHashRate.c_str(), 468, 169, PINK, PINK);  

  // Total hashes
  render.setFontSize(24);
  render.drawString(data.totalMHashes.c_str(), 104, 201, PINK, PINK);
  // Hores
  render.rdrawString(data.hr.c_str(), 26 + 129, 159, TFT_WHITE);
  //Minutss
  render.rdrawString(data.min.c_str(), 169 + 36, 159, TFT_WHITE);
  //Segons
  render.rdrawString(data.sec.c_str(), 222 + 36, 159, TFT_WHITE);  

  // Block templates
  render.setFontSize(13);
  render.drawString(data.templates.c_str(), 346, 59, TFT_WHITE);
  // Best diff
  render.drawString(data.bestDiff.c_str(), 346, 80, TFT_WHITE);
  // 32Bit shares
  render.drawString(data.completedShares.c_str(), 346, 102, TFT_WHITE);
  // Print Temp
  render.rdrawString(data.temp.c_str(), 375 + 50, 4, TFT_WHITE);
  // Print Hour
  render.rdrawString(data.currentTime.c_str(), 448 + 52, 4, TFT_WHITE);  

  // Valid Blocks
  render.setFontSize(40);
  render.drawString(data.valids.c_str(), 459, 78, TFT_WHITE);
}

void amoledDisplay_ClockScreen(unsigned long mElapsed)
{
  clock_data data = getClockData(mElapsed);

  // Print background screen
  background.pushImage(0, 0, minerClockWidth, minerClockHeight, minerClockScreen);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  render.setFontSize(59);
  render.drawString(data.currentTime.c_str(), 294, 51, TFT_WHITE, TFT_WHITE);    

  // Hashrate
  render.setFontSize(49);
  render.drawString(data.currentHashRate.c_str(), 62, 169, PINK, PINK);  

  int lmargin = 260;
  //Hores
  render.setFontSize(24);
  render.rdrawString(data.hr.c_str(), lmargin + 26 + 129, 158, TFT_WHITE);
  //Minutss
  render.rdrawString(data.min.c_str(), lmargin + 169 + 36, 158, TFT_WHITE);
  //Segons
  render.rdrawString(data.sec.c_str(), lmargin + 222 + 36, 158, TFT_WHITE);
  //Total hashes
  render.rdrawString(data.totalMHashes.c_str(), 428, 201, PINK, PINK);

  //Print Temp
  render.setFontSize(13);
  render.rdrawString(data.temp.c_str(), 450 + 50, 4, TFT_WHITE);  

}

void amoledDisplay_LoadingScreen(void)
{
  background.fillScreen(TFT_BLACK);
  background.pushImage(0, 0, initWidth, initHeight, initScreen);
  lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)background.getPointer());
}

void amoledDisplay_SetupScreen(void)
{
  background.pushImage(0, 0, setupModeWidth, setupModeHeight, setupModeScreen);
  lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)background.getPointer());
}

void amoledDisplay_AnimateCurrentScreen(unsigned long frame)
{
  gif.playFrame(false, NULL);
  gifSprite.pushImage(0, 0, G_WIDTH, G_HEIGHT, gifBuf);
  if(frame == 0){
    lcd_PushColors(0, 0, G_WIDTH, G_HEIGHT, (uint16_t *)gifSprite.getPointer());
  } else {
    gifSprite.pushToSprite(&background, 0, 0);
    lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)background.getPointer());
  }
}

void amoledDisplay_DoLedStuff(unsigned long frame)
{
}

CyclicScreenFunction amoledDisplayCyclicScreens[] = {amoledDisplay_MinerScreen, amoledDisplay_ClockScreen};

DisplayDriver nyanAmoledDisplayDriver = {
    amoledDisplay_Init,
    amoledDisplay_AlternateScreenState,
    amoledDisplay_AlternateRotation,
    amoledDisplay_LoadingScreen,
    amoledDisplay_SetupScreen,
    amoledDisplayCyclicScreens,
    amoledDisplay_AnimateCurrentScreen,
    amoledDisplay_DoLedStuff,
    SCREENS_ARRAY_SIZE(amoledDisplayCyclicScreens),
    1,
    WIDTH,
    HEIGHT};
#endif