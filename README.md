CloudLogger by Doug Sisco

This is the firmware for the ESP32 SiscoCircuits CloudLogger module.

See tc_common.h for the version number.

TODO
  BUG: The SonicWall drops http packets:
       Ethernet Header
       Ether Type: IP(0x800), Src=[30:ae:a4:0c:ea:c0], Dst=[18:b1:69:ef:b2:a8]
       IP Packet Header
       IP Type: TCP(0x6), Src=[192.168.1.87], Dst=[192.185.123.20]
       TCP Packet Header
       TCP Flags = [ACK,PSH,], Src=[64060], Dst=[80], Checksum=0x920f
       Application Header
       HTTP
       Value:[0]
       DROPPED, Drop Code: 105(Enforced Content Filter Policy), Module Id: 65(CFS), (Ref.Id: _7155_uyHtJcpfngKrRmv) 1:2)

  Http return code should be examined and the LED turned RED if bad.
   LED should be yellow when first powered on and no internet connection.
   
   Current values: include time of this value since, if it is old it will NOT show in the chart.
		Also may be a warning that the value is not current.
   Startup: currently the web server is blocked until the date is set.
       The date should be set to 1970 (or an old date) and then updated when the time svr is contacted.
   Need unit testing.
   Check that all config settings are within range: I found that one of the axis settings was out of range but was
      saved as part of the config.
   Select temperatures to include in title (tab).
   Setting to enable/disable slope.
   Setting to select channel for title.
   Zoom should be function of browser, not the same on all browsers.
   Arrows in tab and in status pane to show temperature trends.
   In main.htm, update values that are invalid with blanks.
		WHAT IS CHARTSAMPLES??
   Add channel names.
		Setting to enable channel on chart.
		If ajax does not respond within x seconds, show warning
   If last N readings were in error then report.
		Signal strength, battery level.
		Bluetooth interface (for IP address, etc.)
		Label each channel
		Assign each channel to a slot.
		Assign each channel color.
		Warning email when channel temp reaches a high or low value.

		Bluetooth interface (for IP address, etc.)

		Press a push button to enable web server for 10 minutes (make this an option).


		Show statistics: number of data points, current value.

		Use localized graph js.
  
		Download values as csv file.


		Add multiple temperature inputs.

DEBUGGER
   2-13-20 To use the debugger perform the following steps:
     Set the DEBUGGER switch to 1.
     Program using the serial port.
     If debugger still doesn't work, unplug and re-plug the Olimex device. Then disconnect and reconnect the Olimex device
        to the VM. Check that the color LED resistor is 1K or higher.
     If that doesn't work then throw the entire board off the deck and scream as loud as possible.
     Added C18b20::InvalidReadsTotal.

