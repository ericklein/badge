/*
  Project Name:   badge
  Description:    conference badge displaying interesting information

  See README.md for target information and revision history
*/

// Configuration Step 1: Set debug message output
// comment out to turn off; 1 = summary, 2 = verbose

// #define DEBUG 1

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
const int qrCodeScaling = 3; 	// QRCode square size = Round ((smallest screen dimension)-((xmargin)*(ymargin)/33[LOW_ECC])
const int qrCodeVersion = 4;
const String qrCodeURL 	= "https://www.linkedin.com/in/ericklein";

// Configuration Step 5: Set site altitude in meters for SCD40 calibration
#define SITE_ALTITUDE	90 // calibrates SCD40, Mercer Island, WA, in meters above sea level
// #define SITE_ALTITUDE 236 // Pasadena, CA (SuperCon!)

// Configuration variables that are less likely to require changes

// e-paper

// Adafruit MagTag Pin config
// #define EPD_CS      8 	// ECS, value comes from board definition package
// #define EPD_DC      7   // D/C, value comes from board definition package
#define SRAM_CS     -1  // SRCS, can set to -1 to not use a pin (uses ~10KB RAM)
// #define EPD_RESET   6   // RST, value comes from board definition package
#define EPD_BUSY    5   // BUSY, can set to -1 to not use a pin (will wait a fixed delay)

// Allow for adjustable screen as needed for physical packaging. 
// 0 orients horizontally with neopixels on top
// 1 orients vertically with flex cable as top
const uint8_t displayRotation = 1; // rotation 1 = 0,0 is away from flex cable, right aligned with flex cable label

// MagTag neopixel configuration
// const int neoPixelCount = 4;
// const int neoPixelBrightness = 5;

// Button handling for ESP32 deep sleep
// MagTag BUTTON_A = 15, BUTTON_B = 14, BUTTON_C = 12, BUTTON_D = 11 (all RTC pins)

// ext1 wakeup (multiple buttons via bitmask)
//#define BUTTON_PIN_BITMASK 0xD800 // All buttons; 2^15+2^14+2^12+2^11 in hex
// #define BUTTON_PIN_BITMASK 0xC000 // Buttons A,B; 2^15+2^14 in hex
// #define BUTTON_PIN_BITMASK 0x9800 // Buttons A,C,D; 2^15+2^12+2^11 in hex
// #define BUTTON_PIN_BITMASK 0x8800 // Buttons A,D; 2^15+2^11 in hex
// #define BUTTON_PIN_BITMASK 0x1800 // Buttons C,D; 2^12+2^11 in hex
// #define BUTTON_PIN_BITMASK 0x8000 // Button A; 2^15 in hex

// CO2 configuration
const String co2Labels[5]={"Good", "OK", "So-So", "Poor", "Bad"};
// environment sensor sample timing
#ifdef DEBUG
	// number of times SCD40 is read, last read is the sample value
	const uint8_t sensorReadsPerSample =	1;
	// # of uint_16 CO2 samples saved to nvStorage, so limit this
  const uint8_t sensorSampleSize = 2;
	const uint16_t hardwareSleepTime = 15;
#else
	const uint8_t sensorReadsPerSample =	5;
  const uint8_t sensorSampleSize = 10;
	const uint16_t hardwareSleepTime = 60;
#endif

// Sleep time if hardware error occurs in seconds
const uint16_t hardwareErrorSleepTime = 15;