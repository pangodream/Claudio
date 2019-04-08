/**
 * GLOBAL PIN CONFIGURATION
 */

const int TFT_CS = 15;
const int TFT_DC = 4;
const int TFT_MOSI = 23;
const int TFT_SLK = 18;
const int TFT_RST = 2;
const int TFT_LED = 19;     
const int BUZZER = 5; 
const int LUX_SDA = 21;
const int LUX_SCL = 22;
const int DHT_OUT = 27;
/**
 * EEPROM libraries and resources
 */
#include "EEPROM.h"
#define EEPROM_SIZE 64
 
/**
 * ILI9341 TFT libraries and resources
 */
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
//                                      cs  dc  mosi slk rst
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SLK, TFT_RST);

#define ILI9341_LCYAN   0x0418
#define ILI9341_LORANGE 0xFC08
#define ILI9341_LGREEN  0x87F0
#define ILI9341_LGRAY   0x8410

/**
 * BH1750 Lux meter libraries and resources
 */
#include <BH1750FVI.h>
// Create the Lightsensor instance
BH1750FVI LightSensor(BH1750FVI::k_DevModeContHighRes); 

/**
 * DHT-11 Temp and humidity sensor  libraries and resources
 */
#include "DHT.h"
#define DHTTYPE DHT11
DHT dht(DHT_OUT, DHTTYPE);       

/**    
 *  WIFI Libraries and resources   
 */
#include <WiFi.h>
//Absolute path to file containing WiFi credentials
//const char* ssid       = "MyApSSID";
//const char* password   = "MyApPassphrase";
#include <c:\datos\desarrollo\arduino\credentials.h>
const int connTimeout = 10; //Seconds

WiFiServer wifiServer(1234);


/** 
 *  TIME libraries and resources
 */
#include "time.h"
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
 
/**
 * PWM Constants
 */
const int freq = 5000;
const int tftledChannel = 0;
const int resolution = 8;
/**
 * TFT Display Constants 
 */
const int xTime = 14;
const int yTime = 14;
const int tftBG = ILI9341_BLACK;
const int tftTimeFG = ILI9341_RED;


 
/**
 * GLOBALS
 */
