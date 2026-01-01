#include "displayDriver.h"

#ifdef ST7789_240x240_DISPLAY

#include <TFT_eSPI.h>
#include "media/images_240_175.h"
#include "media/images_240_65.h"
#include "media/myFonts.h"
#include "media/Free_Fonts.h"
#include "version.h"
#include "monitor.h"
#include "OpenFontRender.h"
#include "drivers/storage/nvMemory.h"
#include "rotation.h"

#define WIDTH 240
#define HEIGHT 175

OpenFontRender render;
TFT_eSPI tft = TFT_eSPI();                  // Invoke library, pins defined in User_Setup.h
TFT_eSprite background = TFT_eSprite(&tft); // Invoke library sprite

extern nvMemory nvMem;

extern monitor_data mMonitor;
extern pool_data pData;
bool hasChangedScreen = true;
extern unsigned long mPoolUpdate;
extern TSettings Settings;
extern DisplayDriver *currentDisplayDriver;
// extern global_data gData;
int current_screen = NO_SCREEN;

void printheap(){
  Serial.print("$$ Free Heap:");
  Serial.println(ESP.getFreeHeap()); 
  // Serial.printf("### stack WMark usage: %d\n", uxTaskGetStackHighWaterMark(NULL));
}

// void switchToNextScreen()
// {
//   currentDisplayDriver->current_cyclic_screen = (currentDisplayDriver->current_cyclic_screen + 1) % currentDisplayDriver->num_cyclic_screens;
// }

void custom2_Display_Init(void)
{
  tft.init();
  tft.setRotation(PORTRAIT);
  tft.setSwapBytes(true);                 // Swap the colour byte order when rendering
  // tft.invertDisplay(true);
  background.createSprite(WIDTH, HEIGHT); // Background Sprite
  background.setSwapBytes(true);

  if (!background.created()) {    
    Serial.println("#### Sprite Error ####");
  // Serial.printf("Pool data W:%d H:%s D:%s\n", pData.workersCount, pData.workersHash, pData.bestDifficulty);
  Serial.printf("Size w:%d h:%d \n", WIDTH, HEIGHT);
    printheap();        
  } 
  
  render.setDrawer(background);  // Link drawing object to background instance (so font will be rendered on background)
  render.setLineSpaceRatio(0.9); // Espaciado entre texto

  // Load the font and check it can be read OK
  // if (render.loadFont(NotoSans_Bold, sizeof(NotoSans_Bold))) {
  if (render.loadFont(DigitalNumbers, sizeof(DigitalNumbers)))
  {
    Serial.println("Initialise error");
    return;
  }

  pinMode(LED_PIN, OUTPUT),
  pinMode(LED_PIN_G, OUTPUT),
  
  digitalWrite(LED_PIN, LOW);
  digitalWrite(LED_PIN_G, LOW);
}

bool createBackgroundSprite(int16_t wdt, int16_t hgt){  // Set the background and link the render, used multiple times to fit in heap
  background.createSprite(wdt, hgt) ; //Background Sprite
  // printheap();
  if (background.created()) {
      background.setColorDepth(16);
      background.setSwapBytes(true);
      render.setDrawer(background); // Link drawing object to background instance (so font will be rendered on background)
      render.setLineSpaceRatio(0.9);      
  } else {
    Serial.println("#### Sprite Error ####");
    Serial.printf("Size w:%d h:%d \n", wdt, hgt);
    printheap();
  }
  return background.created();
}

void custom2_Display_AlternateScreenState(void)
{
  int screen_state = digitalRead(TFT_BL);
  Serial.println("Switching display state");
  digitalWrite(TFT_BL, !screen_state);
}

void custom2_Display_AlternateRotation(void)
{
  tft.setRotation(rotationRight(tft.getRotation()) );
  hasChangedScreen = true;
}

