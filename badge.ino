/*
  Project Name:   badge
  Description:    conference badge displaying interesting information

  See README.md for target information and revision history
*/

// hardware and internet configuration parameters
#include "config.h"

// QR code support
#include "qrcoderm.h"

// hardware support

#ifndef HARDWARE_SIMULATE
  // instanstiate SCD4X hardware object
  #include <SensirionI2CScd4x.h>
  SensirionI2CScd4x co2Sensor;

  // battery voltage sensor
  #include <Adafruit_LC709203F.h>
  Adafruit_LC709203F lc;
#endif

// neopixels
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel neopixels = Adafruit_NeoPixel(neoPixelCount, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// button support
#include <ezButton.h>
ezButton buttonOne(buttonD1Pin);

// e-ink support
#include <Adafruit_ThinkInk.h>
// 2.96" greyscale display with 296x128 pixels
// colors are EPD_WHITE, EPD_BLACK, EPD_GRAY, EPD_LIGHT, EPD_DARK
ThinkInk_290_Grayscale4_T5 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

#include <Fonts/FreeSans9pt7b.h>    // screenMain
#include <Fonts/FreeSans12pt7b.h>   // screenAlert, screenName, screenThreeThings
#include <Fonts/FreeSans18pt7b.h>   // screenThreeThings
#include <Fonts/FreeSans24pt7b.h>   // screenMain

#include "Fonts/meteocons24pt7b.h"  //screenCO2

// Special glyphs for screenCO2
#include "Fonts/glyphs.h"

// global variables

// environment sensor data
typedef struct {
  float     ambientTemperatureF;
  float     ambientHumidity;     // RH [%]  
  uint16_t  ambientCO2;
} envData;
envData sensorData;

// hardware status data
typedef struct
{
  float batteryPercent;
  float batteryVoltage;
  float batteryTemperatureF;
  //uint8_t rssi;
} hdweData;
hdweData hardwareData;

// int32_t timeLastSensorSample  = -(sensorSampleInterval*1000);  // negative value triggers sensor sample on first iteration of loop()
uint32_t timeLastSensorSample;
uint32_t timeLastScreenSwap;
uint8_t screenCurrent = 0; // tracks which screen is displayed

void setup()
{
  hardwareData.batteryVoltage = 0.0;  // 0 = no battery attached

  #ifdef DEBUG
    Serial.begin(115200);
    // wait for serial port connection
    while (!Serial)
    debugMessage(String("Starting badge with ") + sensorSampleInterval + " second sample interval",1);
  #endif

  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch (wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_TIMER : // do nothing
    {
      debugMessage("wakeup cause: timer",1);
    }
    break;
    case ESP_SLEEP_WAKEUP_EXT0 :
    {
      debugMessage("wakeup cause: RTC gpio pin",1);
    }
    break;
    case ESP_SLEEP_WAKEUP_EXT1 :
    {
      uint16_t gpioReason = log(esp_sleep_get_ext1_wakeup_status())/log(2);
      debugMessage(String("wakeup cause: RTC gpio pin: ") + gpioReason,1);
      // implment switch (gpioReason)
    }
    break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : 
    {
      debugMessage("wakup cause: touchpad",1);
    }  
    break;
    case ESP_SLEEP_WAKEUP_ULP : 
    {
      debugMessage("wakeup cause: program",1);
    }  
    break; 
    default :
    {
      // likely caused by reset after firmware load
      debugMessage(String("Wakeup likely cause: first boot after firmware flash, reason: ") + wakeup_reason,1);
    }
    break;
  }

  powerNeoPixelEnable();

  // Initialize e-ink screen
  // there is no way to query e-ink screen to check for successful initialization
  //display.begin(THINKINK_GRAYSCALE4);
  display.begin(THINKINK_MONO);
  display.setRotation(screenRotation);
  display.setTextWrap(false);
  // debugMessage (String("epd: enabled in grayscale with rotation") + screenRotation,1);
  debugMessage(String("epd: enabled in mono with rotation: ") + screenRotation,1);

  // Initialize SCD4X
  if (!sensorCO2Init())
  {
    debugMessage("SCD4X initialization failure",1);
    screenAlert("No SCD4X");
    // This error often occurs right after a firmware flash and reset.
    // Hardware deep sleep typically resolves it
    powerDisable(hardwareErrorSleepTime);
  }

  buttonOne.setDebounceTime(buttonDebounceDelay);

  // first tme screen draw
  if (!sensorCO2Read())
  {
    screenAlert("CO2 read fail");
  }
  timeLastSensorSample = millis();
  screenUpdate();
  timeLastScreenSwap = millis();
} 

void loop()
{
  buttonOne.loop();
  if (buttonOne.isReleased())
  {
    ((screenCurrent + 1) >= screenCount) ? screenCurrent = 0 : screenCurrent ++;
    debugMessage(String("button 1 press, switch to screen ") + screenCurrent,1);
    screenUpdate();
  }

  // is it time to update the sensor values?
  if((millis() - timeLastSensorSample) >= (sensorSampleInterval * 1000)) // converting sensorSampleInterval into milliseconds
  {
    if (!sensorCO2Read())
    {
      screenAlert("CO2 read fail");
    }
    // Save current timestamp to restart sample delay interval
    timeLastSensorSample = millis();
  }

  // is it time to swap screens?
  if((millis() - timeLastScreenSwap) >= (screenSwapInterval * 1000)) // converting screenSwapInterval into milliseconds
  {
    ((screenCurrent + 1) >= screenCount) ? screenCurrent = 0 : screenCurrent ++;
    debugMessage(String("screen swap timer triggered, switch to screen ") + screenCurrent,1);
    screenUpdate();
    timeLastScreenSwap = millis();
  }

  // is it time to go to sleep?
  if((millis()) >= (sleepInterval * 1000)) // converting sensorSampleInterval into milliseconds
  {
    // redraw main screen
    screenCurrent = 0;
    screenUpdate();
    // go to sleep
    powerDisable(sleepTime);
  }
}

void screenUpdate()
// Description: Display requested screen
// Parameters: NA (global)
// Output: NA (void)
// Improvement: ?
{
  neoPixelCO2();
  switch(screenCurrent) {
    case 0:
      screenMain(badgeNameFirst, badgeNameLast, qrCodeURL);
      break;
    case 1:
      screenThreeThings(badgeFirstThing, badgeSecondThing, badgeThirdThing);
      break;
    case 2:
      screenCO2();
      break;
    case 3:
      screenSensors();
      break;
    default:
      // handle out of range screenCurrent
      screenMain(badgeNameFirst, badgeNameLast, qrCodeURL);
      debugMessage("Error: Unexpected screen ID",1);
      break;
  }
}

bool screenAlert(String messageText)
// Description: Display error message centered on screen, using different font sizes and/or splitting to fit on screen
// Parameters: String containing error message text
// Output: NA (void)
// Improvement: ?
{
  debugMessage("screenAlert start",1);

  bool status = true;
  int16_t x1, y1;
  uint16_t largeFontPhraseOneWidth, largeFontPhraseOneHeight;
  uint16_t smallFontWidth, smallFontHeight;

  display.clearBuffer();
  display.setTextColor(EPD_BLACK);

  display.setFont(&FreeSans12pt7b);
  display.getTextBounds(messageText.c_str(), 0, 0, &x1, &y1, &largeFontPhraseOneWidth, &largeFontPhraseOneHeight);
  if (largeFontPhraseOneWidth <= (display.width()-(display.width()/2-(largeFontPhraseOneWidth/2))))
  {
    // fits with large font, display
    display.setCursor(display.width()/2-largeFontPhraseOneWidth/2,display.height()/2+largeFontPhraseOneHeight/2);
    display.print(messageText);
  }
  else
  {
    debugMessage(String("ERROR: screenAlert messageText '") + messageText + "' with large font is " + abs(largeFontPhraseOneWidth - (display.width()-(display.width()/2-(largeFontPhraseOneWidth/2)))) + " pixels too long", 1);
    // does the string break into two pieces based on a space character?
    uint8_t spaceLocation;
    String messageTextPartOne, messageTextPartTwo;
    uint16_t largeFontPhraseTwoWidth, largeFontPhraseTwoHeight;

    spaceLocation = messageText.indexOf(' ');
    if (spaceLocation)
    {
      // has a space character, will it fit on two lines?
      messageTextPartOne = messageText.substring(0,spaceLocation);
      messageTextPartTwo = messageText.substring(spaceLocation+1);
      display.getTextBounds(messageTextPartOne.c_str(), 0, 0, &x1, &y1, &largeFontPhraseOneWidth, &largeFontPhraseOneHeight);
      display.getTextBounds(messageTextPartTwo.c_str(), 0, 0, &x1, &y1, &largeFontPhraseTwoWidth, &largeFontPhraseTwoHeight);
      if ((largeFontPhraseOneWidth <= (display.width()-(display.width()/2-(largeFontPhraseOneWidth/2)))) && (largeFontPhraseTwoWidth <= (display.width()-(display.width()/2-(largeFontPhraseTwoWidth/2)))))
      {
        // fits on two lines, display
        display.setCursor(display.width()/2-largeFontPhraseOneWidth/2,(display.height()/2+largeFontPhraseOneHeight/2)+6);
        display.print(messageTextPartOne);
        display.setCursor(display.width()/2-largeFontPhraseOneWidth/2,(display.height()/2+largeFontPhraseOneHeight/2)-18);
        display.print(messageTextPartTwo);
      }
    }
    debugMessage(String("Message part one with large fonts is ") + largeFontPhraseOneWidth + " pixels wide vs. available " + (display.width()-(display.width()/2-(largeFontPhraseOneWidth/2))) + " pixels",1);
    debugMessage(String("Message part two with large fonts is ") + largeFontPhraseTwoWidth + " pixels wide vs. available " + (display.width()-(display.width()/2-(largeFontPhraseTwoWidth/2))) + " pixels",1);
    // at large font size, string doesn't fit even if it can be broken into two lines
    // does the string fit with small size text?
    display.setFont(&FreeSans9pt7b);
    display.getTextBounds(messageText.c_str(), 0, 0, &x1, &y1, &smallFontWidth, &smallFontHeight);
    if (smallFontWidth <= (display.width()-(display.width()/2-(smallFontWidth/2))))
    {
      // fits with small size
      display.setCursor(display.width()/2-smallFontWidth/2,display.height()/2+smallFontHeight/2);
      display.print(messageText);
    }
    else
    {
      debugMessage(String("ERROR: screenAlert messageText '") + messageText + "' with small font is " + abs(smallFontWidth - (display.width()-(display.width()/2-(smallFontWidth/2)))) + " pixels too long", 1);
      // doesn't fit at any size/line split configuration, display as truncated, large text
      display.setFont(&FreeSans12pt7b);
      display.getTextBounds(messageText.c_str(), 0, 0, &x1, &y1, &largeFontPhraseOneWidth, &largeFontPhraseOneHeight);
      display.setCursor(display.width()/2-largeFontPhraseOneWidth/2,display.height()/2+largeFontPhraseOneHeight/2);
      display.print(messageText);
      status = false;
    }
  }
  display.display();
  debugMessage("screenAlert end",1);
  return status;
}

void screenMain(String firstName, String lastName, String url)
// Description: Display first and last name, QRCode, and CO2 value in vertical orientation
// Parameters: 
// Output: NA (void)
// Improvement: ?
{
  debugMessage("screenMain start",1);

  // screen layout assists
  const uint16_t xMidMargin = ((display.width()/2) + xMargins);
  const uint16_t yTempF = 36 + yTopMargin;
  const uint16_t yHumidity = 90;
  const uint16_t yCO2 = 40;

  display.clearBuffer();

  display.setTextColor(EPD_BLACK);
  // name
  display.setFont(&FreeSans24pt7b);
  display.setCursor(xMargins,48);
  display.print(firstName);
  display.setFont(&FreeSans12pt7b);
  display.setCursor(xMargins,72);
  display.print(lastName);
  // QR code
  screenHelperQRCode(12,98,url);

  // co2 main line
  display.setFont(&FreeSans12pt7b);
  display.setCursor(xMargins, 240);
  display.print("CO");
  display.setCursor(xMargins+50,240);
  display.print(" " + co2Labels[co2Range(sensorData.ambientCO2)]);
  display.setFont(&FreeSans9pt7b);
  display.setCursor(xMargins+35,(240+10));
  display.print("2");
  // co2 value
  display.setFont();
  display.setCursor((xMargins+80),(240+7));
  display.print("(" + String(sensorData.ambientCO2) + ")");

  // draw battery in the lower right corner. -3 in first parameter accounts for battery nub
  screenHelperBatteryStatus((display.width()-xMargins-batteryBarWidth-3),(display.height()-yBottomMargin-batteryBarHeight),batteryBarWidth,batteryBarHeight);
  display.display();
  debugMessage("screenMain end",1);
}

void screenCO2()
// Description: Display current, high, and low CO2 values plus sparkline in vertical orientation
// Parameters: 
// Output: NA (void)
// Improvement: ?
{
  debugMessage("screenCO2 start",1);

  // screen layout assists
  const uint16_t xMidMargin = ((display.width()/2) - 40);
  const uint16_t yCO2Label = 40;
  const uint16_t yCO2Ambient = 80;
  const uint16_t yCO2High = 120;
  const uint16_t yCO2Low = 160;
  const uint16_t ySparkline = 220;
  const uint16_t sparklineHeight = ((display.height()/2)-(yBottomMargin));

  display.clearBuffer();
  display.setTextColor(EPD_BLACK);

  // label
  display.setFont(&FreeSans24pt7b);
  display.setCursor(xMidMargin, yCO2Label);
  display.print("CO2");

  // ambient CO2
  display.setFont(&FreeSans12pt7b);
  display.setCursor(xMidMargin, yCO2Ambient);
  display.print("Current");
  display.setFont(&FreeSans24pt7b);
  display.setCursor((xMargins),(yCO2Ambient+35));
  display.print(sensorData.ambientCO2);

  // high CO2
  display.setFont(&FreeSans12pt7b);
  display.setCursor(xMidMargin, yCO2High);
  display.print("High");
  display.setFont(&FreeSans24pt7b);
  display.setCursor((xMargins),(yCO2High+35));
  display.print("1200");

    // low CO2
  display.setFont(&FreeSans12pt7b);
  display.setCursor(xMidMargin, yCO2Low);
  display.print("Low");
  display.setFont(&FreeSans24pt7b);
  display.setCursor((xMargins),(yCO2Low+35));
  display.print("432");

  // CO2 sparkline
  screenHelperSparkLine(xMargins,ySparkline,(display.width() - (2 * xMargins)),display.height()-ySparkline-yTopMargin);

  // // draws battery in the lower right corner. -3 in first parameter accounts for battery nub
  // screenHelperBatteryStatus((display.width()-xMargins-batteryBarWidth-3),(display.height()-yBottomMargin-batteryBarHeight),batteryBarWidth,batteryBarHeight);

  display.display();
  debugMessage("screenCO2 end",1);
}

void screenThreeThings(String firstThing, String secondThing, String thirdThing)
// Description: Display three things about the badge owner in vertical orientation
// Parameters: three strings describing owner
// Output: NA
// Improvement: if too long, split into two lines?
{
  debugMessage("screenThreeThings start",1);

  int16_t x1, y1;
  uint16_t largeFontWidth, largeFontHeight;
  uint16_t smallFontWidth, smallFontHeight;

  display.clearBuffer();
  display.setTextColor(EPD_BLACK);

  display.setFont(&FreeSans12pt7b);
  display.setCursor(xMargins,24);
  display.print("I love");

  // display firstThing
  display.setCursor(xMargins,94);
  display.setFont(&FreeSans18pt7b);
  display.getTextBounds(firstThing.c_str(), 0, 0, &x1, &y1, &largeFontWidth, &largeFontHeight);
  if (largeFontWidth >= (display.width()-xMargins))
  {
    debugMessage(String("ERROR: firstThing '") + firstThing + "' with large font is " + abs(display.width()-largeFontWidth-xMargins) + " pixels too long", 1);
    display.setFont(&FreeSans12pt7b);
    display.getTextBounds(firstThing.c_str(), 0, 0, &x1, &y1, &smallFontWidth, &smallFontHeight);
    if (smallFontWidth >= (display.width()-xMargins))
    {
      // text is too long even if we shrink text size
      // display it truncated at larger size
      // IMPROVEMENT: Is it two words we can split?
      debugMessage(String("ERROR: firstThing '") + firstThing + "' with small font is " + abs(display.width()-smallFontWidth-xMargins) + " pixels too long", 1);
      display.setFont(&FreeSans18pt7b);
    }
  }
  display.print(firstThing);

  // display secondThing
  display.setCursor(xMargins,184);
    display.setFont(&FreeSans18pt7b);
  display.getTextBounds(secondThing.c_str(), 0, 0, &x1, &y1, &largeFontWidth, &largeFontHeight);
  if (largeFontWidth >= (display.width()-xMargins))
  {
    debugMessage(String("ERROR: secondThing '") + secondThing + "' with large font is " + abs(display.width()-largeFontWidth-xMargins) + " pixels too long", 1);
    display.setFont(&FreeSans12pt7b);
    display.getTextBounds(secondThing.c_str(), 0, 0, &x1, &y1, &smallFontWidth, &smallFontHeight);
    if (smallFontWidth >= (display.width()-xMargins))
    {
      // text is too long even if we shrink text size
      // display it truncated at larger size
      // IMPROVEMENT: Is it two words we can split?
      debugMessage(String("ERROR: secondThing '") + secondThing + "' with small font is " + abs(display.width()-smallFontWidth-xMargins) + " pixels too long", 1);
      display.setFont(&FreeSans18pt7b);
    }
  }
  display.print(secondThing);

  // display thirdThing
  display.setCursor(xMargins,274);
  display.setFont(&FreeSans18pt7b);
  display.getTextBounds(thirdThing.c_str(), 0, 0, &x1, &y1, &largeFontWidth, &largeFontHeight);
  if (largeFontWidth >= (display.width()-xMargins))
  {
    debugMessage(String("ERROR: thirdThing '") + thirdThing + "' with large font is " + abs(display.width()-largeFontWidth-xMargins) + " pixels too long", 1);
    display.setFont(&FreeSans12pt7b);
    display.getTextBounds(thirdThing.c_str(), 0, 0, &x1, &y1, &smallFontWidth, &smallFontHeight);
    if (smallFontWidth >= (display.width()-xMargins))
    {
      // text is too long even if we shrink text size
      // display it truncated at larger size
      // IMPROVEMENT: Is it two words we can split?
      debugMessage(String("ERROR: thirdThing '") + thirdThing + "' with small font is " + abs(display.width()-smallFontWidth-xMargins) + " pixels too long", 1);
      display.setFont(&FreeSans18pt7b);
    }
  }
  display.print(thirdThing);

  display.display();
  debugMessage("screenThreeThings end",1);
}

void screenSensors()
// Description: Display ambient temp, humidity, and CO2 level and sparklines for each in vertical orientation
// Parameters: NA (globals)
// Output: NA (void)
// Improvement: ?
{
  debugMessage("screenSensors start",1);

  // screen layout assists
  const uint16_t xMidMargin = (display.width()/2);
  const uint16_t yscreenLabel = 40;
  const uint16_t yCO2Label = 80;
  const uint16_t yTempLabel = 120;
  const uint16_t yHumidityLabel = 180;
  const uint16_t yCO2Value = yscreenLabel + 24;
  const uint16_t yTempValue = yTempLabel + 24;
  const uint16_t yHumidityValue = yHumidityLabel + 24;

  display.clearBuffer();
  display.setTextColor(EPD_BLACK);
  
  // // draws battery in the lower right corner. -3 in first parameter accounts for battery nub
  // screenHelperBatteryStatus((display.width()-xMargins-batteryBarWidth-3),(display.height()-yBottomMargin-batteryBarHeight),batteryBarWidth,batteryBarHeight);

  // label
  display.setFont(&FreeSans12pt7b);
  display.setCursor(xMargins, yscreenLabel);
  display.print("Air Quality");

  // co2 main line
  display.setFont(&FreeSans18pt7b);
  display.setCursor(xMidMargin, yCO2Label);
  display.print("CO");
  display.setFont(&FreeSans9pt7b);
  display.setCursor(xMidMargin+35,(yCO2Label+10));
  display.print("2");
  // co2 value
  display.setCursor(xMidMargin,yCO2Value);
  display.print(co2Labels[co2Range(sensorData.ambientCO2)]);
  display.setFont();
  display.setCursor((xMargins+80),(yCO2Value+7));
  display.print("(" + String(sensorData.ambientCO2) + ")");

  // indoor tempF
  // label
  display.setFont(&FreeSans18pt7b);
  display.setCursor(xMidMargin,yTempLabel);
  display.print("Temp");
  // value
  display.setFont(&FreeSans24pt7b);
  display.setCursor(xMidMargin,yTempValue);
  display.print(String((int)(sensorData.ambientTemperatureF + .5)));
  display.setFont(&meteocons24pt7b);
  display.print("+");

  // indoor humidity
  //label
  display.setFont(&FreeSans18pt7b);
  display.setCursor(xMidMargin,yHumidityLabel);
  display.print("Temp");
  //value
  display.setFont(&FreeSans24pt7b);
  display.setCursor(xMidMargin, yHumidityValue);
  display.print(String((int)(sensorData.ambientHumidity + 0.5)));
  // original icon ratio was 5:7?
  display.drawBitmap(xMidMargin+60,yHumidityValue-21,epd_bitmap_humidity_icon_sm4,20,28,EPD_BLACK);

  display.display();
  debugMessage("screenSensors end",1);
}

void screenHelperBatteryStatus(uint16_t initialX, uint16_t initialY, uint8_t barWidth, uint8_t barHeight)
// helper function for screenXXX() routines that draws battery charge %
// Description: helper function for screenXXX() routines, displaying battery charge % in a battery icon
// Parameters:
// Output: NA (void)
// IMPROVEMENT : Screen dimension boundary checks for passed parameters
// IMPROVEMENT : Check for offscreen drawing based on passed parameters
// IMPROVEMENT: batteryNub isn't placed properly when barHeight is changed from (default) 10 pixels
{ 
  if (batteryRead(batteryReadsPerSample))
  {
    // battery nub; width = 3pix, height = 60% of barHeight
    display.fillRect((initialX+barWidth), (initialY+(int(barHeight/5))), 3, (int(barHeight*3/5)), EPD_BLACK);
    // battery border
    display.drawRect(initialX, initialY, barWidth, barHeight, EPD_BLACK);
    // battery percentage as rectangle fill, 1 pixel inset from the battery border
    display.fillRect((initialX + 2), (initialY + 2), int(0.5+(hardwareData.batteryPercent*((barWidth-4)/100.0))), (barHeight - 4), EPD_BLACK);
    // DEBUG USE ONLY, display voltage level in the icon, requires changing bar height to 14 pixels
    // display.setFont(); // switch to generic small font
    // display.setCursor(initialX +2 ,initialY + 2);
    // display.print(hardwareData.batteryVoltage);
    debugMessage(String("screenHelperBatteryStatus displayed ") + hardwareData.batteryPercent + "% -> " + int(0.5+(hardwareData.batteryPercent*((barWidth-4)/100.0))) + " of " + (barWidth-4) + " pixels",1);
  }
}

void screenHelperQRCode(uint16_t initialX, uint16_t initialY, String url)
{
  debugMessage ("screenHelperQRCode begin",1);
  QRCode qrcode;
  
  uint8_t qrcodeData[qrcode_getBufferSize(qrCodeVersion)];
  qrcode_initText(&qrcode, qrcodeData, qrCodeVersion, ECC_LOW, url.c_str());

  for (uint8_t y = 0; y < qrcode.size; y++) 
  {
    for (uint8_t x = 0; x < qrcode.size; x++) 
    {
      //If pixel is on, we draw a ps x ps black square
      if(qrcode_getModule(&qrcode, x, y))
      {
        for(uint16_t xi = x*qrCodeScaling; xi < x*qrCodeScaling + qrCodeScaling; xi++)
        {
          for(uint16_t yi= y*qrCodeScaling; yi < y*qrCodeScaling + qrCodeScaling; yi++)
        // for(uint16_t xi = x*qrCodeScaling + 2; xi < x*qrCodeScaling + qrCodeScaling + 2; xi++)
        // {
        //   for(uint16_t yi= y*qrCodeScaling + 2; yi < y*qrCodeScaling + qrCodeScaling + 2; yi++)
          {
            display.writePixel(xi+initialX, yi+initialY, EPD_BLACK);
          }
        }
      }
    }
  }
  debugMessage("screenHelperQRCode end",1);
}

void screenHelperSparkLine(uint16_t initialX, uint16_t initialY, uint16_t xWidth, uint16_t yHeight) {
  debugMessage("screenHelperSparkLine start",1);
  // // TEST ONLY: load test CO2 values
  // // testSparkLineValues(sensorSampleSize);

  // uint16_t co2Min = co2Samples[0];
  // uint16_t co2Max = co2Samples[0];
  // // # of pixels between each samples x and y coordinates
  // uint8_t xPixelStep, yPixelStep;

  // uint16_t sparkLineX[sensorSampleSize], sparkLineY[sensorSampleSize];

  // // horizontal distance (pixels) between each displayed co2 value
  // xPixelStep = (xWidth / (sensorSampleSize - 1));

  // // determine min/max of CO2 samples
  // // could use recursive function but sensorSampleSize should always be relatively small
  // for (uint8_t loop = 0; loop < sensorSampleSize; loop++) {
  //   if (co2Samples[loop] > co2Max) co2Max = co2Samples[loop];
  //   if (co2Samples[loop] < co2Min) co2Min = co2Samples[loop];
  // }
  // debugMessage(String("Max CO2 in stored sample range is ") + co2Max + ", min is " + co2Min, 2);

  // // vertical distance (pixels) between each displayed co2 value
  // yPixelStep = round(((co2Max - co2Min) / yHeight) + .5);

  // debugMessage(String("xPixelStep is ") + xPixelStep + ", yPixelStep is " + yPixelStep, 2);

  // TEST ONLY : sparkline border box
  display.drawRect(initialX,initialY, xWidth,yHeight, EPD_BLACK);

  // // determine sparkline x,y values
  // for (uint8_t loop = 0; loop < sensorSampleSize; loop++) {
  //   sparkLineX[loop] = (initialX + (loop * xPixelStep));
  //   sparkLineY[loop] = ((initialY + yHeight) - (uint8_t)((co2Samples[loop] - co2Min) / yPixelStep));
  //   // draw/extend sparkline after first value is generated
  //   if (loop != 0)
  //     display.drawLine(sparkLineX[loop - 1], sparkLineY[loop - 1], sparkLineX[loop], sparkLineY[loop], GxEPD_BLACK);
  // }
  // for (uint8_t loop = 0; loop < sensorSampleSize; loop++) {
  //   debugMessage(String("X,Y coordinates for CO2 sample ") + loop + " is " + sparkLineX[loop] + "," + sparkLineY[loop], 2);
  // }
  debugMessage("screenHelperSparkLine end", 1);
}

bool batteryRead(uint8_t reads)
// Description: sets global battery values from i2c battery monitor or analog pin value (on supported boards)
// Parameters: integer that sets how many times the analog pin is sampled
// Output: NA (globals)
// Improvement: MAX17084 support
{
  #ifdef HARDWARE_SIMULATE
    batterySimulate();
    return true;
  #else
    // is LC709203F on i2c available?
    if (lc.begin())
    {
      debugMessage("batteryRead using LC709203F monitor",2);
      lc.setPackAPA(BATTERY_APA);
      lc.setThermistorB(3950);

      hardwareData.batteryPercent = lc.cellPercent();
      hardwareData.batteryVoltage = lc.cellVoltage();
      hardwareData.batteryTemperatureF = 32 + (1.8* lc.getCellTemperature());
    }
    else 
    {
      // read gpio pin on supported boards for battery level
      // modified from the Adafruit power management guide
      debugMessage("batteryRead using GPIO pin",2);
      float accumulatedVoltage = 0.0f;
      for (uint8_t loop = 0; loop < reads; loop++)
      {
        accumulatedVoltage += analogReadMilliVolts(BATTERY_VOLTAGE_PIN);
        debugMessage(String("Battery read ") + loop + " is " + analogReadMilliVolts(BATTERY_VOLTAGE_PIN) + "mV",2);
      }
      hardwareData.batteryVoltage = accumulatedVoltage/reads; // we now have the average reading
      // convert into volts  
      hardwareData.batteryVoltage *= 2;    // we divided by 2, so multiply back
      hardwareData.batteryVoltage /= 1000; // convert to volts!
      hardwareData.batteryPercent = batteryGetChargeLevel(hardwareData.batteryVoltage);
    }
    if (hardwareData.batteryVoltage) {
      debugMessage(String("Battery voltage: ") + hardwareData.batteryVoltage + "v, percent: " + hardwareData.batteryPercent + "%", 1);
      return true;
    }
    else
      return false;
  #endif
}

uint8_t batteryGetChargeLevel(float volts)
// Description: sets global battery values from i2c battery monitor or analog pin value (on supported boards)
// Parameters: battery voltage as float
// Output: battery percentage as int (0-100)
// Improvement: NA
{
  uint8_t idx = 50;
  uint8_t prev = 0;
  uint8_t half = 0;
  if (volts >= 4.2)
    return 100;
  if (volts <= 3.2)
    return 0;
  while (true) {
    half = abs(idx - prev) / 2;
    prev = idx;
    if (volts >= batteryVoltageTable[idx]) {
      idx = idx + half;
    } else {
      idx = idx - half;
    }
    if (prev == idx) {
      break;
    }
  }
  debugMessage(String("batteryGetChargeLevel returning ") + idx + " percent", 1);
  return idx;
}

bool sensorCO2Init()
// initializes CO2 sensor to read
{
  #ifdef HARDWARE_SIMULATE
    return true;
 #else
    char errorMessage[256];
    uint16_t error;

    Wire.begin();
    co2Sensor.begin(Wire);

    // Question : needed for MagTag version, but not ESP32V2?!
    co2Sensor.wakeUp();

    // stop potentially previously started measurement.
    error = co2Sensor.stopPeriodicMeasurement();
    if (error) {
      errorToString(error, errorMessage, 256);
      debugMessage(String(errorMessage) + " executing SCD40 stopPeriodicMeasurement()",1);
      return false;
    }

    // Check onboard configuration settings while not in active measurement mode
    float offset;
    error = co2Sensor.getTemperatureOffset(offset);
    if (error == 0){
        error = co2Sensor.setTemperatureOffset(sensorTempCOffset);
        if (error == 0)
          debugMessage(String("Initial SCD40 temperature offset ") + offset + " ,set to " + sensorTempCOffset,2);
    }

    uint16_t sensor_altitude;
    error = co2Sensor.getSensorAltitude(sensor_altitude);
    if (error == 0){
      error = co2Sensor.setSensorAltitude(SITE_ALTITUDE);  // optimizes CO2 reading
      if (error == 0)
        debugMessage(String("Initial SCD40 altitude ") + sensor_altitude + " meters, set to " + SITE_ALTITUDE,2);
    }

    // Start Measurement.  For high power mode, with a fixed update interval of 5 seconds
    // (the typical usage mode), use startPeriodicMeasurement().  For low power mode, with
    // a longer fixed sample interval of 30 seconds, use startLowPowerPeriodicMeasurement()
    // uint16_t error = co2Sensor.startPeriodicMeasurement();
    error = co2Sensor.startLowPowerPeriodicMeasurement();
    if (error) {
      errorToString(error, errorMessage, 256);
      debugMessage(String(errorMessage) + " executing SCD40 startLowPowerPeriodicMeasurement()",1);
      return false;
    }
    else
    {
      debugMessage("SCD40 starting low power periodic measurements",1);
      return true;
    }
  #endif
}

bool sensorCO2Read()
// Description: Sets global environment values from SCD40 sensor
// Parameters: none
// Output : true if successful read, false if not
// Improvement : This routine needs to return FALSE after XX read fails
{
  #ifdef HARDWARE_SIMULATE
    sensorCO2Simulate();
    debugMessage(String("SIMULATED SCD40: ") + sensorData.ambientTemperatureF + "F, " + sensorData.ambientHumidity + "%, " + sensorData.ambientCO2 + " ppm",1);
  #else
    char errorMessage[256];
    bool status;
    uint16_t co2 = 0;
    float temperature = 0.0f;
    float humidity = 0.0f;
    uint16_t error;

    debugMessage("CO2 sensor read initiated",1);
    status = false;
    while(!status) {
      // Is data ready to be read?
      bool isDataReady = false;
      error = co2Sensor.getDataReadyFlag(isDataReady);
      if (error) {
          errorToString(error, errorMessage, 256);
          debugMessage(String("Error trying to execute getDataReadyFlag(): ") + errorMessage,1);
          continue; // Back to the top of the loop
      }
      //debugMessage("CO2 sensor data available",2);
      // wonder if a small delay here would remove the prevalance of error messages from the next if block

      error = co2Sensor.readMeasurement(co2, temperature, humidity);
      if (error) {
          errorToString(error, errorMessage, 256);
          debugMessage(String("SCD40 executing readMeasurement(): ") + errorMessage,2);
          // Implicitly continues back to the top of the loop
      }
      else if (co2 < sensorCO2Min || co2 > sensorCO2Max)
      {
        debugMessage(String("SCD40 CO2 reading: ") + sensorData.ambientCO2 + " is out of expected range",1);
        //(sensorData.ambientCO2 < sensorCO2Min) ? sensorData.ambientCO2 = sensorCO2Min : sensorData.ambientCO2 = sensorCO2Max;
        // Implicitly continues back to the top of the loop
      }
      else
      {
        // valid measurement available, update globals
        sensorData.ambientTemperatureF = (temperature*1.8)+32.0;
        sensorData.ambientHumidity = humidity;
        sensorData.ambientCO2 = co2;
        debugMessage(String("SCD40: ") + sensorData.ambientTemperatureF + "F, " + sensorData.ambientHumidity + "%, " + sensorData.ambientCO2 + " ppm",1);
        status = true;
        break;
      }
    delay(100); // why is this needed, sensor issue?
    }
  #endif
  return(status);
}

#ifdef HARDWARE_SIMULATE
  void sensorCO2Simulate()
  // Description: Simulate temperature, humidity, and CO2 data from SCD40 sensor
  // Parameters: NA (globals)
  // Output : NA
  // Improvement: implement stable, rapid rise and fall 
  {
    sensorData.ambientTemperatureF = (random(sensorTempMinF,sensorTempMaxF) / 100.0);
    sensorData.ambientHumidity = random(sensorHumidityMin,sensorHumidityMax) / 100.0;
    sensorData.ambientCO2 = random(sensorCO2Min, sensorCO2Max);
    debugMessage(String("SIMULATED SCD40: ") + sensorData.ambientTemperatureF + "F, " + sensorData.ambientHumidity + "%, " + sensorData.ambientCO2 + " ppm",1);
  }

  void batterySimulate()
  // Simulate battery data
  // IMPROVEMENT: Simulate battery below SCD40 required level
  {
    hardwareData.batteryVoltage = random(batterySimVoltageMin, batterySimVoltageMax) / 100.00;
    hardwareData.batteryPercent = batteryGetChargeLevel(hardwareData.batteryVoltage);
    debugMessage(String("SIMULATED Battery voltage: ") + hardwareData.batteryVoltage + "v, percent: " + hardwareData.batteryPercent + "%", 1);  
  }
#endif

void powerNeoPixelEnable()
// enables MagTag neopixels
{
  // enable board Neopixel
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, LOW); // on

  // enable MagTag neopixels
  neopixels.begin();
  neopixels.setBrightness(neoPixelBrightness);
  neopixels.show(); // Initialize all pixels to off
  debugMessage("power on: neopixels",1);
}