String   prevTime = "";
String   currTime = "";
String   prevDate = "";
String   currDate = ""; 
uint16_t prevLux = 0;
uint16_t currLux = 0;
float    prevTemp = 0;
float    currTemp = 0;
float    prevHumi = 0;
float    currHumi = 0;
bool     onWifi = false;
String   weekDays[] = {"", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab", "Dom"};
String   months[] = {"", " Ene ", "Feb", "Mar", "Abr", "May", "Jun", "Jul", "Ago", "Sep", "Oct", "Nov", "Dic"};
int      pinTouchR = 14; 
int      pinTouchM = 13; 
int      pinTouchL = 12;
int      statusTouchR = 0;
int      statusTouchM = 0;
int      statusTouchL = 0;
ulong    lastTouchR;
ulong    lastTouchM;
ulong    lastTouchL;
bool     doAlarm;
String   alarmTime;
bool     onAlarm;
bool     waitingToTouch = false;
bool     onConfig = false;

//Configurable values
int      bootWait; //Seconds
int      touchReadings;
int      thresholdTouchR; 
int      thresholdTouchM; 
int      thresholdTouchL;
int      longTouchThreshold; // Tenths of second
int      snoozeMinutes;
int      alarmBeeps;
byte     maxLux;
byte     minLux;
byte     minBrightness;



void setup() {
  /**
   * Serial port
   */
  Serial.begin(115200);
  
  /**
   * EEPROM
   */
  EEPROM.begin(EEPROM_SIZE);
  /**
   * Loads EEPROM configuration
   */
  loadConfiguration();
  
  /**
   * TFT DISPLAY
   */
  //Background light PWM
  ledcSetup(tftledChannel, freq, resolution);
  ledcAttachPin(TFT_LED, tftledChannel);
  //Start with high intensity 
  setBGLuminosity(255);
  tft.begin();
  tft.setRotation(3);
  yield();
  
  //Boot screen
  tft.fillScreen(ILI9341_BLACK);
  yield();
  tft.setTextSize(4);
  tft.setTextColor(ILI9341_LORANGE);
  tft.println("Claudio!");
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.println("");
  tft.println("Booting...");
  tft.println("Setting up devices...");
  /**
   * BUZZER
   */
  pinMode(BUZZER, OUTPUT); 
  /**
   * Light sensor
   */
  LightSensor.begin(); 
  /**
   * Temperature and humidity sensor
   */
  dht.begin();    
  tft.print("Connecting to WiFi AP "); 
  tft.println(ssid);     
  /**
   * Wifi connect
   */
  wifiConnect();
  if(onWifi == true){
    tft.print("   Connection succeed, obtained IP ");
    tft.println(WiFi.localIP());
  }else{
    tft.println("   Connection failed. Unexpected operation results.");
  }
  tft.println("Obtaining NTP time from remote server...");
  /**
   * NTP Time
   */
  getNtpTime();
  delay(100); //We need a delay to allow info propagation
  
  /**
   * Wifi Server for configuration
   */
  tft.println("Starting remote configuration server...");
  wifiServer.begin();

  tft.println("End of booting process.");
   
  //Wait twenty seconds or until the user touches a button
  waitTouch(bootWait);

  //Prepare screen for normal operation
  tft.fillScreen(ILI9341_BLACK);
  yield();

  //Display touch buttons
  displayButton(0, false, false);
  displayButton(1, false, false);
  displayButton(2, false, false);
  
  /** 
   *  TESTING
   */


}

void loop() {
  getCurrentLux();
  getCurrentTemp();
  getCurrentHumi();
  refreshTime();
  readTouch();
  configMode();
  delay(100);
}

/**
 * Checks for next alarm in queue
 */
void checkAlarm(){
  //TODO: Check in queue what is the nex alarm to be activated...
  alarmTime = hourMinuteToTime(getAlarmHour(), getAlarmMinute()); 
  doAlarm = true;
  displayAlarm(); 
}
/**
 * Connects to WIFI
 */

bool wifiConnect(){
  onWifi = false;
  int retries = 0;
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      retries++;
      if(retries == (connTimeout * 2)){
        Serial.println(" TIMEOUT");
        break;
      }
  }
  if(WiFi.status() == WL_CONNECTED){
    onWifi = true;
    Serial.println(" CONNECTED");
  }
  return onWifi;
}
/**
 * Obtains time from NTP server
 */
bool getNtpTime(){
  bool result = false;
  if(onWifi == true){
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    result = true;
  }else{
    Serial.println("getNtpTime: Not connected to wifi!"); 
  }
  return result;
}

/**
 * Returns day of week (1-Mon, 2-Tue, ...., 7-Sun)
 */
int getDayOfWeek(){ //1-Mon, 2-Tue, ...., 7-Sun
  int wDay;
  time_t now;
  struct tm * timeinfo;
  time(&now);
  timeinfo = localtime(&now); 
  wDay = timeinfo->tm_wday;
  if(wDay == 0){
    wDay = 7;
  }
  return wDay;
}
/**
 * Adds a number of snooze minutes to s String time
 */
String addSnooze(String sTime, int snooze){
  int hour;
  int minute;
  int tMinutes;
  int tHours;
  String newTime;
  char cTime[5]=" ";
  hour = sTime.substring(0,2).toInt();
  minute = sTime.substring(3,5).toInt();
  tMinutes = (minute+snooze) % 60;
  tHours = (hour + ((minute+snooze) / 60)) % 24;

  sprintf(cTime, "%02d:%02d", tHours, tMinutes); 
  newTime = (char*)cTime;
  return newTime;
}
/**
 * Returns a string formatted HH:MM based on hours and minutes
 */
String hourMinuteToTime(int hour, int minute){
  String sTime;
  char cTime[5]=" ";
  sprintf(cTime, "%02d:%02d", hour, minute); 
  sTime = (char*)cTime; 
  return sTime;
}
/*
 * Returns current time in HH:MM format
 */