MAINTENANCE
   +-------- Major revisions - Major bug fixes or changes which affect functionality or interface
   | +------ Minor revisions - Minor bug fixes
   | | +---- Cosmetic or revisions - All other changes
   X.X.X 
   11-21-22 5.6.2 Fixed bug where 'Sw1Pressed' message was being sent every second from units that don't have Sw1 installed.
   
   11-20-22 5.6 Fixed bug where event log was not showing queued events. Changed CEventLog::Json() so that 
     only one event is returned at a time.
     Also fixed problem where copyright was not logging. Found that log string CANNOT contain '&' symbol.
     
   8-11-21 5.4.1 Fixed wifi config problem where punctuation in the post password was not being URL-decoded, 
         so passwords like "$JerRizzo" were being saved as "%40JerRizzo".

   7-24-21 5.2 Added Wifi pwd validity test.

   7-24-21 5.1 Changed wifi mechanism to account for missing SW1 pullup. 
         Without this change the captive portal will always be entered.

   7-18-21 5.0  Added user-initiated wifi configuration:
         Press SW1 at power-up and hold, when the LED begins fast-blinking red release SW1.
         LED will begin fast-blinking green.
         Use any wireless device to connect to the temporary wifi network "CLOUDLOGGER".
         Open a web page to aaaa.com.
         Within a minute the CloudLogger network page should appear.
         Select your wifi network, enter the wifi password and press SUBMIT.
         The CloudLogger sensor should reboot and connect to the wifi network.
         
     Also increased stack size for Thread_OncePerSecond due to stack overflow errors when logging many events.
     
   4-25-21 4.11 Added C18b20::ds18b20_init() so that the config register is set for 9-bit precision, though this may only be called on power-up.
     Also, I determined that the 18B20P (parasitic-only) device will not work above 120F. These devices are marked with a 'P'.
     
     There is definately a problem reading temperatures higher than 180F/82C over a 6ft cable, even with 5V-powered devices.
     Devices connected directly to the board work up to 220F/105C (lower than the datasheet value of 257F/125C).
     
        To investigate the problem I tried
           - confirming with an ammeter that the devices are drawing power from the 5V line (I measured 0.7mA at intervals of about 1hz).
           - slowing the conversion rate to 1/5 the normal rate.
           - inserting a 470 resistor in series with data line at both ends.
           - using only a three wire bus (instead of an ethernet cable with all 8 wires, 5 unused).
           - powering from 3.3V instead of 5V

   5-15-20 4.10.0 Same for SendHttpRequestErrorsCount. Also re-added log for socket errors. I really want to see these
     errors but I don't want the reboot unless they occur multiple times.
   5-14-20 4.9.0 Changed socket error reporting so that a number of consecutive errors must occur before the error
     is reported.
   4-28-20 4.8.0 Added error log string argument to CStatusLED::Set(), added log detail to all errors.
     Warning is no longer issued on startup, the led is just changed to yellow.
   4-27-20 4.6.2 Fixed led colors and web post blinking.
   4-25-20 4.5.0 Shortened delay between samples from 500 to 100.
      Changed led to 'warning' during firmware upgrade.
   4-23-20 4.3.4 Fixed problem where 18B20 errors were occurring frequently on some sensors. 
      Added portENTER_CRITICAL(&mutex) calls to several functions in the DS18B20 driver.
      Added PauseTemperatureThread flag to prevent problems with the firmware upgrade feature.
      NOTE that this definitely fixed the 18B20 errors as there were almost no errors on any of the five 
         test devices.

   4-22-20 4.2.1 Added logging of 18b20 sensor errors.

   4-22-20 4.1.2 Fixed reboot time: was 18000 sec now 600 sec (10 min).

   4-11-20 4.11 Error state changes are now logged. 
     Cancellation of reboot on system error is now logged.

   4-11-20 4.9 Added Identify feature. 

   4-11-20 4.8 Added remote reset capability.

   4-10-20 4.7 Added logging of channel ONLINE/OFFLINE status messages. 

   4-9-20 Changed the TC temperature read functions to use the built-in MAX31856 open-circuit detection.
     Also determined that the MAX31856 MUX design is sound and accurate for K-type thermocouples but 
     is not accurate above 1000F.

   4-8-20 Researched new driver for MAX31856 to fix the problem where tc readings are about 5F too high.
     After some testing I found that the problem is the thermocouple wire, not the MAX31856.
     Also, high temperature measurements are accurate.
     I also researched whether a new driver would allow for faster temperature readings. I (re-)discovered that 
         the MAX31856 datasheet indicates that measurements take a minimum of 160ms.

   3-29-20 4.0 Added Over-The-Air (OTA) program updates (see Design Notes, below).
     Added EventLog (see Design Notes, below). 

   2-13-20 Confirmed that the time is synchronized every hour. I did this by setting SW1 to change the time and then waiting 
     for the time to be set correctly.
     
   Also found a fix to the problem that starting a serial connection using Putty causes a reboot: pins 1 or 2 of the serial
      connector (PREN or D0) are enabled when Putty is started.  I made a connector that breaks these connections, and this fixes
      the problem but of course prevents programming through the serial port.

   2-13-20 Pushbutton switches: attempted to create an interrupt for the switches but after many hours
     concluded that the gpio interrupts are buggy in this ESP-IDF release. One guy on a forum said he finally
     gave up on getting interrupts to work. I found that the interrupts work but that they trigger intermittently
     regardless of the mode (neg or pos edge). I finally decided to simply poll the switches at 10hz.

   2-11-20 2.1.1 Updated to ESP/IDF release v3.3 (was 3.2.2).

   2-10-20 2.1 System reboots if a WIFI error persists for 10 minutes.

   11-15-19 To enhance detection of good thermocouple temperature readings I added TC_LOW_MAX test in Thread_ReadTemperature().
   Changed constant values of min and max TC values to TC_HIGH_MAX and TC_LOW_MAX.
 
   11-13-19 In Thread_SendDataToSvr() removed LoopCount and added uptime.
   Configured new 'effort_pi' network.
   Changed WEB_URL to 'post_data.php'.

   10-20-19 We now check for the svr response: server errors are now detected and the LED is changed accordingly.
   Note that if the SQL database cannot be contacted at all then an error is only reported in OpenSocket() at a timeout
      which takes about 30 seconds.
   Also note that this functionality cannot be tested by suspending the svr account because suspension does not disable the SQL server.

   10-5-19 Removed code to read and write Config to flash. Eventually we may want to save the webserver address to flash.
   Removed hostname from config.

   9-5-19 Added MCP2317 with I2C interface.
   Changed TC MUX address pins to MCP2317 outputs.
   Added channel LEDs to MCP2317 output.
   Changed tri-color LED pins to debugger pins.
   Changed triac output to MCP2317 output.

   8-30-19 Changed MAX31856 SPI to use alternate pins so that it doesn't interfere with debugger connection.

   8-29-19 Thermocouple open-sensor errors are now detected and reported as INVALID_TEMPERATURE.
   Confirmed that MAX31856 library uses the standard spi_master.c SPI library.
   
   Found that MAX31856 takes 143ms to take a measurement, and thus the 250ms delay in max31856_oneshot_temperature() is warranted.
   
   Found that unconnected TC channels were "bleeding" to the next adjacent channel because the 74HC4051 was holding the charge. 
      To fix this I added a 1M resistor across the MAX31856 sensor input. It does not seem to interfere with readings but it fixes the 
      "bleeding" problem. 
      We may also need protection devices from each input to ground to prevent static damage.
      
   Tested TC inputs for accuracy at 32F and range from -28F to 1000F.

	  8-25-19 In appmain(), changed the priority of the Thread_ReadTemperature thread to maximum. This significantly reduces the number of DS18B20 read errors
		from 6% to .5%. 
	  Changed Thread_ReadTemperature() so that when a sensor read error occurs, the last good value is stored in the data point. In the last version
		a null value was stored.
	  Confirmed the timing of DS18B20 reads in ds18b20.cpp.
	  Added CChannel::Connected (at least one valid read has occurred).

	  8-11-19 Attempted setting the current time from the GET response but there doesn't seem to be
	     an easy way to do this. It appears that the time is always synced to the NTP using an open socket,
	     though I could not confirm this.

   6-19-19 Fixed memory leak in server/common.js/UpdateData(). 

   6-18-19 Found that if a channel is not updating we will still see a current value but
      the chart line will not appear. This is because the chart only shows current values.

   6-17-19 Moved project to local drive (j:) because phpStorm doees not play well with mapped drives: some of the files simply 
      would not sync from phpStorm to the NAS folder.

   6-16-19 Completed printing current values on charts using additional SQL query.
   Fixed problem where chart updates would halt. This was due to occasional errors in the JSON returned 
      to the JS script: these errors were not handled so the script would simply quit. I added an error
      handler which simply calls another

	  6-15-19 ESP-IDF V3.3 
	     Disabled html server.
	     Bug: crash after about 5 hours:
      -assertion "Operating mode must not be set while SNTP client is running" failed: file "C:/SysGCC/esp32/esp-idf/v3.3/components/lwip/lwip/src/apps/sntp/sntp.c", line 600, function: sntp_setoperatingmode abort() was called at PC 0x400d4243 on core 0
      This was due to the thread Thread_UpdateTime() which was created on 5-3-19.
      Removed all ESP-IDF informational statements by phy_printf(const char* format, ...) with an empty function in components/esp32/lib_printf.c to solve your problem.

   5-3-19 Time is updated once per hour. 

   2-15-19 Added Sensor Type menu to settings panel.
			Added slope calculation.

   2-13-19 Added TC selector, removed thermocouple_task() and moved it to Thread_ReadTemperature().

   2.10 Fixed thermocouple sign problem in max31856.c/thermocouple_read_temperature().

   2.9 Added Config::CheckValues().
       Increased accuracy of 18b20 sensors in ds18b20_get_temp().

   2.8 Initial update does not print invalid values on left.
			 Bigger buttons.
       After MAX_SUCCESSIVE_INVALID_READS successive temp reads the value is recorded as INVALID_TEMP (instead of being ignored).
       Chart now reflects the same colors as the left panel. 
          In ProcessChartRequest() added 'null' entries for invalid values instead of simply skipping the values in the chart data.
   2.7 Changed X-axis date format.


