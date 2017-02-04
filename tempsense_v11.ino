
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

#include <LiquidCrystal.h>
#include <math.h>

#define RELAY 16 // Relay pin
#define THERMISTOR 0 // Thermistor pin
#define BUTTON1 14 // Menu button
#define BUTTON2 12 // Menu button
#define BUTTON3 13 // Menu button
/*
LCD Connections:
rs (LCD pin 4) to Arduino pin 5 (ESP D1)
rw (LCD pin 5) to GND
enable (LCD pin 6) to Arduino pin 4 (ESP D2)
LCD pin 15 to 5v+ 
LCD pins d4, d5, d6, d7 to Arduino pins 0, 2, 14, 12
*/
LiquidCrystal lcd(5, 4, 0, 2, 3, 10);
int buttons[] = {BUTTON1, BUTTON2, BUTTON3}; // 3 buttons for input/setting menu 
int counter[] = {0, 0, 0};       // how many times we have seen new value
int reading[3];           // the current value read from the input pin
int current_state[] = {LOW, LOW, LOW};    // the debounced input value
bool configured = false; // If false, show the configuration menu to user (on boot)
// DEFAULT VALUES ARE SET BELOW
double thresholdTemp = 40.0; // Temperature at which the relay will switch on
int relayRun = 10; // Number of mins to run the fan once thresholdTemp is hit
// DEFAULT VALUES ARE SET ABOVE
unsigned long bootTime = millis(); // Time when first booted
int menuProgress = 0; // Used to keep track of progressing through initial cfg menu
                      // 0 = no cfg set, 1 = threshold temp set, 2 = duration set
unsigned long timeNow = millis(); // Used for timing the duration the relay is enabled
bool relayEnabled = false; // indicator that the relay is enabled (fan is running)
bool testing = false; // Set to TRUE to put the device in testing mode
           // where it will cycle through turning itself on and off for stress testing.

// the following variable is a long because the time, measured in milliseconds,
// will quickly become a bigger number than can be stored in an int.
long time = 0;         // the last time the output pin was sampled
int debounce_count = 10; // number of millis/samples to consider before declaring a debou
int timers[] = {0, 0, 0, 0};           // Used to introduce delay in various parts of the code


void setup(void) {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println();
  Serial.println("--- Eddys Temp Sensor Relay Device thingy for John ---");
  Serial.println("--- www.makeitbreakitfixit.com ---");
  Serial.println("February 2017 - (Hovik smells)");
  Serial.println();
  Serial.println();
  pinMode(RELAY, OUTPUT);
  //pinMode(BACKLIGHT, OUTPUT);
  //digitalWrite(BACKLIGHT, HIGH); // turn backlight on. Replace 'HIGH' with 'LOW' to turn it off.
  lcd.begin(20, 4);              // rows, columns.  use 16,2 for a 16x2 LCD, etc.
  lcd.clear();                   // start with a blank screen
  lcd.setCursor(0,0);            // set cursor to column 0, row 0
  pinMode(buttons[0], INPUT_PULLUP);
  pinMode(buttons[1], INPUT_PULLUP);
  pinMode(buttons[2], INPUT_PULLUP);
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, HIGH); // HIGH means relay is disabled. Weird, I know
  if (!testing) initialConfig(); // Only run if not in testing mode
}
/* 
    Arduino thermistor example software
    Tutorial:  http://www.hacktronics.com/Tutorials/arduino-thermistor-tutorial
    Copyright (c) 2010 Mark McComb, hacktronics LLC
    License: http://www.opensource.org/licenses/mit-license.php (Go crazy)
*/
double Thermister(int RawADC) {
  double temp = log(((10240000/RawADC) - 10000));
  // See http://en.wikipedia.org/wiki/Thermistor for explanation of formula
  temp = 1 / (0.001129148 + (0.000234125 * temp) + (0.0000000876741 * temp * temp * temp));
  temp = temp - 273.15;           // Convert Kelvin to Celcius
  return temp;
}

double getTemp(void) {
  double temp = Thermister(analogRead(THERMISTOR));  // Read temp sensor
  if ((timers[0] % 100) == 0) {
    lcd.setCursor(10,1);
    if (temp < 10)
      lcd.print("0"); // Pad with a zero if less than 10
    lcd.print(temp);
    lcd.print("C");
  }
  timers[0]++;
  return temp;
}

boolean checkButton(int button) {
  reading[button] = digitalRead(buttons[button]); // Take button value
  if((reading[button] == current_state[button]) && (counter[button] > 0))
    counter[button]--;
  if(reading[button] != current_state[button]) // Button state change detected
     counter[button]++; 
  // If the Input has shown the same value for long enough let's switch it
  if(counter[button] >= debounce_count && reading[button] == 0) { // Only register a button press when it is pressed, not released
    counter[button] = 0;
    current_state[button] = reading[button];
    Serial.print("Button "); Serial.print(button); Serial.println(" pressed");
    return true; // positive button press
  }
    return false;
}

