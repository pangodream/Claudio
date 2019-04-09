# Claudio
Claudio is a wake up alarm based on ESP32 by Espressif Systems and programmed with Arduino IDE

![Claudio Main Screen](https://raw.githubusercontent.com/pangodream/Claudio/master/hardware/CLAUDIO_MAIN_SCREEN.jpg)

Folders:

/          Arduino IDE code

/box       Prototype print 3d files

/hardware  Hardware pictures



More info and demos at: https://www.youtube.com/watch?v=NURag-YYxIY

# WIRING DIAGRAM
![Claudio Main Screen](https://raw.githubusercontent.com/pangodream/Claudio/master/hardware/Claudio_Wiring_Diagram.png)

# REMOTE CONFIGURATION
By default, Claudio is listening on port 1234 for incoming connections. 

These connections allow a remote system to configure or perform some actions on the system. (Change parameters, Set alarm time, Deactivate alarm, ...)

Once the remote client is connected to claudio, commands are sent concatenated to items and values all in upper case. 

For instance, if we want to know the number of beeps to sound during an alarm, we must use the command GET followed by the item ALBEEP:
```bash
GETALBEEP
```
 If all succeed, the system will reply
```bash
90
```
which is the value cofigured for this parameter. Otherwise, the system could reply "Invalid Command" or "Invalid Item"

If we want to change the value of this parameter, for instance to 120, we should send
```bash
SETALBEEP120
```
and the system should reply
```bash
OK
```


## Available commands 
**GET**
Retrieves the value of the specified item.

**SET**
Stores the specified value in the specified item.

**GET**
Retrieves all the items value in a concatenated string ITEM1 + VALUE1 + ; + ... + ITEMn + VALUEn + ; 

For instance:
```bash
BOOTWT5;TOUCHR120;THTCHR70;THTCHM65;THTCHL65;LTCHTH12;...
```

**RST**
Reboot the system.

**TSS**
Stops and deactivates alarm, even while alarm is being executed.

## Available items
**BOOTWT**

Number of seconds (aprox.) to wait after boot screen to start the normal operation

EEPROM stored: Yes

Default value: 20

**TOUCHR**

Number of readings to calculate sensor capacitance average value

EEPROM stored: Yes

Default value: 120

**THTCHR**

Average capacitance threshold value for Right sensor. Under this value the sensor is considered activated

EEPROM stored: Yes

Default value: 70

**THTCHM**

Average capacitance threshold value for Middle sensor. Under this value the sensor is considered activated

EEPROM stored: Yes

Default value: 65

**THTCHL**

Average capacitance threshold value for Left sensor. Under this value the sensor is considered activated

EEPROM stored: Yes

Default value: 65

**LTCHTH**

Long touch threshold. Number of tenths of a second to consider a sensor has been long touched.

EEPROM stored: Yes

Default value: 12   (1.2 secs)

**SNZMIN**

Number of minutes to postpone alarm time (Snooze)

EEPROM stored: Yes

Default value: 9

**ALBEEP**

Number of beeps to sound during alarm

EEPROM stored: Yes

Default value: 90

**MAXLUX**

Maximum lux value to take into account. Above this any value is just considered maximum value.

When ambient light reaches this value, the display backlight is set to 100% power. 

EEPROM stored: Yes

Default value: 20

**MINLUX**

Minimum lux value to take into account. Below this any value is just considered minimum value.

When ambient light goes down to this value, the display backlight is set to Minimun Brightness value. (see next item))

EEPROM stored: Yes

Default value: 0

**MINBRG**

When ambient light reaches its minimum value, display backlight power is set to this value.

This is commonly used to set the backlight level we want while sleeping. 

EEPROM stored: Yes

Default value: 10

**ALARHO**

Defines the hour part of alarm time 

EEPROM stored: No

Default value: No default value

**ALARMI**

Defines the minutes part of alarm time

EEPROM stored: No

Default value: No default value

**DOALAR**

Switch ON/OFF alarm by sending 1/0 value for this item

EEPROM stored: No

Default value: No default value

**ONALAR**

Indicates whether the alarm is beeping or not. As well, it stops alarm beeping if its value is set to 0

EEPROM stored: No

Default value: No default value


To test remote configuration capabilities you can use Claudio Remote, an Android app to change configuration values easily:
https://github.com/pangodream/ClaudioRemote