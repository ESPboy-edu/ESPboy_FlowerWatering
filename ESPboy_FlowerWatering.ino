#include "lib/ESPboyInit.h"
#include "lib/ESPboyInit.cpp"
#include "lib/RTClib/RTClib.h"
#include "lib/RTClib/RTClib.cpp"
#include <ESP_EEPROM.h>

#define MAX_TIME_WATERING 10000
#define MAX_FLOW_WATERING 200
#define MAX_HUM_WATERING 10
#define MAX_TIME_FLOW_CHECK 3000
#define MAX_QUANT_FLOW_CHECK 5

#define SETTING_MAX_PERIOD 172800
#define SETTING_MIN_PERIOD 3600
#define SETTING_MAX_HUM 1000
#define SETTING_MIN_HUM 100
#define SETTING_MAX_FLOW 500
#define SETTING_MIN_FLOW 10
#define SETTING_MAX_TIME 30000 
#define SETTING_MIN_TIME 3000

#define DEFAULT_PERIOD_SEC 60
#define APP_ID 0xABCD


RTC_DS3231 rtc;
ESPboyInit myESPboy;
TFT_eSprite sprite = TFT_eSprite(&myESPboy.tft);
DateTime nowTime;


struct wateringData{
  bool noFlowFlag = 0;
  bool flowHalPrev = 0;
  bool flowHalCur = 0;
  uint32_t appID = APP_ID;
  uint32_t startCounter = 0;
  uint32_t secondsPeriod = DEFAULT_PERIOD_SEC; //periods between ESPboy startings in seconds
  uint32_t flowCounterTotal = 0; //total water pumped in watercounter rpm
  uint32_t flowTimeCounterTotal = 0; //total water pumped in min/secs
  uint32_t flowCounterCurrent = 0; //current watering counter in watercounter rpm
  uint32_t flowTimeCurrent = 0; //current watering time counter
  uint32_t humidityCounter = 0; //current humidity
  uint32_t humidityLimit = MAX_HUM_WATERING/2; //max humidity setting for water pumping
  uint32_t flowCounterLimit = MAX_FLOW_WATERING/2; //max humidity setting for water pumping
  uint32_t flowTimeLimit = MAX_TIME_WATERING/2; //max time setting for water pumping
} wateringDataSet;


struct saveData{
  bool noFlowFlag;
  uint32_t appID;
  uint32_t secondsPeriod;
  uint32_t startCounter;
  uint32_t flowCounterTotal;
  uint32_t flowTimeCounterTotal;
  uint32_t humidityLimit;
  uint32_t flowCounterLimit;
  uint32_t flowTimeLimit;
} saveDataSet;


void ESPboySleep(){
  saveData();
  nowTime = rtc.now();
  rtc.setAlarm1(nowTime+wateringDataSet.secondsPeriod, DS3231_A1_Date);
  rtc.writeSqwPinMode(DS3231_Alarm1);
  myESPboy.myLED.setRGB(0, 0, 0);
  rtc.clearAlarm(1);
}


void doCheckAndWatering(){
  uint32_t cntrTime = millis();

  wateringDataSet.startCounter++;
  
  if (wateringDataSet.noFlowFlag) myESPboy.myLED.setRGB(100,0,0);
  else myESPboy.myLED.setRGB(100,100,0);
  
  if (!wateringDataSet.noFlowFlag) myESPboy.mcp.digitalWrite(15, HIGH);

  while (millis() < cntrTime + MAX_TIME_WATERING){
    drawUI();
    delay(5);
    
    wateringDataSet.humidityCounter = 1024-analogRead(A0);
    wateringDataSet.flowTimeCurrent = millis() - cntrTime;
    
    wateringDataSet.flowHalPrev = wateringDataSet.flowHalCur;
    wateringDataSet.flowHalCur = myESPboy.mcp.digitalRead(14);
    if (!wateringDataSet.flowHalCur && wateringDataSet.flowHalPrev) 
      wateringDataSet.flowCounterCurrent++;
    
    if (wateringDataSet.flowTimeCurrent > MAX_TIME_FLOW_CHECK && wateringDataSet.flowCounterCurrent < MAX_QUANT_FLOW_CHECK) {
      wateringDataSet.noFlowFlag=1;
      break;}

    if (wateringDataSet.flowTimeCurrent > wateringDataSet.flowTimeLimit) {
      break;}   
    
    if (wateringDataSet.humidityCounter > wateringDataSet.humidityLimit) {
      break;}
  }

  myESPboy.mcp.digitalWrite(15, LOW);

  wateringDataSet.flowCounterTotal+=wateringDataSet.flowCounterCurrent;
  wateringDataSet.flowTimeCounterTotal+=millis()-cntrTime;
  drawUI();
  wateringDataSet.flowCounterCurrent = 0;
  wateringDataSet.flowTimeCurrent = 0;
  saveParameters();

  if (!wateringDataSet.noFlowFlag && wateringDataSet.humidityCounter < wateringDataSet.humidityLimit){
    myESPboy.tft.setTextSize(1);
    myESPboy.tft.setTextColor(TFT_GREEN); 
    myESPboy.myLED.setRGB(0, 100, 0);
    myESPboy.tft.drawString ("Watering DONE!", 20, 120);
  }

  if (!wateringDataSet.noFlowFlag && wateringDataSet.humidityCounter > wateringDataSet.humidityLimit){
    myESPboy.tft.setTextSize(1);
    myESPboy.tft.setTextColor(TFT_BLUE); 
    myESPboy.myLED.setRGB(0, 0, 100);
    myESPboy.tft.drawString ("Humidity OK", 30, 120);
  }
  
  delay(3000);
  ESPboySleep();    
}


