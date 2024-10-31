/*
  Project Name:   badge
  Description:    conference badge displaying interesting information

  See README.md for target information and revision history
*/

// hardware and internet configuration parameters
#include "config.h"

// Utility class for easy handling aggregate sensor data
#include "measure.h"

// QR code support
#include "qrcoderm.h"

// hardware support

// neopixels
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel neopixels = Adafruit_NeoPixel(neoPixelCount, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// scd40 environment sensor
#include <SensirionI2CScd4x.h>
SensirionI2CScd4x envSensor;

// battery voltage sensor
#include <Adafruit_LC709203F.h>
Adafruit_LC709203F lc;

// button support
#include <ezButton.h>
ezButton buttonOne(buttonD1Pin,INPUT_PULLDOWN);

// eink support
#include <Adafruit_ThinkInk.h>
// 2.96" greyscale display with 296x128 pixels
// colors are EPD_WHITE, EPD_BLACK, EPD_GRAY, EPD_LIGHT, EPD_DARK
ThinkInk_290_Grayscale4_T5 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

#include <Fonts/FreeSans9pt7b.h>    // screenCO2
#include <Fonts/FreeSans12pt7b.h>   // screenAlert, screenName, screenCO2
#include <Fonts/FreeSans18pt7b.h>   // screenThreeThings
#include <Fonts/FreeSans24pt7b.h>   // screenCO2

#include "Fonts/FreeSerif24pt7b.h"  // screenName
#include "Fonts/FreeSerif48pt7b.h"  // screenName
#include "Fonts/FreeSerif18pt7b.h"  // screenQRCodeCO2
#include "Fonts/FreeSerif12pt7b.h"  // screenQRCodeCO2

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
  //uint8_t rssi;
} hdweData;
hdweData hardwareData;

// Utility class used to streamline accumulating sensor values
Measure totalCO2, totalTemperatureF, totalHumidity;

long timeLastSample  = -(sensorSampleInterval*1000);  // set to trigger sample on first iteration of loop()
uint8_t screenCurrent = 0;

void setup()
{
  hardwareData.batteryVoltage = 0.0;  // 0 = no battery attached

  #ifdef DEBUG
    Serial.begin(115200);
    // wait for serial port connection
    while (!Serial)
    debugMessage(String("Starting badge with ") + sensorSampleInterval + " second sample interval",1);
  #endif

  powerNeoPixelEnable();

  // there is no way to query screen for status
  //display.begin(THINKINK_GRAYSCALE4);
  // debugMessage ("epd: power on in grayscale",1);
  display.begin(THINKINK_MONO);
  display.setRotation(displayRotation);
  display.setTextWrap(false);
  debugMessage(String("epd: power on in mono with rotation: ") + displayRotation,1);

  // Initialize environmental sensor
  if (!sensorCO2Init())
  {
    debugMessage("Environment sensor failed to initialize",1);
    screenAlert("CO2 sensor?");
    // This error often occurs right after a firmware flash and reset.
    // Hardware deep sleep typically resolves it, so quickly cycle the hardware
    powerDisable(hardwareErrorSleepTime);
  }

  buttonOne.setDebounceTime(buttonDebounceDelay); 
} 

void loop()
{
  // update current timer value
  unsigned long timeCurrent = millis();

  buttonOne.loop();
  // check if buttons were pressed
  if (buttonOne.isReleased())
  {
    ((screenCurrent + 1) >= screenCount) ? screenCurrent = 0 : screenCurrent ++;
    debugMessage(String("button 1 press, switch to screen ") + screenCurrent,1);
    screenUpdate(true);
  }

  // is it time to read the sensor?
  if((timeCurrent - timeLastSample) >= (sensorSampleInterval * 1000)) // converting sensorSampleInterval into milliseconds
  {
    if (!sensorCO2Read())
    {
      screenAlert("CO2 read fail");
    }
    else
    {
      // update neopixels
      neoPixelCO2();

      // Received new data so update aggregate information
      totalCO2.include(sensorData.ambientCO2);
      totalTemperatureF.include(sensorData.ambientTemperatureF);
      totalHumidity.include(sensorData.ambientHumidity);// refresh current screen based on new sensor reading
      // Update the TFT display with new readings on the current screen (hence false)
      screenUpdate(true);
    }
    // Save current timestamp to restart sample delay interval
    timeLastSample = timeCurrent;
  }
}

void screenUpdate(bool firstTime) 
{
  switch(screenCurrent) {
    case 0:
      screenQRCodeCO2(nameFirst, nameLast, qrCodeURL);
      break;
    case 1:
      screenThreeThings();
      break;
    // case 2:
    //   screenAggregateData();
    //   break;
    // case 3:
    //   screenColor();
    //   break;
    // case 4:
    //   screenGraph();
    //   break;
    default:
      // This shouldn't happen, but if it does...
      screenQRCodeCO2(nameFirst, nameLast, qrCodeURL);
      debugMessage("bad screen ID",1);
      break;
  }
}