String refreshTime(){
  //Time
  time_t now;
  struct tm * timeinfo;
  time(&now);
  timeinfo = localtime(&now); 
  prevTime = currTime;
  currTime = hourMinuteToTime(timeinfo->tm_hour, timeinfo->tm_min); 
  if(prevTime != currTime){
    timeChanged(prevTime, currTime);
    //If time has changed, lets check if date has changed too
    //Date
    int wDay;
    wDay = timeinfo->tm_wday;
    if(wDay == 0){
      wDay = 7;
    }
    String calDate = "";
    char cDate[30]=" ";
    //Serial.println(weekDays[wDay]);
    //sprintf(cDate, "%s, %i de %s de %i", weekDays[wDay], timeinfo->tm_mday, months[(timeinfo->tm_mon + 1)],(timeinfo->tm_year + 1900)); 
    calDate = calDate + weekDays[wDay];
    calDate = calDate + ", ";
    calDate = calDate + timeinfo->tm_mday;
    calDate = calDate + " ";
    calDate = calDate + months[(timeinfo->tm_mon + 1)];
    calDate = calDate + " ";    
    calDate = calDate + (timeinfo->tm_year + 1900);
    if(calDate.length() == 21){
      calDate = " " + calDate;
    }
    prevDate = currDate;
    currDate = calDate;
    if(prevDate != currDate){
      dateChanged(prevDate, currDate);
    }
  }
}

/**
 * Displays time string erasing the previous one
 */
void displayTime(){
  tft.setTextSize(10);
  tft.setCursor(xTime, yTime);
  tft.setTextColor(tftBG);
  yield();
  tft.println(prevTime);
  yield();
  tft.setCursor(xTime, yTime);
  tft.setTextColor(tftTimeFG);
  yield();
  tft.println(currTime);
  yield();
}
/**
 * Displays date string 
 */
void displayDate(){
  tft.fillRect(14, 94, 192, 30, ILI9341_BLACK);
  tft.setTextSize(3);
  tft.setCursor(26, 94);
  tft.setTextColor(ILI9341_LGREEN);
  yield();
  tft.println(currDate);
}
/**
 * Displays temperature
 */
void displayTemp(){
  int bgColor = 0;
  if(currTemp == prevTemp){
    bgColor = ILI9341_LGREEN;
  }else if(currTemp < prevTemp){
    bgColor = ILI9341_LCYAN;
  }else{
    bgColor = ILI9341_LORANGE;
  }
  
  tft.setTextColor(ILI9341_BLACK);
  yield();
  tft.fillRect(14, 170, 92, 60, bgColor);
  tft.drawRect(14, 170, 92, 60, ILI9341_WHITE);
  yield();
  tft.setTextSize(2);
  tft.setCursor(34, 174);
  tft.print("TEMP");
  tft.setTextSize(3);
  tft.setCursor(16, 200);
  tft.print(currTemp);
}
/**
 * Displays relative humidity
 */
void displayHumi(){
  int bgColor = 0;
  if(currHumi == prevHumi){
    bgColor = ILI9341_LGREEN;
  }else if(currHumi < prevHumi){
    bgColor = ILI9341_LCYAN;
  }else{
    bgColor = ILI9341_LORANGE;
  }
  tft.setTextColor(ILI9341_BLACK);
  yield();
  tft.fillRect(113, 170, 92, 60, bgColor);
  tft.drawRect(113, 170, 92, 60, ILI9341_WHITE);
  yield();
  tft.setTextSize(2);
  tft.setCursor(135, 174);
  tft.print("H.R.");
  tft.setTextSize(3);
  tft.setCursor(115, 200);
  tft.print(currHumi);
}
/**
 * Displays lux 
 */
void displayLux(){
  int bgColor = 0;
  if(currLux == prevLux){
    bgColor = ILI9341_LGREEN;
  }else if(currLux < prevLux){
    bgColor = ILI9341_LCYAN;
  }else{
    bgColor = ILI9341_LORANGE;
  }
  tft.setTextColor(ILI9341_BLACK);
  yield();
  tft.fillRect(212, 170, 92, 60, bgColor);
  tft.drawRect(212, 170, 92, 60, ILI9341_WHITE);
  yield();
  tft.setTextSize(2);
  tft.setCursor(242, 174);
  tft.print("LUX");
  tft.setTextSize(3);
  tft.setCursor(214, 200);
  char cLux[5]=" ";
  sprintf(cLux, "%05d", currLux); 
  //currTime = (char*)cTime;
  tft.print(cLux);
}
/**
 * Displays Alarm Time 
 */
void displayAlarm(){
  if(doAlarm == true){
    tft.setTextColor(ILI9341_YELLOW);
  }else{
    tft.setTextColor(ILI9341_LGRAY);
  }
  yield();
  tft.fillRect(212, 129, 92, 36, ILI9341_BLACK);
  yield();
  tft.setTextSize(3);
  tft.setCursor(214, 133);
  tft.print(alarmTime);
}
/**
 * Sets button status
 * 0 - Left
 * 1 - Middle
 * 2 - Right
 */
