/*
  Project Name:   badge
  Description:    conference badge displaying interesting information

  See README.md for target information and revision history
*/

// Configuration Step 1: Set debug message output
// comment out to turn off; 1 = summary, 2 = verbose

#define DEBUG 2

// Configuration Step 2: Set battery size if applicable
// based on a settings curve in the LC709203F datasheet

// #define BATTERY_APA 0x08 // 100mAH
// #define BATTERY_APA 0x0B // 200mAH
#define BATTERY_APA 0x10 // 500mAH
// #define BATTERY_APA 0x19 // 1000mAH
// #define BATTERY_APA 0x1D // 1200mAH
// #define BATTERY_APA 0x2D // 2000mAH
// #define BATTERY_APA 0x32 // 2500mAH
// #define BATTERY_APA 0x36 // 3000mAH

// Configuration Step 3: Set personal information for screen display
const String nameFirst	= "Eric";
const String nameLast		= "Klein";
const String nameEmail	= "eric@lemnos.vc";

// Configuration Step 4: QR Code information for screen display
const uint8_t qrCodeScaling = 3; 	// QRCode square size = Round ((smallest screen dimension)-((xmargin)*(ymargin)/33[LOW_ECC])
const uint8_t qrCodeVersion = 4;
const String qrCodeURL 	= "https://www.linkedin.com/in/ericklein";

// Configuration Step 5: Set site altitude in meters for SCD40 calibration
#define SITE_ALTITUDE	90 // calibrates SCD40, Mercer Island, WA, in meters above sea level
// #define SITE_ALTITUDE 236 // Pasadena, CA (SuperCon!)

// Configuration variables that are less likely to require changes

// display
// Adafruit MagTag pin config
// #define EPD_CS      8 	// ECS, value comes from board definition package
// #define EPD_DC      7   // D/C, value comes from board definition package
#define SRAM_CS     -1  // SRCS, can set to -1 to not use a pin (uses ~10KB RAM)
// #define EPD_RESET   6   // RST, value comes from board definition package
#define EPD_BUSY    5   // BUSY, can set to -1 to not use a pin (will wait a fixed delay)

// Allow for adjustable screen as needed for physical packaging. 
// 0 orients horizontally with neopixels on top
// 1 orients vertically with flex cable as top
const uint8_t screenRotation = 1; // rotation 1 = 0,0 is away from flex cable, right aligned with flex cable label
const uint8_t screenCount = 4;
const uint8_t screenSwapInterval = 30; // in seconds

// screen layout assists
const uint8_t xMargins = 5;
const uint8_t yTopMargin = 5;
const uint8_t yBottomMargin = 2;
const uint8_t batteryBarWidth = 28;
const uint8_t batteryBarHeight = 10;

// MagTag neopixel configuration
const uint8_t neoPixelCount = 4;
const uint8_t neoPixelBrightness = 30;

// Buttons
const uint8_t buttonD1Pin = 11;
const uint16_t buttonDebounceDelay = 50; // time in milliseconds to debounce button

// CO2 sensor
#ifdef DEBUG
	// time between samples in seconds
  const uint16_t sensorSampleInterval = 30;
#else
  const uint16_t sensorSampleInterval = 180;
#endif

const uint16_t co2Warning = 800; // Associated with "OK"
const uint16_t co2Alarm = 1000; // Associated with "Poor"

const String co2Labels[3]={"Good", "So-So", "Poor"};

const uint16_t sensorCO2Min =      400;
const uint16_t sensorCO2Max =      2000;
const uint16_t sensorTempCOffset = 0; // in C

// Sleep timers
const uint8_t hardwareErrorSleepTime = 10; // in seconds
const uint16_t sleepInterval = 120; // in seconds
const uint16_t sleepTime = 300; // in seconds