void screenAlert(String messageText)
// Display error message centered on screen
{
  debugMessage("screenAlert start",1);

  int16_t x1, y1;
  uint16_t width,height;

  display.clearBuffer();
  display.setTextColor(EPD_BLACK);
  display.setFont(&FreeSans12pt7b);
  display.getTextBounds(messageText.c_str(), 0, 0, &x1, &y1, &width, &height);

  if (width >= display.width())
  {
    debugMessage(String("ERROR: screenAlert '") + messageText + "' is " + abs(display.width()-width) + " pixels too long", 1);
  }
  display.setCursor(display.width()/2-width/2,display.height()/2+height/2);
  display.print(messageText);
  //update display
  display.display();
  debugMessage("screenAlert end",1);
}

void screenName(String firstName, String lastName, String email, bool invert)
// Display badge owner's name
{
  // ADD string validation
  debugMessage ("screenName start",1);

  display.clearBuffer();
  display.setTextColor(EPD_BLACK);
  if (invert)
  {
    display.fillRect(0,0,display.width(),display.height(),EPD_BLACK);
    display.setTextColor(EPD_WHITE);
  }
  display.setFont(&FreeSerif48pt7b);
  display.setCursor(xMargins,display.height()*5/8);
  display.print(firstName);
  display.setFont(&FreeSerif24pt7b);
  display.print(" ");
  display.print(lastName);
  display.setCursor(xMargins,display.height()*7/8);
  display.setFont(&FreeSans12pt7b);
  display.print(email);
  display.display();
  debugMessage("screenName end",1);
}

// void screenCO2()
// // Display ambient temp, humidity, and CO2 level
// {
//   debugMessage("screenCO2 start",1);
//   display.clearBuffer();
//   display.setTextColor(EPD_BLACK);
  
//   // draws battery in the lower right corner. -3 in first parameter accounts for battery nub
//   screenHelperBatteryStatus((display.width()-xMargins-batteryBarWidth-3),(display.height()-yBottomMargin-batteryBarHeight),batteryBarWidth,batteryBarHeight);

//   // display sparkline
//   screenHelperSparkLines(xMargins,ySparkline,((display.width()/2) - (2 * xMargins)),sparklineHeight);

//   // CO2 level
//   // calculate CO2 value range in 400ppm bands
//   int co2range = ((sensorData.ambientCO2[sampleCounter] - 400) / 400);
//   co2range = constrain(co2range,0,4); // filter CO2 levels above 2400

//   // main line
//   display.setFont(&FreeSans12pt7b);
//   display.setCursor(xMargins, yCO2);
//   display.print("CO");
//   display.setCursor(xMargins+50,yCO2);
//   display.print(": " + String(co2Labels[co2range]));
//   display.setFont(&FreeSans9pt7b);
//   display.setCursor(xMargins+35,(yCO2+10));
//   display.print("2");
//   // value line
//   display.setFont();
//   display.setCursor((xMargins+88),(yCO2+7));
//   display.print("(" + String(sensorData.ambientCO2[sampleCounter]) + ")");

//   // indoor tempF
//   display.setFont(&FreeSans24pt7b);
//   display.setCursor(xMidMargin,yTempF);
//   display.print(String((int)(sensorData.ambientTemperatureF + .5)));
//   display.setFont(&meteocons24pt7b);
//   display.print("+");

//   // indoor humidity
//   display.setFont(&FreeSans24pt7b);
//   display.setCursor(xMidMargin, yHumidity);
//   display.print(String((int)(sensorData.ambientHumidity + 0.5)));
//   // original icon ratio was 5:7?
//   display.drawBitmap(xMidMargin+60,yHumidity-21,epd_bitmap_humidity_icon_sm4,20,28,EPD_BLACK);

//   display.display();
//   debugMessage("screenCO2 end",1);
// }