void displayButton(int button, bool activated, bool longTouch){
  int bgColor;
  int x;
  if(activated == true){
    if(longTouch == true){
      bgColor = ILI9341_RED;
    }else{
      bgColor = ILI9341_YELLOW;
    }
  }else{
    bgColor = ILI9341_LGRAY;
  }
  yield();
  x = 128 + (button * 30);
  tft.fillCircle(x, 144, 11,bgColor);
  tft.drawCircle(x, 144, 11,ILI9341_WHITE);
  yield();
}


/**
 * Sets TFT background luminosity (0-255)
 */
void setBGLuminosity(int level){
  ledcWrite(tftledChannel, level);
}
/**
 * Plays beep on buzzer a number of times
 */
void playBuzzer(int times){
  onAlarm = true;
  for(int t=0; t< times; t++){
    if(onAlarm == false){
      break;
    }
    for(int i=0; i< 200; i++)
    {
      digitalWrite(BUZZER, HIGH);
      delay(1);
      digitalWrite(BUZZER, LOW);
      delay(1);
    }
    readTouch();
    delay(10);
  }
}
/**
 * Waits for the user to touch a button
 * Timeout in seconds (aprox.)
 */
void waitTouch(int timeout){ 
  waitingToTouch = true;
  for(int t=0; t< (timeout * 3.6); t++){
    readTouch();
    if(waitingToTouch == false){
      break;
    }
    delay(100);
  }
}
/**
 * Returns ambient light luxes 
 */
uint16_t getCurrentLux(){
  prevLux = currLux;
  currLux = LightSensor.GetLightIntensity();
  if(prevLux != currLux){
    luxChanged(prevLux , currLux);
  }
  return currLux;
}

/**
 * Returns temperature
 */
float getCurrentTemp(){
  prevTemp = currTemp;
  currTemp = dht.readTemperature();;
  if(prevTemp != currTemp){
    tempChanged(prevTemp , currTemp);
  }
  return currTemp;
}
/**
 * Returns humidity
 */
float getCurrentHumi(){
  prevHumi = currHumi;
  currHumi = dht.readHumidity();
  if(prevHumi != currHumi){
    humiChanged(prevHumi , currHumi);
  }
  return currHumi;
}
/**
 * Reads touch buttons status and launches events
 */
void readTouch(){
  int avgTouchR = 0; int avgTouchM = 0; int avgTouchL = 0;
  for(int i=0; i< touchReadings; i++){
    avgTouchR += touchRead(pinTouchR);
    avgTouchM += touchRead(pinTouchM);
    avgTouchL += touchRead(pinTouchL);
  }   
  yield();
  avgTouchR = avgTouchR / touchReadings;
  avgTouchM = avgTouchM / touchReadings;
  avgTouchL = avgTouchL / touchReadings;

  //LEFT BUTTON  
  if(avgTouchL < thresholdTouchL){
    if(statusTouchL == 0){
      lastTouchL = millis();
      touchL();
      statusTouchL = 1;
    }else if(statusTouchL == 1){
      if((millis() - lastTouchL) > (longTouchThreshold * 100)){
        statusTouchL = 2;
        touchLongL();
      }
    }  
  }else{
    if(statusTouchL != 0){
      releaseL();
    }
    statusTouchL = 0;    
  }    
  //MIDDLE BUTTON  
  if(avgTouchM < thresholdTouchM){
    if(statusTouchM == 0){
      lastTouchM = millis();
      touchM();
      statusTouchM = 1;
    }else if(statusTouchM == 1){
      if((millis() - lastTouchM) > (longTouchThreshold * 100)){
        statusTouchM = 2;
        touchLongM();
      }
    }  
  }else{
    if(statusTouchM != 0){
      releaseM();
    }
    statusTouchM = 0;    
  }  
  //RIGHT BUTTON  
  if(avgTouchR < thresholdTouchR){
    if(statusTouchR == 0){
      lastTouchR = millis();
      touchR();
      statusTouchR = 1;
    }else if(statusTouchR == 1){
      if((millis() - lastTouchR) > (longTouchThreshold * 100)){
        statusTouchR = 2;
        touchLongR();
      }
    }  
  }else{
    if(statusTouchR != 0){
      releaseR();
    }
    statusTouchR = 0;    
  }  
}