DESIGN NOTES

NETWORK DISCOVERY WAP
  IP is set in tcpip_adapter_lwip.c/tcpip_adapter_init().
  
  See https://www.freertos.org/FreeRTOS_Support_Forum_Archive/December_2017/freertos_Receive_muliple_packages_ff35b821j.html
  
OTA (Over The Air Updates) 
  OTA firmware URL path is FIRMWARE_UPGRADE_URL.
  Firmware thermologger.bin file is created in C:\Projects\ThermoLogger\VisualGDB\Debug
  
  To update the online firmware run desktop .bat file Update 'Firmware.bat'.
     copy /y "C:\Projects\ThermoLogger\VisualGDB\Debug\thermologger.bin" "\\homedgs2\j$\vmware\TC Logger Svr\tl.bin"

     3-29-20 OTA uses the simple non-SSL example in C:\SysGCC\esp32\esp-idf\v3.3\examples\system\ota\simple_ota_example.
         This code had a few problems and I had to modifiy some library code to make it work properly.
         FIRMWARE_UPGRADE_PORT is defined in C:\SysGCC\esp32\esp-idf\v3.3\components\esp_http_client\esp_http_client.c
         I had to hard code this because it overrides the URL port nor the client port.
         See also the change to C:\SysGCC\esp32\esp-idf\v3.3\components\esp_https_ota\src\esp_https_ota.c to prevent an infinite loop in the case of a bad URL.
         
         OTA has it's own task loop which checks the flag FirmwareUpgradeInProcess once per second. It retreives the bin file, flashes it
         and then reboots.
         OTA is initiated by setting the SQL table var deviceflags bit 1. This flag is read with every data POST and is then reset during the next POST.

     To use the Olimex debugger the tricolor led 100 ohm resistor must be removed. 
     ALSO, you must first set DEBUGGER compiler directive in tc_common.h and flash using the serial port.
     You might be able to flash using the debugger but it's easier to use the serial port first.

     The MCP23S17 driver code is from  https://esp32.com/viewtopic.php?t=9309

		The MAX31856 driver code is from https://github.com/djkilgore/max31856.