void screenQRCodeCO2(String firstName, String lastName, String url)
// Displays first name, QRCode, and CO2 value in vertical orientation
{
  debugMessage("screenQRCodeCO2 start",1);

  // screen layout assists
  const int xMidMargin = ((display.width()/2) + xMargins);
  const int yTempF = 36 + yTopMargin;
  const int yHumidity = 90;
  const int yCO2 = 40;

  display.clearBuffer();

  // draws battery in the lower right corner. -3 in first parameter accounts for battery nub
  screenHelperBatteryStatus((display.width()-xMargins-batteryBarWidth-3),(display.height()-yBottomMargin-batteryBarHeight),batteryBarWidth,batteryBarHeight);

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

  //main line
  display.setFont(&FreeSans12pt7b);
  display.setCursor(xMargins, 240);
  display.print("CO");
  display.setCursor(xMargins+50,240);
  display.print(" " + co2Labels[co2Range(sensorData.ambientCO2)]);
  display.setFont(&FreeSans9pt7b);
  display.setCursor(xMargins+35,(240+10));
  display.print("2");
  // value line
  display.setFont();
  display.setCursor((xMargins+80),(240+7));
  display.print("(" + String(sensorData.ambientCO2) + ")");

  // draw battery in the lower right corner. -3 in first parameter accounts for battery nub
  screenHelperBatteryStatus((display.width()-xMargins-batteryBarWidth-3),(display.height()-yBottomMargin-batteryBarHeight),batteryBarWidth,batteryBarHeight);
  display.display();
  debugMessage("screenQRCodeCO2 end",1);
}

// void screenQRCode(String firstName, String lastName, String url)
// // Display name and a QR code for more information
// {
//   debugMessage("screenQRCode start",1);
//   display.clearBuffer();
//   display.setTextColor(EPD_BLACK);
//   display.setFont(&FreeSerif48pt7b);
//   display.setCursor(xMargins,display.height()/2);
//   display.print(firstName);
//   display.setFont(&FreeSerif24pt7b);
//   display.setCursor(xMargins,display.height()*7/8);
//   display.print(lastName);
//   screenHelperQRCode(display.width()/2+30,yTopMargin+10,url);
//   display.display();
//   debugMessage("screenQRCode end",1);
// }

void screenThreeThings()
// Display three things about the badge owner
{
  debugMessage("screenThreeThings start",1);
  display.clearBuffer();
  display.setTextColor(EPD_BLACK);
  display.setFont(&FreeSans18pt7b);
  display.setCursor(xMargins,display.height()/4);
  display.print("Hockey");
  display.setCursor(xMargins,display.height()/2);
  display.print("Video games");
  display.setCursor(xMargins,display.height()*3/4);
  display.print("Making");

  display.display();
  debugMessage("screenThreeThings update completed",1);
}

void screenHelperBatteryStatus(uint16_t initialX, uint16_t initialY, uint8_t barWidth, uint8_t barHeight)
// helper function for screenXXX() routines that draws battery charge %
{
  // IMPROVEMENT : Screen dimension boundary checks for passed parameters
  // IMPROVEMENT : Check for offscreen drawing based on passed parameters
 
  if (batteryRead())
  {
    // battery nub; width = 3pix, height = 60% of barHeight
    display.fillRect((initialX+barWidth), (initialY+(int(barHeight/5))), 3, (int(barHeight*3/5)), EPD_BLACK);
    // battery border
    display.drawRect(initialX, initialY, barWidth, barHeight, EPD_BLACK);
    //battery percentage as rectangle fill, 1 pixel inset from the battery border
    display.fillRect((initialX + 2), (initialY + 2), int(0.5+(hardwareData.batteryPercent*((barWidth-4)/100.0))), (barHeight - 4), EPD_BLACK);
    debugMessage(String("Battery percent displayed is ") + hardwareData.batteryPercent + "%, " + int(0.5+(hardwareData.batteryPercent*((barWidth-4)/100.0))) + " of " + (barWidth-4) + " pixels",1);
  }
}

void screenHelperQRCode(int initialX, int initialY, String url)
{
  QRCode qrcode;
  
  uint8_t qrcodeData[qrcode_getBufferSize(qrCodeVersion)];
  qrcode_initText(&qrcode, qrcodeData, qrCodeVersion, ECC_LOW, url.c_str());

  debugMessage(String("QRCode size is: ")+ qrcode.size + " pixels",2);
  for (uint8_t y = 0; y < qrcode.size; y++) 
  {
    for (uint8_t x = 0; x < qrcode.size; x++) 
    {
      //If pixel is on, we draw a ps x ps black square
      if(qrcode_getModule(&qrcode, x, y))
      {
        for(int xi = x*qrCodeScaling; xi < x*qrCodeScaling + qrCodeScaling; xi++)
        {
          for(int yi= y*qrCodeScaling; yi < y*qrCodeScaling + qrCodeScaling; yi++)
        // for(int xi = x*qrCodeScaling + 2; xi < x*qrCodeScaling + qrCodeScaling + 2; xi++)
        // {
        //   for(int yi= y*qrCodeScaling + 2; yi < y*qrCodeScaling + qrCodeScaling + 2; yi++)
          {
            display.writePixel(xi+initialX, yi+initialY, EPD_BLACK);
          }
        }
      }
    }
  }
  debugMessage("QRCode drawn to screen",1);
}

