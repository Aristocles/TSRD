# TSRD
Temperature Sensor Relay Device (TSRD)

/*
 * www.makeitbreakitfixit.com - February 2017 - v1.1
 * Eddys Temp Sensor Relay Device thingy for John
 * 
 * This device was custom-made for Johnny.
 * Johnny bought a NAS/server recently which he wants to put inside a
 * small cavity in his wardrobe. The problem is the space is so small
 * that it will heat up very quickly in there. He has a small exhaust
 * fan which runs on mains power (240v AC) which he will use to keep
 * it cool in there, but he doesn't want the fan to stay on 24/7.
 * So this Temp Sensor Relay Device (TSRD) has one cable which plugs in to mains
 * power and another cable which plugs in to the exhaust fan.
 * The TSRD sits in between this power circuit.
 * 
 * The TSRD is made up of an ESP8266 controller, temperature sensor, and relays
 * and controls when the exhaust fan should be powered.
 * The TSRD will monitor the temperature until a pre-configured threshold is
 * met. Once met, it will trigger the relay to allow 240v AC power through
 * to the fan for a pre-configured amount of time. Once the time elapses if
 * the temperature is still above the threshold it will turn the fan on again
 * for the same amount of time. This cycle will continue until the temperature
 * drops back below the threshold. Once below the threshold, it will wait until
 * the temperature rises above the threshold to turn back on again.
 * 
 * When first powered on the TSRD will ask for the temperature threshold and for
 * the duration of time to run the exhaust fan. This information is not saved if
 * the device is restarted. If no information is entered within a few minutes then
 * the device will load default configuration which you can set below.
 * 
 * TODO: Maybe next time use EEPROM to store the settings.
 */