/**
 *  EVENTS
 */ 
/**
 * Event for change of time HH:MM
 */
void timeChanged(String prevTime, String currTime){
  Serial.println("timeChanged event fired!");
  displayTime();
  if(currTime == alarmTime){
    if(doAlarm == true){
      if(onAlarm == false){
        playBuzzer(alarmBeeps);
        doAlarm = false;
      }
    }
  }
}
/**
 * Event for change of date weekDay, day de Month de Year
 */
void dateChanged(String prevDate, String currDate){
  Serial.print("dateChanged event fired! ");
  Serial.println(currDate);
  displayDate();
  //New day, lets check for the next alarm
  checkAlarm();
}
/**
 * Event for change of ambient light
 */
void luxChanged(uint16_t prevLux, uint16_t currLux){
  //Serial.print("luxChanged event fired! ");
  Serial.println(currLux);
  displayLux();

  int finalLux = currLux;
  if(finalLux > maxLux){
    finalLux = maxLux;
  }
  if(finalLux < minLux){
    finalLux = minLux;
  }
  double levelsWidth = maxLux - minLux;
  double level = finalLux - minLux;
  double ratio = level / levelsWidth;
  double brightnessWidth = 255 - minBrightness;
  int brightnessValue = (brightnessWidth * ratio) + minBrightness;
  setBGLuminosity(brightnessValue);
}
/**
 * Event for change of temperature
 */
void tempChanged(float prevTemp, float currTemp){
  Serial.println("tempChanged event fired!");
  Serial.println(currTemp);
  displayTemp();
}
/**
 * Event for change of humidity
 */
void humiChanged(float prevHumi, float currHumi){
  Serial.println("humiChanged event fired!");
  displayHumi();
  
}
/**
 * Touch buttons events
 */
void touchR(){
  Serial.println("Touch Right");
  displayButton(2, true, false);
  touchAny();
}
void touchLongR(){
  Serial.println("Touch Long Right");
  displayButton(2, true, true);
  touchLongAny();
}
void releaseR(){
  Serial.println("Release Right");
  displayButton(2, false, false);
  releaseAny();
}

void touchM(){
  Serial.println("Touch Middle");
  displayButton(1, true, false);
  touchAny();
}
void touchLongM(){
  Serial.println("Touch Long Middle");
  displayButton(1, true, true);
  touchLongAny();
}
void releaseM(){
  Serial.println("Release Middle");
  displayButton(1, false, false);
  releaseAny();
}

void touchL(){
  Serial.println("Touch Left");
  displayButton(0, true, false);
  touchAny();
}
void touchLongL(){
  Serial.println("Touch Long Left");
  displayButton(0, true, true);
  touchLongAny();
}
void releaseL(){
  Serial.println("Release Left");
  displayButton(0, false, false);
  releaseAny();
} 

void touchAny(){
  waitingToTouch = false;
  if(onAlarm == true){
      onAlarm = false;
      doAlarm = false;
      displayAlarm();
  }
}
void touchLongAny(){
  Serial.println("Touch Long Any");
  //Extra alarma functionalities (Alarm Off, Alarm On and Snooze)
  //Check if event occurs during first alarm minute
  if(alarmTime == currTime){
    //Add Snooze time to alarmTime
    alarmTime = addSnooze(alarmTime, snoozeMinutes);
    doAlarm = true;
    displayAlarm();
  }else{
    //Switch On/Off alarm
    doAlarm = !doAlarm;
    displayAlarm();
  }

  //Only for testing!!!
  //Set alarm time equal to current time if left and right buttons are longTouched simultaneously
  if(statusTouchL == 2 && statusTouchR == 2){
    alarmTime = currTime;
    displayAlarm();
    timeChanged(currTime, currTime);
  }
}
void releaseAny(){

}
void configOn(){
  Serial.println("Entered config mode");
}
void configOff(){
  Serial.println("Exited config mode");
}


/**
 * Remote configuration
 */
