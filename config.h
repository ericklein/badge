/*
  Project Name:   badge
  Description:    conference badge displaying interesting information

  See README.md for target information and revision history
*/

#include <FastLED.h> // needed for CRGB definition

// Configuration Step 1: Set debug message output
// comment out to turn off; 1 = summary, 2 = verbose
#define DEBUG 2

// Configuration Step 2: simulate WiFi and sensor hardware,
// returning random but plausible values
// comment out to turn off
// #define HARDWARE_SIMULATE

// Configuration Step 3: Set battery size if applicable
// based on a settings curve in the LC709203F datasheet

// #define BATTERY_APA 0x08 // 100mAH
// #define BATTERY_APA 0x0B // 200mAH
#define BATTERY_APA 0x10 // 500mAH
// #define BATTERY_APA 0x19 // 1000mAH
// #define BATTERY_APA 0x1D // 1200mAH
// #define BATTERY_APA 0x2D // 2000mAH
// #define BATTERY_APA 0x32 // 2500mAH
// #define BATTERY_APA 0x36 // 3000mAH

// Configuration Step 4: Set personal information for screen display
const String badgeNameFirst	=   "Eric";
const String badgeNameLast =    "Klein";
const String badgeEmail	=       "eric@lemnos.vc";
const String badgeFirstThing =  "Hockey";
const String badgeSecondThing = "Gaming";
const String badgeThirdThing =  "Making";

// Configuration Step 5: QR Code information for screen display
const uint8_t qrCodeScaling = 3; 	// QRCode square size = Round ((smallest screen dimension)-((xmargin)*(ymargin)/33[LOW_ECC])
const uint8_t qrCodeVersion = 4;
const String qrCodeURL 	= "https://www.linkedin.com/in/ericklein";

// Configuration Step 6: Set site altitude in meters for SCD40 calibration
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
const uint8_t screenCount = 3;
const uint32_t screenSwapIntervalMS = 30000;

// screen layout assists
const uint8_t xMargins = 5;
const uint8_t yTopMargin = 5;
const uint8_t yBottomMargin = 5;
const uint8_t batteryBarWidth = 28;
const uint8_t batteryBarHeight = 10;
// const uint8_t batteryBarHeight = 14; // larger to display debug voltage text

// MagTag neopixels
const uint8_t neoPixelCount = 4;
const uint8_t neoPixelBrightness = 25;

// Battery 
// analog pin used to reading battery voltage
// #define BATTERY_VOLTAGE_PIN A13 // Adafruit Feather ESP32V2
// #define BATTERY_VOLTAGE_PIN A7 // Adafruit Feather M0 Express, pin 9
#define BATTERY_VOLTAGE_PIN 04 // Adafruit MagTag

// number of analog pin reads sampled to average battery voltage
const uint8_t   batteryReadsPerSample = 5;

// battery charge level lookup table
const float batteryVoltageTable[101] = {
  3.200,  3.250,  3.300,  3.350,  3.400,  3.450,
  3.500,  3.550,  3.600,  3.650,  3.700,  3.703,
  3.706,  3.710,  3.713,  3.716,  3.719,  3.723,
  3.726,  3.729,  3.732,  3.735,  3.739,  3.742,
  3.745,  3.748,  3.752,  3.755,  3.758,  3.761,
  3.765,  3.768,  3.771,  3.774,  3.777,  3.781,
  3.784,  3.787,  3.790,  3.794,  3.797,  3.800,
  3.805,  3.811,  3.816,  3.821,  3.826,  3.832,
  3.837,  3.842,  3.847,  3.853,  3.858,  3.863,
  3.868,  3.874,  3.879,  3.884,  3.889,  3.895,
  3.900,  3.906,  3.911,  3.917,  3.922,  3.928,
  3.933,  3.939,  3.944,  3.950,  3.956,  3.961,
  3.967,  3.972,  3.978,  3.983,  3.989,  3.994,
  4.000,  4.008,  4.015,  4.023,  4.031,  4.038,
  4.046,  4.054,  4.062,  4.069,  4.077,  4.085,
  4.092,  4.100,  4.111,  4.122,  4.133,  4.144,
  4.156,  4.167,  4.178,  4.189,  4.200 };

// Buttons
const uint8_t buttonD1Pin = 14;
const uint16_t buttonDebounceDelayMS = 50;
#define WAKE_FROM_SLEEP_PIN GPIO_NUM_14

// Simulation boundary values
#ifdef HARDWARE_SIMULATE
  const uint16_t sensorTempMinF =       1400; // divided by 100.0 to give floats
  const uint16_t sensorTempMaxF =       14000;
  const uint16_t sensorHumidityMin =    0; // RH%, divided by 100.0 to give float
  const uint16_t sensorHumidityMax =    10000;
#endif

// CO2 sensor
#ifdef DEBUG
	// time between samples
  const uint32_t sensorSampleIntervalMS = 30000;
#else
  const uint32_t sensorSampleIntervalMS = 180000;
#endif

const uint16_t co2Fair =  800;
const uint16_t co2Poor =  1200;
const uint16_t co2Bad =   1600;
const String warningLabel[4]={"Good", "Fair", "Poor", "Bad"};
const CRGB warningColor[4]={CRGB::Green, CRGB::Yellow, CRGB::OrangeRed, CRGB::Red};
const uint16_t sensorCO2Min = 400;
const uint16_t sensorCO2Max = 2000;
const uint8_t co2SensorReadFailureLimit = 20;

// Sleep timers
const uint32_t hardwareErrorSleepTimeμS = 10000000;  // sleep time if hardware error occurs
const uint32_t sleepIntervalMS = screenCount * 30000;
#ifdef DEBUG
  const uint32_t sleepTimeμS = 30000000;
#else
  const uint32_t sleepTimeμS = 300000000;
#endif  