bool batteryRead()
// stores battery information in global hardware data structure
{
  // check to see if i2C monitor is available
  if (lc.begin())
  // Check battery monitoring status
  {
    lc.setPackAPA(BATTERY_APA);
    hardwareData.batteryPercent = lc.cellPercent();
    hardwareData.batteryVoltage = lc.cellVoltage();
    debugMessage(String("Battery voltage: ") + hardwareData.batteryVoltage + "v, percent: " + hardwareData.batteryPercent + "%",1);
    return true;
  }
  else
  {
    debugMessage("Did not detect i2c battery monitor",1);
    return false;
  }
}

bool sensorCO2Init()
// initializes CO2 sensor to read
{
  #ifdef SENSOR_SIMULATE
    return true;
 #else
    char errorMessage[256];
    uint16_t error;

    Wire.begin();
    envSensor.begin(Wire);

    // stop potentially previously started measurement.
    error = envSensor.stopPeriodicMeasurement();
    if (error) {
      errorToString(error, errorMessage, 256);
      debugMessage(String(errorMessage) + " executing SCD40 stopPeriodicMeasurement()",1);
      return false;
    }

    // Check onboard configuration settings while not in active measurement mode
    float offset;
    error = envSensor.getTemperatureOffset(offset);
    if (error == 0){
        error = envSensor.setTemperatureOffset(sensorTempCOffset);
        if (error == 0)
          debugMessage(String("Initial SCD40 temperature offset ") + offset + " ,set to " + sensorTempCOffset,2);
    }

    uint16_t sensor_altitude;
    error = envSensor.getSensorAltitude(sensor_altitude);
    if (error == 0){
      error = envSensor.setSensorAltitude(SITE_ALTITUDE);  // optimizes CO2 reading
      if (error == 0)
        debugMessage(String("Initial SCD40 altitude ") + sensor_altitude + " meters, set to " + SITE_ALTITUDE,2);
    }

    // Start Measurement.  For high power mode, with a fixed update interval of 5 seconds
    // (the typical usage mode), use startPeriodicMeasurement().  For low power mode, with
    // a longer fixed sample interval of 30 seconds, use startLowPowerPeriodicMeasurement()
    // uint16_t error = envSensor.startPeriodicMeasurement();
    error = envSensor.startLowPowerPeriodicMeasurement();
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
// sets global environment values from SCD40 sensor
{
  #ifdef SENSOR_SIMULATE
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

    // Loop attempting to read Measurement
    status = false;
    while(!status) {
      delay(100);

      // Is data ready to be read?
      bool isDataReady = false;
      error = envSensor.getDataReadyFlag(isDataReady);
      if (error) {
          errorToString(error, errorMessage, 256);
          debugMessage(String("Error trying to execute getDataReadyFlag(): ") + errorMessage,1);
          continue; // Back to the top of the loop
      }
      if (!isDataReady) {
          continue; // Back to the top of the loop
      }
      debugMessage("CO2 sensor data available",2);

      error = envSensor.readMeasurement(co2, temperature, humidity);
      if (error) {
          errorToString(error, errorMessage, 256);
          debugMessage(String("SCD40 executing readMeasurement(): ") + errorMessage,1);
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
        // Successfully read valid data
        sensorData.ambientTemperatureF = (temperature*1.8)+32.0;
        sensorData.ambientHumidity = humidity;
        sensorData.ambientCO2 = co2;
        debugMessage(String("SCD40: ") + sensorData.ambientTemperatureF + "F, " + sensorData.ambientHumidity + "%, " + sensorData.ambientCO2 + " ppm",1);
        // Update global sensor readings
        status = true;  // We have data, can break out of loop
      }
    }
  #endif
  return(true);
}

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
  debugMessage("power: neopixels on",1);
}

void neoPixelCO2()
{
  // hard coded for MagTag
  debugMessage("neoPixelCO2 begin",1);
  neopixels.clear();
  neopixels.show();
  for (int i=0;i<neoPixelCount;i++)
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

void powerDisable(int deepSleepTime)
// Powers down hardware activated via w() then deep sleep MCU
{
  char errorMessage[256];

  debugMessage("powerDisable start",1);

  // power down epd
  display.powerDown();
  digitalWrite(EPD_RESET, LOW);  // hardware power down mode
  debugMessage("power off: epd",1);

  // power down SCD40
  // stops potentially started measurement then powers down SCD40
  uint16_t error = envSensor.stopPeriodicMeasurement();
  if (error) {
    errorToString(error, errorMessage, 256);
    debugMessage(String(errorMessage) + " executing SCD40 stopPeriodicMeasurement()",1);
  }
  envSensor.powerDown();
  debugMessage("power off: SCD40",1);

  //power down neopixels
  // pinMode(NEOPIXEL_POWER, OUTPUT);
  // digitalWrite(NEOPIXEL_POWER, HIGH); // off
  // debugMessage("power off: neopixels",1);

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

void debugMessage(String messageText, int messageLevel)
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