void configMode(){
  String instruction = "";
  String reply = "";
  WiFiClient client = wifiServer.available();
  if (client) {
    onConfig = true;
    configOn();
    while (client.connected()) {
      while (client.available()>0) {
        char c = client.read();
        if(c == 10){
          reply = configExecute(instruction);
          instruction = "";
          client.println(reply);
        }else{
          instruction = instruction + c;
        }
        
      }
      delay(10);
    }
    client.stop();
    onConfig = false;
    configOff();
  }  
} 
String configExecute(String instruction){
  String command;
  String item;
  String value;
  int setValue;
  command = instruction.substring(0, 3);
  item = instruction.substring(3, 9);
  
  if(command == "GET"){
      value = getConfigValue(item);
  }else if(command == "SET"){
      setValue = instruction.substring(9, 12).toInt();
      value = setConfigValue(item, setValue);
  }else if(command == "RST"){
      ESP.restart();  
  }else{
      value = "Invalid command";
  }
  return value;
}
String getConfigValue(String item){
  String value;
  if(item == "BOOTWT"){
    value = (String) getBootWait();
  }else if(item == "TOUCHR"){
    value = (String) getTouchReadings();
  }else if(item == "THTCHR"){
    value = (String) getThresholdTouchR();
  }else if(item == "THTCHM"){
    value = (String) getThresholdTouchM();
  }else if(item == "THTCHL"){
    value = (String) getThresholdTouchL();
  }else if(item == "LTCHTH"){
    value = (String) getLongTouchThreshold();
  }else if(item == "SNZMIN"){
    value = (String) getSnoozeMinutes();
  }else if(item == "ALBEEP"){
    value = (String) getAlarmBeeps();
  }else if(item == "MAXLUX"){
    value = (String) getMaxLux();
  }else if(item == "MINLUX"){
    value = (String) getMinLux();
  }else if(item == "MINBRG"){
    value = (String) getMinBrightness();
  }else if(item == "ALARHO"){
    value = (String) getAlarmHour();
  }else if(item == "ALARMI"){
    value = (String) getAlarmMinute();
  }else if(item == "DOALAR"){
    value = (String) getDoAlarm();  
  }else{
    value = "Invalid item";
  }  
  return value;
}
String setConfigValue(String item, int setValue){
  String value = "OK";
  if(item == "BOOTWT"){
    setBootWait(setValue);
  }else if(item == "TOUCHR"){
    setTouchReadings(setValue);
  }else if(item == "THTCHR"){
    setThresholdTouchR(setValue);
  }else if(item == "THTCHM"){
    setThresholdTouchM(setValue);
  }else if(item == "THTCHL"){
    setThresholdTouchL(setValue);
  }else if(item == "LTCHTH"){
    setLongTouchThreshold(setValue);
  }else if(item == "SNZMIN"){
    setSnoozeMinutes(setValue);
  }else if(item == "ALBEEP"){
    setAlarmBeeps(setValue);
  }else if(item == "MAXLUX"){
    setMaxLux(setValue);
  }else if(item == "MINLUX"){
    setMinLux(setValue);
  }else if(item == "MINBRG"){
    setMinBrightness(setValue);
  }else if(item == "ALARHO"){
    setAlarmHour(setValue);
  }else if(item == "ALARMI"){
    setAlarmMinute(setValue);
  }else if(item == "DOALAR"){
    setDoAlarm(setValue);  
  }else{
    value = "Invalid item";
  }  
  return value;
}

/**
 * Load Configuration from EEPROM
 */
void loadConfiguration(){
  bootWait = getBootWait();
  touchReadings = getTouchReadings();
  thresholdTouchR = getThresholdTouchR();
  thresholdTouchM = getThresholdTouchM(); 
  thresholdTouchL = getThresholdTouchL();
  longTouchThreshold = getLongTouchThreshold();
  snoozeMinutes = getSnoozeMinutes();
  alarmBeeps = getAlarmBeeps();
  maxLux = getMaxLux();
  minLux = getMinLux();
  minBrightness = getMinBrightness(); 
}
/**
 * Boot Wait
 * Address 0
 */
byte getBootWait(){
  byte value = byte(EEPROM.read(0));
  if(value == 255){
    value = 20;
  }
  return value;
}
void setBootWait(byte value){
  EEPROM.write(0, value);
  EEPROM.commit();
  bootWait = value;
}
/* Touch readings
 * Address 1
 */