void printPoolData(){
  if ((hasChangedScreen) || (mPoolUpdate == 0) || (millis() - mPoolUpdate > UPDATE_POOL_min * 60 * 1000)){     
      if (Settings.PoolAddress != "tn.vkbit.com") { 
          pData = getPoolData();             
          background.createSprite(poolStatsBottomWidth, poolStatsBottomHeight); //Background Sprite
          if (!background.created()) {    
            Serial.println("###### POOL SPRITE ERROR ######");
          // Serial.printf("Pool data W:%d H:%s D:%s\n", pData.workersCount, pData.workersHash, pData.bestDifficulty);
            printheap();        
          }       
          background.setSwapBytes(true);
          // if (bottomScreenBlue) {
          background.pushImage(0, -20, WIDTH, poolStatsBottomHeight, poolStatsBottom);
          tft.pushImage(0, 175, WIDTH, 20, poolStatsBottom);      

          render.setDrawer(background); // Link drawing object to background instance (so font will be rendered on background)
          render.setLineSpaceRatio(1);
          
          render.setFontSize(20);
          render.setAlignment(Align::BottomCenter);
          render.cdrawString(String(pData.workersCount).c_str(), 120, 36-20, TFT_BLACK);
          render.setFontSize(18);
          render.setAlignment(Align::BottomRight);
          render.cdrawString(pData.workersHash.c_str(), 200, 36-20, TFT_BLACK);
          render.setAlignment(Align::BottomLeft);
          render.cdrawString(pData.bestDifficulty.c_str(), 40, 36-20, TFT_BLACK);

          background.pushSprite(0,195);      
          background.deleteSprite();
      } else {
        pData.bestDifficulty = "TESTNET";
        pData.workersHash = "TESTNET";
        pData.workersCount = 1;
        tft.fillRect(0,170,WIDTH,70, TFT_DARKGREEN);        
        background.createSprite(WIDTH,40); //Background Sprite
        background.fillSprite(TFT_DARKGREEN);
          if (!background.created()) {    
            Serial.println("###### POOL SPRITE ERROR ######");
          // Serial.printf("Pool data W:%d H:%s D:%s\n", pData.workersCount, pData.workersHash, pData.bestDifficulty);
            printheap();        
          }
        background.setFreeFont(FF24);
        background.setTextDatum(TL_DATUM);
        background.setTextSize(1);
        background.setTextColor(TFT_WHITE, TFT_DARKGREEN);        
        background.drawString("TESTNET", 50, 0, GFXFF);
        background.pushSprite(0,185);  
        mPoolUpdate = millis();
        Serial.println("Testnet");
        background.deleteSprite();
      }
  }
}

void custom2_Display_MinerScreen(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);

  
  if (hasChangedScreen || (current_screen!=SCREEN_MINING)) tft.pushImage(0, 0, minerScreenWidth, minerScreenHeight, minerScreen);
  current_screen=SCREEN_MINING;
  
  
  printPoolData();
  hasChangedScreen = false; 
  // mMonitor.NerdStatus=NM_hashing;

  //  ------------------- block info
  createBackgroundSprite(140, 100);
  // background.fillSprite(TFT_RED);
  background.pushImage(-100, -20, minerScreenWidth, minerScreenHeight, minerScreen);
  
  render.setFontSize(16);
  render.setAlignment(Align::TopLeft);
  render.drawString(data.templates.c_str(), 40, 0, 0xDEDB);
  // Best diff
  render.drawString(data.bestDiff.c_str(), 40, 25, 0xDEDB);
  // // 32Bit shares
  // render.setFontSize(18);
  render.drawString(data.completedShares.c_str(), 40, 50, 0xDEDB);
  // // Hores
  render.setFontSize(14);

  char timeMining[15]; 
  unsigned long secElapsed = millis() / 1000;
  int days = secElapsed / 86400; 
  int hours = (secElapsed - (days * 86400)) / 3600;                                                        //Number of seconds in an hour
  int mins = (secElapsed - (days * 86400) - (hours * 3600)) / 60;                                              //Remove the number of hours and calculate the minutes.
  int secs = secElapsed - (days * 86400) - (hours * 3600) - (mins * 60);   
  sprintf(timeMining, "%01d %02d:%02d:%02d", days, hours, mins, secs);

  render.rdrawString(String(timeMining).c_str(), 135, 78, 0xDEDB);
  

  background.pushSprite(100, 20);
  background.deleteSprite();

  // ---------------------- valid block
  createBackgroundSprite(50, 45);
  background.fillSprite(TFT_BLUE);
  background.pushImage(-18, -68, minerScreenWidth, minerScreenHeight, minerScreen);
  
  render.setFontSize(28);
  render.setAlignment(Align::BottomCenter);
  render.cdrawString(data.valids.c_str(), 24, 3, 0xDEDB);

  background.pushSprite(18, 68);
  background.deleteSprite();
  
  // ----------------------- hashing rate
  createBackgroundSprite(240, 30);
  // background.fillSprite(TFT_GREEN);
  background.pushImage(0, -128, minerScreenWidth, minerScreenHeight, minerScreen);
  
  render.setFontSize(24);
  render.rdrawString(data.totalMHashes.c_str(), 230, 0, TFT_BLACK);
  render.rdrawString(data.currentHashRate.c_str(), 120, 0, TFT_BLACK);

  background.pushSprite(0, 128);
  background.deleteSprite();

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());
}

unsigned long previousMillisGlobal = 0;

