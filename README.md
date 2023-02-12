# energydisplay

![Finished display](https://i.imgur.com/8EXR7QX.png)

This is an energy status display that uses a 400x300 e-ink screen by Waveshare and a standard ESP32 with integrated WiFi.
The data is fetched from a Raspberry Pi or Beaglebone or any http web server for that matter. Getting the data into the relevant files is done elsewhere, this is just a display. There's one file for the core values, and then two text files for the histogram data for PV energy production and domestic power usage.

To extend the lifetime of the e-ink display, the image is refreshed only once every 5 minutes. Your SSID, wifi password and URL to fetch the files has to be filled into the relevant variables in the source.

The following libraries are needed to compile:
- The Display Library GxEPD. It can be found at https://github.com/ZinggJM/GxEPD. This implementation uses V3.1.
- As an dependcy the Adafruit GFX Library must also be installed. It can be found at https://github.com/adafruit/Adafruit-GFX-Library.

The used pins are 3, 5, 16, 17, 18, 19 and 23 as per the sourcecode, but this can be changed as needed. Wiring is up to personal preference. 

A color LCD version with animations for use with the M5Stack line of modules can be found on https://github.com/dividebysandwich/energydisplay-m5 as well.

`/energydisplay/state`
|line|name   |value        |unit|description                          |
|----|-------|-------------|----|-------------------------------------|
|1   |battery|0-100        | %  |battery state in percent             |
|2   |pv     |0.0-99.9     | kW |current PV power                     |
|3   |use    |0.0-99.9     | kW |current usage                        |
|4   |grid   |-99.9-99.9   | kW |current power from or to grid        |
|5   |battuse|-99.9-99.9   | kW |current power from or to the battery |
|6   |curtime|00:00-23:59  |    |current time (formatted)             | 
|7   |curdate|02.11.2023   |    |current date (formatted)             |

`/energydisplay/lastpv`
|line|name   |value        |unit|description                          |
|----|-------|-------------|----|-------------------------------------|
|1   |current|0-65535      | W  |pv power output                      |
|... |       |             |    |                                     |
|130 |latest |0-65535      | W  |pv power output                      |

`/energydisplay/lastuse`
|line|name   |value        |unit|description                          |
|----|-------|-------------|----|-------------------------------------|
|1   |current|0-65535      | W  |power usage                          |
|... |       |             |    |                                     |
|130 |latest |0-65535      | W  |power usage                          |