byte getTouchReadings(){
  byte value = byte(EEPROM.read(1));
  if(value == 255){
    value = 120;
  }
  return value;
}
void setTouchReadings(byte value){
  EEPROM.write(1, value);
  EEPROM.commit();
  touchReadings = value;
}
/* thresholdTouchR
 * Address 2
 */
byte getThresholdTouchR(){
  byte value = byte(EEPROM.read(2));
  if(value == 255){
    value = 70;
  }
  return value;
}
void setThresholdTouchR(byte value){
  EEPROM.write(2, value);
  EEPROM.commit();
  thresholdTouchR = value;
}
/* thresholdTouchM
 * Address 3
 */
byte getThresholdTouchM(){
  byte value = byte(EEPROM.read(3));
  if(value == 255){
    value = 65;
  }
  return value;
}
void setThresholdTouchM(byte value){
  EEPROM.write(3, value);
  EEPROM.commit();
  thresholdTouchM = value;
}
/* thresholdTouchL
 * Address 4
 */
byte getThresholdTouchL(){
  byte value = byte(EEPROM.read(4));
  if(value == 255){
    value = 65;
  }
  return value;
}
void setThresholdTouchL(byte value){
  EEPROM.write(4, value);
  EEPROM.commit();
  thresholdTouchL = value;
}
/* longTouchThreshold
 * Address 5
 */
byte getLongTouchThreshold(){
  byte value = byte(EEPROM.read(5));
  if(value == 255){
    value = 12;
  }
  return value;
}
void setLongTouchThreshold(byte value){
  EEPROM.write(5, value);
  EEPROM.commit();
  longTouchThreshold = value;
}
/* snoozeMinutes
 * Address 6
 */
byte getSnoozeMinutes(){
  byte value = byte(EEPROM.read(6));
  if(value == 255){
    value = 9;
  }
  return value;
}
void setSnoozeMinutes(byte value){
  EEPROM.write(6, value);
  EEPROM.commit();
  snoozeMinutes = value;
}
/* alarmBeeps
 * Address 7
 */
byte getAlarmBeeps(){
  byte value = byte(EEPROM.read(7));
  if(value == 255){
    value = 90;
  }
  return value;
}
void setAlarmBeeps(byte value){
  EEPROM.write(7, value);
  EEPROM.commit();
  alarmBeeps = value;
}
/* maxLux
 * Address 8
 */
byte getMaxLux(){
  byte value = byte(EEPROM.read(8));
  if(value == 255){
    value = 20;
  }
  return value;
}
void setMaxLux(byte value){
  EEPROM.write(8, value);
  EEPROM.commit();
  maxLux = value;
}
/* minLux
 * Address 9
 */
byte getMinLux(){
  byte value = byte(EEPROM.read(9));
  if(value == 255){
    value = 0;
  }
  return value;
}
void setMinLux(byte value){
  EEPROM.write(9, value);
  EEPROM.commit();
  minLux = value;
}
/* minBrightness
 * Address 10
 */
byte getMinBrightness(){
  byte value = byte(EEPROM.read(10));
  if(value == 255){
    value = 10;
  }
  return value;
}
void setMinBrightness(byte value){
  EEPROM.write(10, value);
  EEPROM.commit();
  minBrightness = value;
}
/* alarmHour
 * Address 11
 */
byte getAlarmHour(){
  byte value = byte(EEPROM.read(11));
  if(value == 255){
    value = 8;
  }
  return value;
}
void setAlarmHour(byte value){
  EEPROM.write(11, value);
  EEPROM.commit();
  alarmTime = hourMinuteToTime(value, getAlarmMinute()); 
  displayAlarm();
}
/* alarmMinute
 * Address 12
 */
byte getAlarmMinute(){
  byte value = byte(EEPROM.read(12));
  if(value == 255){
    value = 0;
  }
  return value;
}
void setAlarmMinute(byte value){
  EEPROM.write(12, value);
  EEPROM.commit();
  alarmTime = hourMinuteToTime(getAlarmHour(), value); 
  displayAlarm();
}
/* doAlarm
 * Address --- Not stored into EEPROM
 */
byte getDoAlarm(){
  byte value;
  if(doAlarm == true){
    value = 1;
  }else{
    value = 0;
  }
  return value;
}
void setDoAlarm(byte value){
  if(value == 1){
    doAlarm = true;
  }else{
    doAlarm = false;
  }
  displayAlarm();
}