void loadParameters(){
  EEPROM.get(0,saveDataSet);
  if (saveDataSet.appID == APP_ID){
    wateringDataSet.startCounter = saveDataSet.startCounter;
    wateringDataSet.noFlowFlag = saveDataSet.noFlowFlag;
    wateringDataSet.secondsPeriod = saveDataSet.secondsPeriod;
    wateringDataSet.flowCounterTotal = saveDataSet.flowCounterTotal;
    wateringDataSet.flowTimeCounterTotal = saveDataSet.flowTimeCounterTotal;
    wateringDataSet.humidityLimit = saveDataSet.humidityLimit;
    wateringDataSet.flowCounterLimit = saveDataSet.flowCounterLimit;
    wateringDataSet.flowTimeLimit = saveDataSet.flowTimeLimit;
    }
  else
     saveParameters();
};


void saveParameters(){
    saveDataSet.appID = wateringDataSet.appID;
    saveDataSet.startCounter = wateringDataSet.startCounter;
    saveDataSet.noFlowFlag = wateringDataSet.noFlowFlag;
    saveDataSet.secondsPeriod = wateringDataSet.secondsPeriod;
    saveDataSet.flowCounterTotal = wateringDataSet.flowCounterTotal;
    saveDataSet.flowTimeCounterTotal = wateringDataSet.flowTimeCounterTotal;
    saveDataSet.humidityLimit = wateringDataSet.humidityLimit;
    saveDataSet.flowCounterLimit = wateringDataSet.flowCounterLimit;
    saveDataSet.flowTimeLimit = wateringDataSet.flowTimeLimit;
    EEPROM.put(0,saveDataSet);
    EEPROM.commit();
};


void drawUI(){
  sprite.fillSprite(TFT_BLACK);

  sprite.setTextColor(TFT_GREEN); 
  sprite.setTextSize(2);
  sprite.drawString ("Telemetry",0,0);
  sprite.setTextSize(1);
  sprite.drawString ("Start No " + (String)wateringDataSet.startCounter, 0, 16);
  sprite.drawString ("HumidityNow " + (String)wateringDataSet.humidityCounter, 0, 24);
  sprite.drawString ("FlowVol " + (String)wateringDataSet.flowCounterCurrent, 0, 32);
  sprite.drawString ("TimeOfFlow " + (String)(wateringDataSet.flowTimeCurrent/1000), 0, 40);
  sprite.drawString ("FlowVolTotal " + (String)wateringDataSet.flowCounterTotal, 0, 48);
  sprite.drawString ("TimeOfFlowTotal " + (String)(wateringDataSet.flowTimeCounterTotal/1000), 0, 56);

  sprite.setTextColor(TFT_YELLOW); 
  sprite.setTextSize(2);
  sprite.drawString ("Settings",0,66);
  sprite.setTextSize(1);
  sprite.drawString ("PeriodToStartHr " + (String)(wateringDataSet.secondsPeriod/3600), 0, 82);
  sprite.drawString ("HumidityLimit " + (String)wateringDataSet.humidityLimit, 0, 90);
  sprite.drawString ("FlowLimit " + (String)wateringDataSet.flowCounterLimit, 0, 98);
  sprite.drawString ("TimeOfFlowLimitSc " + (String)(wateringDataSet.flowTimeLimit/1000), 0, 106);

  if (wateringDataSet.noFlowFlag){
    sprite.setTextColor(TFT_RED); 
    sprite.drawString ("NO WATER! A+B - RESET",0,120);}

  sprite.pushSprite(0, 0);
};


