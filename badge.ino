/*
  Project Name:   badge
  Description:    Magtag as conference badge

  See README.md for target information and revision history
*/

// hardware and internet configuration parameters
#include "config.h"

// environment sensor data
typedef struct
{
  float ambientTempF;
  float ambientHumidity;
  uint16_t ambientCO2[SAMPLE_SIZE];
} envData;
envData sensorData;

int sampleCounter;

// hardware status data
typedef struct
{
  float batteryPercent;
  float batteryVoltage;
} hdweData;
hdweData hardwareData;

// Magtag neopixels
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel neopixels = Adafruit_NeoPixel(4, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// initialize scd40 environment sensor
#include <SensirionI2CScd4x.h>
SensirionI2CScd4x envSensor;

// Battery voltage sensor
#include <Adafruit_LC709203F.h>
Adafruit_LC709203F lc;

// eink support
#include <Adafruit_ThinkInk.h>

#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans24pt7b.h>

// #include "Fonts/FreeSerifItalic24pt7b.h"
// #include "Fonts/FreeSerifItalic48pt7b.h"
// #include "Fonts/FreeSerifItalic12pt7b.h"

#include "Fonts/FreeSerif24pt7b.h"
#include "Fonts/FreeSerif48pt7b.h"
#include "Fonts/FreeSerif12pt7b.h"

#include "Fonts/meteocons12pt7b.h"

// Special glyphs for screenCO2
#include "Fonts/glyphs.h"

// 2.96" greyscale display with 196x128 pixels
// colors are EPD_WHITE, EPD_BLACK, EPD_RED, EPD_GRAY, EPD_LIGHT, EPD_DARK
ThinkInk_290_Grayscale4_T5 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

// screen layout assists
const int xMargins = 5;
const int xMidMargin = ((display.width()/2) + xMargins);
const int yMargins = 2;
// yCO2 not used
const int yCO2 = 20;
const int ySparkline = 40;
const int yTemp = 100;
// BUG, 7/8 = 112, WiFi status is 15 (5*3) pixels high
const int sparklineHeight = 40;
const int batteryBarWidth = 28;
const int batteryBarHeight = 10;

void setup()
{
  hardwareData.batteryVoltage = 0;  // 0 = no battery attached
  sampleCounter = 0;
  for (int i=0; i<SAMPLE_SIZE;i++)
  {
    sensorData.ambientCO2[i] = 400;
  }

  #ifdef DEBUG
    Serial.begin(115200);
    // wait for serial port connection
    while (!Serial)
      delay(10);
    debugMessage("Badge started");
  #endif

  // // Neopixel power
  // pinMode(NEOPIXEL_POWER, OUTPUT);
  // digitalWrite(NEOPIXEL_POWER, LOW); // on

  // // prep MagTag neopixels
  // neopixels.begin();
  // neopixels.setBrightness(50);
  // neopixels.show(); // Initialize all pixels to off

  debugMessage("neopixels ready");

  // prep MagTag buttons
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  pinMode(BUTTON_D, INPUT_PULLUP);
  debugMessage("buttons ready");

  // there is no way to query screen for status
  //display.begin(THINKINK_GRAYSCALE4);
  display.begin(THINKINK_MONO);
  debugMessage("Display ready");

  // Initialize environmental sensor
  if (!sensorInit()) {
    debugMessage("Environment sensor failed to initialize");
    screenAlert("NO SCD40");
    // This error often occurs right after a firmware flash and reset.
    // Hardware deep sleep typically resolves it, so quickly cycle the hardware
    powerDisable(HARDWARE_ERROR_INTERVAL);
  }
}

void loop()
{
  if (! digitalRead(BUTTON_A)) 
    {
      debugMessage("Button A pressed");
      // neopixels.clear();
      // neopixels.show();
      // neopixels.setPixelColor(3,255,0,0);
      // neopixels.show();
      screenName("Eric","Klein","eric@lemnos.vc");
    }
    else if (! digitalRead(BUTTON_B)) {
      debugMessage("Button B pressed");
      // neopixels.clear();
      // neopixels.show();
      // neopixels.setPixelColor(2,0,255,0);
      // neopixels.show();
      batteryReadVoltage();
      if (!sensorRead()) {
        debugMessage("SCD40 returned no/bad data");
        screenAlert("SCD40 no/bad data");
        powerDisable(HARDWARE_ERROR_INTERVAL);
      }
      screenCO2();
      if (sampleCounter<SAMPLE_SIZE)
        sampleCounter++;
      else
        sampleCounter = 0;
    }
    else if (! digitalRead(BUTTON_C)) {
      debugMessage("Button C pressed");
      // neopixels.clear();
      // neopixels.show();
      // neopixels.setPixelColor(1,0,0,255);
      // neopixels.show();
      screenAlert("QR Code");
    }
    else if (! digitalRead(BUTTON_D)) {
      debugMessage("Button D pressed");
      // neopixels.clear();
      // neopixels.show();
      // neopixels.setPixelColor(0,255,255,255);
      // neopixels.show();
      screenThreeThings();
    }  
}

void debugMessage(String messageText)
// wraps Serial.println as #define conditional
{
#ifdef DEBUG
  Serial.println(messageText);
  Serial.flush();  // Make sure the message gets output (before any sleeping...)
#endif
}

void screenAlert(String messageText)
// Display critical error message on screen
{
  display.clearBuffer();
  display.setTextColor(EPD_BLACK);
  display.setFont(&FreeSans12pt7b);
  display.setCursor(40,(display.height()/2+6));
  display.print(messageText);

  //update display
  display.display();
}

void screenName(String firstName, String lastName, String email)
// Display badge owner's name
{
  display.clearBuffer();
  display.setTextColor(EPD_BLACK);
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
  debugMessage("screenName update completed");
}

void screenCO2()
// Display current ambient temp, humidity, and CO2 level
{
  display.clearBuffer();
  display.setTextColor(EPD_BLACK);
  
  // screen helper routines
  // draws battery in the lower right corner. -3 in first parameter accounts for battery nub
  screenHelperBatteryStatus((display.width()-xMargins-batteryBarWidth-3),(display.height()-yMargins-batteryBarHeight),batteryBarWidth,batteryBarHeight);

  // display sparkline
  screenHelperSparkLines(xMargins,ySparkline,((display.width()/2) - (2 * xMargins)),sparklineHeight);

  // Indoor
  // CO2 level
  // calculate CO2 value range in 400ppm bands
  int co2range = ((sensorData.ambientCO2[sampleCounter] - 400) / 400);
  co2range = constrain(co2range,0,4); // filter CO2 levels above 2400

  // main line
  display.setFont(&FreeSans12pt7b);
  display.setCursor(xMargins, yCO2);
  display.print("CO");
  display.setCursor(xMargins+50,yCO2);
  display.print(": " + String(co2Labels[co2range]));
  display.setFont(&FreeSans9pt7b);
  display.setCursor(xMargins+35,(yCO2+10));
  display.print("2");
  // value line
  display.setFont();
  display.setCursor((xMargins+88),(yCO2+7));
  display.print("(" + String(sensorData.ambientCO2[sampleCounter]) + ")");

  // Indoor temp
  display.setFont(&FreeSans12pt7b);
  display.setCursor(xMargins,yTemp);
  display.print(String((int)(sensorData.ambientTempF + .5)));
  display.setFont(&meteocons12pt7b);
  display.print("+");

  // Indoor humidity
  display.setFont(&FreeSans12pt7b);
  display.setCursor(xMargins+60, yTemp);
  display.print(String((int)(sensorData.ambientHumidity + 0.5)));
  // original icon ratio was 5:7?
  display.drawBitmap(xMargins+90,yTemp-21,epd_bitmap_humidity_icon_sm4,20,28,EPD_BLACK);

  display.display();
  debugMessage("screenCO2 update completed");
}

void screenQRCode()
// Display 
{
 
}

void screenThreeThings()
// Display three things about the badge owner
{
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
  debugMessage("screenName update completed");
}

void screenHelperBatteryStatus(int initialX, int initialY, int barWidth, int barHeight)
// helper function for screenXXX() routines that draws battery charge %
{
  // IMPROVEMENT : Screen dimension boundary checks for function parameters
  if (hardwareData.batteryVoltage>0) 
  {
    // battery nub; width = 3pix, height = 60% of barHeight
    display.fillRect((initialX+barWidth),(initialY+(int(barHeight/5))),3,(int(barHeight*3/5)),EPD_BLACK);
    // battery border
    display.drawRect(initialX,initialY,barWidth,barHeight,EPD_BLACK);
    //battery percentage as rectangle fill, 1 pixel inset from the battery border
    display.fillRect((initialX + 2),(initialY + 2),(int((hardwareData.batteryPercent/100)*barWidth) - 4),(barHeight - 4),EPD_GRAY);
    debugMessage(String("battery status drawn to screen as ") + hardwareData.batteryPercent + "%" );
  }
}

void screenHelperSparkLines(int initialX, int initialY, int xWidth, int yHeight)
{
  // TEST ONLY: load test CO2 values
  // testSparkLineValues(SAMPLE_SIZE);

  uint16_t co2Min = sensorData.ambientCO2[0];
  uint16_t co2Max = sensorData.ambientCO2[0];
  // # of pixels between each samples x and y coordinates
  int xPixelStep, yPixelStep;

  int sparkLineX[SAMPLE_SIZE], sparkLineY[SAMPLE_SIZE];

  // horizontal distance (pixels) between each displayed co2 value
  xPixelStep = (xWidth / (SAMPLE_SIZE - 1));

  // determine min/max of CO2 samples
  // could use recursive function but SAMPLE_SIZE should always be relatively small
  for(int i=0;i<SAMPLE_SIZE;i++)
  {
    if(sensorData.ambientCO2[i] > co2Max) co2Max = sensorData.ambientCO2[i];
    if(sensorData.ambientCO2[i] < co2Min) co2Min = sensorData.ambientCO2[i];
  }
  debugMessage(String("Max CO2 in stored sample range is ") + co2Max +", min is " + co2Min);

  // vertical distance (pixels) between each displayed co2 value
  yPixelStep = round(((co2Max - co2Min) / yHeight)+.5);

  debugMessage(String("xPixelStep is ") + xPixelStep + ", yPixelStep is " + yPixelStep);

  // TEST ONLY : sparkline border box
  // display.drawRect(initialX,initialY, xWidth,yHeight, EPD_BLACK);

  // determine sparkline x,y values
  for(int i=0;i<SAMPLE_SIZE;i++)
  {
    sparkLineX[i] = (initialX + (i * xPixelStep));
    sparkLineY[i] = ((initialY + yHeight) - (int)((sensorData.ambientCO2[i]-co2Min) / yPixelStep));

    // draw/extend sparkline after first value is generated
    if (i != 0)
      display.drawLine(sparkLineX[i-1],sparkLineY[i-1],sparkLineX[i],sparkLineY[i],EPD_BLACK);  
  }
  for (int i=0;i<SAMPLE_SIZE;i++)
  {
    debugMessage(String("X,Y coordinates for CO2 sample ") + i + " is " + sparkLineX[i] + "," + sparkLineY[i]);
  }
    debugMessage("sparkline drawn to screen");
}

void batteryReadVoltage() 
{
  // check to see if i2C monitor is available
  if (lc.begin())
  // Check battery monitoring status
  {
    lc.setPackAPA(BATTERY_APA);
    hardwareData.batteryPercent = lc.cellPercent();
    hardwareData.batteryVoltage = lc.cellVoltage();
    debugMessage(String("Battery voltage: ") + hardwareData.batteryVoltage + "v, percent: " + hardwareData.batteryPercent + "%");
  }
}

bool sensorInit() {
  char errorMessage[256];

  #if defined(ARDUINO_ADAFRUIT_QTPY_ESP32S2) || defined(ARDUINO_ADAFRUIT_QTPY_ESP32S3_NOPSRAM) || defined(ARDUINO_ADAFRUIT_QTPY_ESP32S3) || defined(ARDUINO_ADAFRUIT_QTPY_ESP32_PICO)
    // these boards have two I2C ports so we have to initialize the appropriate port
    Wire1.begin();
    envSensor.begin(Wire1);
  #else
    // only one I2C port
    Wire.begin();
    envSensor.begin(Wire);
  #endif

  envSensor.wakeUp();
  envSensor.setSensorAltitude(SITE_ALTITUDE);  // optimizes CO2 reading

  uint16_t error = envSensor.startPeriodicMeasurement();
  if (error) {
    // Failed to initialize SCD40
    errorToString(error, errorMessage, 256);
    debugMessage(String(errorMessage) + " executing SCD40 startPeriodicMeasurement()");
    return false;
  } 
  else
  {
    debugMessage("SCD40 initialized");
    return true;
  }
}

bool sensorRead()
// reads SCD40 READS_PER_SAMPLE times then stores last read
{
  char errorMessage[256];

  screenAlert("CO2 check");
  for (int loop=1; loop<=READS_PER_SAMPLE; loop++)
  {
    // SCD40 datasheet suggests 5 second delay between SCD40 reads
    delay(5000);
    uint16_t error = envSensor.readMeasurement(sensorData.ambientCO2[sampleCounter], sensorData.ambientTempF, sensorData.ambientHumidity);
    // handle SCD40 errors
    if (error) {
      errorToString(error, errorMessage, 256);
      debugMessage(String(errorMessage) + " error during SCD4X read");
      return false;
    }
    if (sensorData.ambientCO2[sampleCounter]<400 || sensorData.ambientCO2[sampleCounter]>6000)
    {
      debugMessage("SCD40 CO2 reading out of range");
      return false;
    }
    //convert C to F for temp
    sensorData.ambientTempF = (sensorData.ambientTempF * 1.8) + 32;
    debugMessage(String("SCD40 read ") + loop + " of " + READS_PER_SAMPLE + " : " + sensorData.ambientTempF + "F, " + sensorData.ambientHumidity + "%, " + sensorData.ambientCO2[sampleCounter] + " ppm");
  }
  return true;
}

void powerDisable(int deepSleepTime)
// Powers down hardware activated via powerEnable() then deep sleep MCU
{
  char errorMessage[256];

  debugMessage("Starting power down activities");
  // power down epd
  display.powerDown();
  digitalWrite(EPD_RESET, LOW);  // hardware power down mode
  debugMessage("powered down epd");

  // power down SCD40
  // stops potentially started measurement then powers down SCD40
  uint16_t error = envSensor.stopPeriodicMeasurement();
  if (error) {
    errorToString(error, errorMessage, 256);
    debugMessage(String(errorMessage) + " executing SCD40 stopPeriodicMeasurement()");
  }
  envSensor.powerDown();
  debugMessage("SCD40 powered down");

  esp_sleep_enable_timer_wakeup(deepSleepTime*1000000); // ESP microsecond modifier
  debugMessage(String("Going to sleep for ") + deepSleepTime + " seconds");
  esp_deep_sleep_start();
}