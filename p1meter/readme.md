## Usage

Reset WiFi: plug in with DIP 4 ON, then turn DIP off and watch for SSID "P1 Metertje" to configure.
(ip 192.168.4.1)

DIP3 sets needle to 0 for offset afjust.

## Working

1. Connects to wifi.
2. Waits for broadcast message of P1 Monitor (https://www.ztatz.nl/)
3. Use source IP of broadcast to do an api request.  
4. Show sum of export and import on needle.

## Libraries

- [WiFiManager 2.0.3-alpha](https://github.com/tzapu/WiFiManager)
	- By tzapu,tablatronix
	
- [ArduinoJSON 6.18.0](https://arduinojson.org/)
	- By Benoit Blanchon
	
## Board	
- [Arduino ESP32 boards](https://github.com/espressif/arduino-esp32)
		- repository: https://dl.espressif.com/dl/package_esp32_index.json
		
Selected board:  ESP32 Dev Module