void setup () {
  myESPboy.begin("Flowers watering");
  EEPROM.begin(sizeof(saveDataSet));
  sprite.createSprite(128, 128);
  
  loadParameters();

  myESPboy.tft.setTextSize(1);
  myESPboy.tft.setTextColor(TFT_RED); 

  if (!rtc.begin()) {
    myESPboy.myLED.setRGB(100, 0, 0);
    myESPboy.tft.drawString ("RTC not found", 24, 120);
    delay(20000);
    ESP.reset();
  }

  if (wateringDataSet.noFlowFlag) {
    myESPboy.myLED.setRGB(100, 0, 0);
    myESPboy.tft.drawString ("No water!", 36, 120);
    delay(5000);
    ESPboySleep();}
  
  myESPboy.mcp.pinMode(15, OUTPUT);
  myESPboy.mcp.digitalWrite(15, LOW);
  pinMode(A0, INPUT);  
  myESPboy.mcp.pinMode(14, INPUT);
  myESPboy.mcp.pinMode(13, INPUT);
  delay(100);
  
  //if (rtc.lostPower()){  
  //  Serial.println("RTC lost power!");
  //  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));}
  
  drawUI();
  myESPboy.myLED.setRGB(0, 0, 0);
}


void loop () {
  static uint8_t readedKey;
  static bool updateFlag = false;
  static uint32_t tmrCnt;
  static uint32_t redrawCnt;
  
  if(rtc.alarmFired(1)) doCheckAndWatering();

  readedKey = myESPboy.getKeys();
  if (readedKey){
    if (readedKey&PAD_ACT && readedKey&PAD_ESC){
      wateringDataSet.noFlowFlag = 0;
      wateringDataSet.startCounter = 0;
      wateringDataSet.flowCounterTotal = 0;
      wateringDataSet.flowTimeCounterTotal = 0;
      wateringDataSet.flowCounterCurrent = 0;
      wateringDataSet.flowTimeCurrent = 0;
      updateFlag = true;
     }
   if (readedKey&PAD_UP && wateringDataSet.secondsPeriod < SETTING_MAX_PERIOD) wateringDataSet.secondsPeriod+=3600;
   if (readedKey&PAD_DOWN && wateringDataSet.secondsPeriod > SETTING_MIN_PERIOD) wateringDataSet.secondsPeriod-=3600;
   if (readedKey&PAD_LEFT && wateringDataSet.humidityLimit > SETTING_MIN_HUM) wateringDataSet.humidityLimit-=5;
   if (readedKey&PAD_RIGHT && wateringDataSet.humidityLimit < SETTING_MAX_HUM) wateringDataSet.humidityLimit+=5;
   if (readedKey&PAD_ESC && wateringDataSet.flowCounterLimit < SETTING_MAX_FLOW) wateringDataSet.flowCounterLimit+=10;
   if (readedKey&PAD_ACT && wateringDataSet.flowCounterLimit > SETTING_MIN_FLOW) wateringDataSet.flowCounterLimit-=10;
   if (readedKey&PAD_RGT && wateringDataSet.flowTimeLimit < SETTING_MAX_TIME) wateringDataSet.flowTimeLimit+=1000;
   if (readedKey&PAD_LFT && wateringDataSet.flowTimeLimit > SETTING_MIN_TIME) wateringDataSet.flowTimeLimit-=1000;
     
   delay(50);
   updateFlag = true;
   drawUI();
   }

  if (updateFlag && millis()-tmrCnt > 2000){
    tmrCnt = millis();
    updateFlag = false;
    nowTime = rtc.now();
    rtc.setAlarm1(nowTime+wateringDataSet.secondsPeriod, DS3231_A1_Date);
    rtc.writeSqwPinMode(DS3231_Alarm1);
    saveParameters();
  }

  if (millis()>redrawCnt+500){
    wateringDataSet.humidityCounter = 1024-analogRead(A0);
    redrawCnt=millis();
    drawUI();}
   
  delay(10);
}