void neoPixelCO2()
{
  // hard coded for MagTag
  debugMessage("neoPixelCO2 begin",1);
  neopixels.clear();
  neopixels.show();
  for (uint8_t i=0;i<neoPixelCount;i++)
  {
      if (sensorData.ambientCO2 < co2Warning)
        neopixels.setPixelColor(i,0,255,0);
      else if (sensorData.ambientCO2 > co2Alarm)
        neopixels.setPixelColor(i,255,0,0);
      else
        neopixels.setPixelColor(i,255,255,0);
      neopixels.show();
  }
  debugMessage("neoPixelCO2 end",1);
}

void powerDisable(uint16_t deepSleepTime)
// Powers down hardware then deep sleep MCU
{
  char errorMessage[256];

  debugMessage("powerDisable start",1);

  // power down epd
  display.powerDown();
  digitalWrite(EPD_RESET, LOW);  // hardware power down mode
  debugMessage("power off: epd",1);

  // power down SCD40
  // stops potentially started measurement then powers down SCD40
  uint16_t error = co2Sensor.stopPeriodicMeasurement();
  if (error) {
    errorToString(error, errorMessage, 256);
    debugMessage(String(errorMessage) + " executing SCD40 stopPeriodicMeasurement()",1);
  }
  co2Sensor.powerDown();
  debugMessage("power off: SCD40",1);

  //power down neopixels
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, HIGH); // off
  debugMessage("power off: neopixels",1);

  // Using external trigger ext1 to support multiple button interupt
  // esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);

  // Using external trigger ext0 to support one button interupt
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_14,0);  //1 = High, 0 = Low

  // ESP32 timer based deep sleep
  esp_sleep_enable_timer_wakeup(deepSleepTime*1000000); // ESP microsecond modifier
  debugMessage(String("powerDisable complete: ESP32 deep sleep for ") + (deepSleepTime) + " seconds",1);
  esp_deep_sleep_start();
}

uint8_t co2Range(uint16_t value)
// places CO2 value into a three band range for labeling and coloring. See config.h for more information
{
  if (value < co2Warning)
    return 0;
  else if (value < co2Alarm)
    return 1;
  else
    return 2;
}

void debugMessage(String messageText, uint8_t messageLevel)
// wraps Serial.println as #define conditional
{
  #ifdef DEBUG
    if (messageLevel <= DEBUG)
    {
      Serial.println(messageText);
      Serial.flush();  // Make sure the message gets output (before any sleeping...)
    }
  #endif
}