void initialConfig() {
  lcd.setCursor(0,0);
  lcd.print("Set Temperature:");
  lcd.setCursor(0,1);
  lcd.print(thresholdTemp);
  lcd.setCursor(5,1);
  lcd.print("C");

  while (configured == false) {
    delay(5);
    if (millis() - bootTime > 120000) { // wait some time for configuration, if not buttons pressed then just roll with default cfg
      Serial.println("WARNING! Configuration menu timeout. Using default values.");
      Serial.println(millis() - bootTime);
      configured = true; // Exit configuration mode
    }
    if (menuProgress == 2) { // Exit initial cfg menu once user input all values
      configured = true;
      drawMainScreen();
    }
    bool button1 = checkButton(0); // Left button
    bool button2 = checkButton(1); // Middle button
    bool button3 = checkButton(2); // Right button
    if (button1 == true || button2 == true || button3 == true)
      menuButtons(button1, button2, button3);
  }
}

void menuButtons(bool b1, bool b2, bool b3) {

// Initial cfg menu for threshold temp
  if (b1 && menuProgress == 0) { // reduce threshold temp
    lcd.setCursor(0,1);
    if (thresholdTemp > 0)
      thresholdTemp = thresholdTemp - 0.5;
    lcd.print(thresholdTemp);
    return; // exit function avoid triggering other button presses
  }
  if (b3 && menuProgress == 0) { // increase threshold temp
    lcd.setCursor(0,1);
    if (thresholdTemp < 80)
      thresholdTemp = thresholdTemp + 0.5;
    lcd.print(thresholdTemp);
    return;
  }
  if (b2 && menuProgress == 0) { // set threshold temp
    menuProgress = 1; // go to next initial cfg menu item (set duration)
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Set Duration:");
    lcd.setCursor(0,1);
    lcd.print(relayRun);
    lcd.setCursor(3,1);
    lcd.print("Minutes");
    return;
  }

// Initial cfg menu for fan duration (mins)
  if (b1 && menuProgress == 1) { // reduce duration
    lcd.setCursor(0,1);
    if (relayRun > 1)
      relayRun--;
    if (relayRun < 10) // print a leading zero if needed
      lcd.print("0");
    lcd.print(relayRun);
    return;
  }
  if (b3 && menuProgress == 1) { // increase duration
    lcd.setCursor(0,1);
    if (relayRun < 999)
      relayRun++;
    if (relayRun < 10) // print leading zero if needed
      lcd.print("0");
    lcd.print(relayRun);
    return;
  }
  if (b2 && menuProgress == 1) { // set duration (minutes)
    menuProgress = 2; // signal to complete the initial cfg
    // Config is complete. Show config for a few seconds before beginning main program
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Config Complete");
    lcd.setCursor(0,1);
    lcd.print(thresholdTemp);
    lcd.print("C    ");
    lcd.print(relayRun);
    lcd.print("mins");
    delay(3000); // wait a few seconds for user to see this screen
    return;
  }
}

void drawMainScreen() {
  lcd.clear(); // clear LCD
  lcd.setCursor(0,0);
  lcd.print("Set:       Temp:");
  lcd.setCursor(0,1);
  lcd.print(thresholdTemp);
  lcd.print("C");
}

void drawCountdownScreen() {
  if (timers[1] % 100 == 0) { // using modulus to slow down LCD draw problems
    lcd.setCursor(0,1);
    lcd.print("     secs");
    lcd.setCursor(0,1);
    lcd.print((relayRun * 60) - ((millis() - timeNow) / 1000)); // number of second left until relay is disabled
  }
  timers[1]++;
}

void relay(bool set) {
  if (set == true) {
    Serial.println("!!!Threshold Reached!!! - Relay is now enabled");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Fan running for");
    relayEnabled = true; // indicator variable that the relay is running
    digitalWrite(RELAY, LOW); // enable relay
  }
  if (set == false) {
    Serial.println("Duration timer completed - Relay is now disabled");
    relayEnabled = false; // indicator variable that the relay is running
    digitalWrite(RELAY, HIGH); // disable relay
  }
}

void loop(void) {
  delay(5);
  if (testing) { // Only this code runs when in test mode
    delay(500);
    Serial.println("================");
    Serial.println("==TESTING MODE==");
    Serial.println("================");
    Serial.println("To disable testing mode set 'testing' bool variable to false and recompile/upload");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Test Mode");
    unsigned int ctr = 0;
    unsigned int iterations = 0;
    while (testing) { 
      delay(1000); // 1000ms delay == 1 second delay
      if (ctr % 30 == 0) { // wait 60 iterations. 60 seconds in total between flips
        Serial.println();
        Serial.println("Flipping");
        if (relayEnabled) {
          relay(false);
          relayEnabled = false;
        } else {
          relay(true);
          relayEnabled = true;
        }
        iterations++;
        lcd.setCursor(0,1);
        lcd.print(iterations);
      }
      ctr++;
      Serial.print(".");
    }
  }

  double temp = getTemp();
  if (temp >= thresholdTemp && relayEnabled == false) { // if room temp is at|above set threshold temp, turn on fan
    timeNow = millis(); // get the time now so we can count the duration the relay should be enabled
    relay(true);
  }
  if (relayEnabled == true) {
    drawCountdownScreen();
    Serial.print("Relay enabled, count down until switching off: ");
    Serial.println((relayRun * 60) - ((millis() - timeNow) / 1000));
    if (((relayRun * 60) - ((millis() - timeNow) / 1000)) <= 0) { // If duration time is completed, turn off relay
      relay(false); // duration timer complete, turn off the relay now
      drawMainScreen();
    }
  }
}