void custom2_Display_GlobalHashScreen(unsigned long mElapsed)
{

    printPoolData();
    hasChangedScreen = false;

    if (hasChangedScreen || (current_screen!=SCREEN_GLOBAL))  tft.pushImage(0, 0, globalStatsWidth, globalStatsHeight, globalStats);
    current_screen=SCREEN_GLOBAL;
    
    coin_data data = getCoinData(mElapsed, false);
    mining_data mining_data = getMiningData(mElapsed);
    // 
    // global_data g_data;
    // printheap();
    String globalHash = getGlobalHashRate();
    
  //   if ((millis() - previousMillisGlobal) >= 10000){
  //     Serial.println(" ------ ciaoooo ");
  //     previousMillisGlobal=millis();
  //   }
    //   updateGlobalData();
    //   previousMillisGlobal = millis();
    // } else {
      
    // }

    // Serial.println("globals -----------");
    // String btcPrice = getBTCprice();
    // String currHashRate = getCurrentHashRate(mElapsed);
    // String time = getTime();
    // String blkHeight = getBlockHeight();
    Serial.println(" ------ globals ");
    Serial.println(data.btcPrice);
    Serial.println(globalHash);
    Serial.println(data.currentTime);
    Serial.println(data.blockHeight);
    Serial.println(data.remainingBlocks);
    Serial.println(" ---------------");

    // String btcPrice = getBTCprice();
    // String blkHeight = getBlockHeight();
    // String currHashRate = getCurrentHashRate(mElapsed);
    // String time = getTime();
    // ---------------------- bitcoin price
    createBackgroundSprite(100, 20);
    background.fillSprite(TFT_RED);
    // background.pushImage(-138, 0, globalStatsWidth, globalStatsHeight, globalStats);
    
    // Print BTC Price
    background.setFreeFont(FSSB9);
    background.setTextSize(1);
    background.setTextDatum(TL_DATUM);
    background.setTextColor(0xE71C);
    background.drawString(data.globalHashRate.c_str(), 0, 3, GFXFF);

    background.pushSprite(138, 0);
    background.deleteSprite();

    // // ---------------------- block progress
    // createBackgroundSprite(210, 28);
    // // background.fillSprite(TFT_GREEN);
    // background.pushImage(0, -146, globalStatsWidth, globalStatsHeight, globalStats);
    

    // render.setFontSize(20);
    // render.setAlignment(Align::TopRight);
    // render.rdrawString(data.globalHashRate.c_str(), 205, 0, TFT_BLACK);

    // // Draw percentage rectangle
    // int x2 = 1 + (87 * data.progressPercent / 100);
    // background.fillRect(1, 28-19, x2, 18, 0xDEDB);

    // background.setTextFont(FONT2);
    // background.setTextSize(1); 
    // background.setTextDatum(MC_DATUM);
    // background.setTextColor(TFT_BLACK);
    // background.drawString(data.remainingBlocks.c_str(), 43, 28-10, FONT2);
    
    // background.pushSprite(0, 146);
    // background.deleteSprite();

    // // ---------------------- block properties
    // createBackgroundSprite(120, 60);
    // // background.fillSprite(TFT_MAGENTA);
    // // background.pushImage(0, 0, globalStatsWidth, globalStatsHeight, globalStats);
    
    // background.pushSprite(120, 49);
    // background.deleteSprite();

    // // ---------------------- current block
    // createBackgroundSprite(95, 40);
    // // background.fillSprite(TFT_BLUE);
    // // background.pushImage(0, 0, globalStatsWidth, globalStatsHeight, globalStats);
    
    // background.pushSprite(0, 98);
    // background.deleteSprite();

    Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                mining_data.completedShares.c_str(), mining_data.totalKHashes.c_str(), mining_data.currentHashRate.c_str());

}


void custom2_Display_LoadingScreen(void)
{
  tft.fillScreen(TFT_BLACK);
  tft.pushImage(0, 0, initScreenWidth, initScreenHeight, initScreen);
  tft.setTextColor(TFT_GOLD);
  tft.drawString(CURRENT_VERSION, 2, 100, FONT4); 
}

void custom2_Display_SetupScreen(void)
{
  tft.pushImage(0, 0, configScreenTopWidth, configScreenTopHeight, configScreenTop);
  tft.pushImage(0, 175, configScreenBottomWidth, configScreenBottomHeight, configScreenBottom);
}

void custom2_Display_AnimateCurrentScreen(unsigned long frame)
{
}

unsigned long previousMillis = 0;

void custom2_Display_DoLedStuff(unsigned long frame)
{
  unsigned long currentMillis = millis();
  switch (mMonitor.NerdStatus)
  {
  case NM_waitingConfig:
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(LED_PIN_G, LOW);
    break;

  case NM_Connecting:
    if (currentMillis - previousMillis >= 500)
    { // 0.5sec blink
      previousMillis = currentMillis;
      int led_state = digitalRead(LED_PIN);
      digitalWrite(LED_PIN, !led_state); // Cambia el estado del LED
      digitalWrite(LED_PIN_G, led_state); // Cambia el estado del LED
    }
    break;

  case NM_hashing:
    digitalWrite(LED_PIN, LOW);
    digitalWrite(LED_PIN_G, HIGH);
    break;
  }
}

CyclicScreenFunction custom2_DisplayCyclicScreens[] = {custom2_Display_GlobalHashScreen, custom2_Display_MinerScreen};

DisplayDriver custom2_DisplayDriver = {
    custom2_Display_Init,
    custom2_Display_AlternateScreenState,
    custom2_Display_AlternateRotation,
    custom2_Display_LoadingScreen,
    custom2_Display_SetupScreen,
    custom2_DisplayCyclicScreens,
    custom2_Display_AnimateCurrentScreen,
    custom2_Display_DoLedStuff,
    SCREENS_ARRAY_SIZE(custom2_DisplayCyclicScreens),
    0,
    WIDTH,
    HEIGHT};
#endif