Unusable: 6,7,8,9,10,11 (connected internally)
Inputs-only: 34,35,36,39
  12 is also unusable as an input on startup. * 

TOOLCHAIN NOTES

To send messages to error.txt, use make flash monitor &> error.txt

3-28-20 For OTA, in VisualGDB Project Properties, ESP-IDF Configuration, Partition Settings section choose "custom partition table csv".

BUGS
	  8-10-19 I had the same problem as 3-28-19, this time I removed the USB power supply from the wall and 
	     re-plugged and this immediately fixed the problem.
   3-28-19 Wlan suddenly would not connect until I removed the 'lasertag' wless entry, then it connected imediately.


 ***************** ESP PIN ASSIGNMENTS *****************

D0      MAX31856 SDI also serial port
D1      Serial port TXD
D2      Unused
D3      Serial port RXD
D4      MAX31856 SDO
D5      SENSOR 3
D6-11   unusable (connected internally)
D12     JTAG, LED BLUE
D13     JTAG, LED GREEN
D14     JTAG, LED RED
D15     JTAG
D16     MAX31856 SCK
D17     unused
D18     SENSOR 10
D19     SENSOR 4
D20 no pin
D21     SENSOR 11
D22     SENSOR 12
D23     unused
D24 no pin
D25     SENSOR 1
D26     SENSOR 2
D27     MAX31856 CS
D28 no pin
D32     MCP2317 SCK
D33     MCP2317 SDA
D34     unused         INPUT-ONLY
D35     unused         INPUT-ONLY
D36     PB2            INPUT-ONLY
D37, D38 no pin
D39     PB1            INPUT-ONLY
     
See https://randomnerdtutorials.com/esp32-pinout-reference-gpios/

