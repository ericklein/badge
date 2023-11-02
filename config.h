/*
  Project Name:   badge
  Description:    Magtag as conference badge

  See README.md for target information and revision history
*/

// Configuration Step 1: Set debug message output
// comment out to turn off; 1 = summary, 2 = verbose
#define DEBUG 1

// Step 2: Set battery size if applicable
// based on a settings curve in the LC709203F datasheet
// #define BATTERY_APA 0x08 // 100mAH
// #define BATTERY_APA 0x0B // 200mAH
#define BATTERY_APA 0x10 // 500mAH
// #define BATTERY_APA 0x19 // 1000mAH
// #define BATTERY_APA 0x1D // 1200mAH
// #define BATTERY_APA 0x2D // 2000mAH
// #define BATTERY_APA 0x32 // 2500mAH
// #define BATTERY_APA 0x36 // 3000mAH

const int neoPixelCount = 4;
const int neoPixelBrightness = 5;

const String nameFirst = "Eric";
const String nameLast = "Klein";
const String nameEmail = "eric@lemnos.vc";

#define SITE_ALTITUDE	90 // calibrates SCD40, Mercer Island, WA, in meters above sea level

// Sleep time if hardware error occurs in seconds
#define HARDWARE_ERROR_INTERVAL 10

// Configuration variables that are less likely to require changes

// Pin config for e-paper display

// Adafruit MagTag
	// #define EPD_CS      8 	// ECS, value comes from board definition package
	// #define EPD_DC      7   // D/C, value comes from board definition package
	#define SRAM_CS     -1  // SRCS, can set to -1 to not use a pin (uses ~10KB RAM)
	// #define EPD_RESET   6   // RST, value comes from board definition package
	#define EPD_BUSY    5   // BUSY, can set to -1 to not use a pin (will wait a fixed delay)

// CO2 configuration
const String co2Labels[5]={"Good", "OK", "So-So", "Poor", "Bad"};
// environment sensor sample timing
#ifdef DEBUG
	// number of times SCD40 is read, last read is the sample value
	#define READS_PER_SAMPLE	1
	// # of uint_16 CO2 samples saved to nvStorage, so limit this
  #define SAMPLE_SIZE				2
#else
	#define READS_PER_SAMPLE	5
  #define SAMPLE_SIZE 			10
#endif