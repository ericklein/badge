# badge
 
### Purpose
Badge is a fun way to display pertinent information to someone at an event. It is designed to replace the borrow paper "about" stickers or badges you get at events and spark a conversation.
### Features
![Screenshot](readme/main_screen.jpg)
- #1 - Badge wearer's name (set in config.h)
- #2 - QR Code to a URL (set in config.h)
- #3 - US standard CO2 label and value
- #4 - Battery level
- #5 - Wakes the ESP32 from deep sleep and refreshes the screen
### Target configuration
- See config.h for parameter configuration
### Bill of Materials (BOM)
- MCU
	- ESP32
- WiFi
	- comment #define RJ45, uncomment #define WIFI in config.h
	- Supported hardware
		- ESP32 based boards
- Environment Sensors
	- Supported hardware
		- [SCD40 temp/humidity/CO2 sensor](https://www.adafruit.com/product/5187)
	- Technical references 
		- https://cdn-learn.adafruit.com/assets/assets/000/104/015/original/Sensirion_CO2_Sensors_SCD4x_Datasheet.pdf?1629489682
		- https://github.com/Sensirion/arduino-i2c-scd4x
		- https://github.com/sparkfun/SparkFun_SCD4x_Arduino_Library
		- https://emariete.com/en/sensor-co2-sensirion-scd40-scd41-2/
- Battery (monitor)
	- define battery size in Step 2 of config.h
	- Supported hardware
		- 	[LC709203F battery voltage monitor](https://www.adafruit.com/product/4712)
		- [Adafruit batteries](https://www.adafruit.com/product/2011)
- Screen
	- Supported hardware
		- 296x128 e-paper display
			- [Adafruit MagTag (EPD)](https://www.adafruit.com/product/4800)
	- Technical References
		- https://cdn-learn.adafruit.com/downloads/pdf/adafruit-gfx-graphics-library.pdf

### Issues and Feature Requests
- See GitHub Issues for project

### .plan (big ticket items)
- gxEPD2 support
- GPIO (button) ESP32 wakeup support to have multiple screens of information
	- code is largely included to support this, need to change hardware