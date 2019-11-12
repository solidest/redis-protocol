
#include <DS1307RTC.h>
#include <Time.h>
#include <Wire.h>
#include <SD.h>
#include <TFT.h>  // Arduino LCD library
#include <SPI.h>
//#include "pitches.h"
#include "huluwa.h"

//#define _SET_TIME_

#ifdef _SET_TIME_
const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
#endif

//人体离开时间间隔
#define HUMAN_LIMIT 300 //5分钟

//引脚定义
#define beepPin 3
#define buttonPin 2
#define humanIn 7
#define lockPin 5
#define temperaturePin A6

//temperature adjust
#define TEMP_DIFF 10

#define cs   10
#define dc   9
#define rst  8

//葫芦娃
char length_hlw1;
char length_hlw2;
char length_hlw3;

const int melody_hlw1[] = 
  {NOTE_DH1,NOTE_D6,NOTE_D5,NOTE_D6,NOTE_D0,
  NOTE_DH1,NOTE_D6,NOTE_D5,NOTE_DH1,NOTE_D6,NOTE_D0};


const int melody_hlw2[] =
  {NOTE_DH1,NOTE_D0,NOTE_D6,NOTE_D6,NOTE_D5,NOTE_D5,NOTE_D6,NOTE_D6,
  NOTE_D0,NOTE_D5,NOTE_D1,NOTE_D3,NOTE_D0};
 
const int melody_hlw3[] =
  {NOTE_D1,NOTE_D1,NOTE_D3,
  NOTE_D1,NOTE_D1,NOTE_D3,NOTE_D0,
  NOTE_D6,NOTE_D6,NOTE_D6,NOTE_D5,NOTE_D6,
  NOTE_D5,NOTE_D1,NOTE_D3,NOTE_D0,
  NOTE_DH1,NOTE_D6,NOTE_D6,NOTE_D5,NOTE_D6,
  NOTE_D5,NOTE_D1,NOTE_D2,NOTE_D0};
  
const float noteDurations_hlw1[] =
  {1,1,0.5,0.5,1,
  0.5,0.5,0.5,0.5,1,0.5};

const float noteDurations_hlw2[] =
  {0.5,0.5,0.5+0.25,0.25,0.5+0.25,0.25,0.5+0.25,0.25,
  0.5,1,0.5,1,1};
   
const float noteDurations_hlw3[] =
  {1,1,1+1,
  0.5,1,1+0.5,1,
  1,1,0.5,0.5,1,
  0.5,1,1+0.5,1,
  0.5,0.5,0.5,0.5,1+1,
  0.5,1,1+0.5,1};

//人体感应计数
int humanLeaveCount = 0;
int humanActiveCount = 1;

//蜂鸣器
int beepIdx = 0;

//继电器
bool isLock = false;

//时间计数
tmElements_t tm;
int tm_count = 0;

//温度传感器
float temparray[10];
float temperature = 0;
int tempidx = -1;

TFT TFTscreen = TFT(cs, dc, rst);

void setup() {
  
  Serial.begin(9600);
  
  length_hlw1 = sizeof(melody_hlw1)/sizeof(melody_hlw1[0]);
  length_hlw2 = sizeof(melody_hlw2)/sizeof(melody_hlw2[0]);
  length_hlw3 = sizeof(melody_hlw3)/sizeof(melody_hlw3[0]);
  
  pinMode(lockPin, OUTPUT);
  pinMode(beepPin, OUTPUT);
  pinMode(humanIn, INPUT);
  //pinMode(buttonPin, INPUT_PULLUP);

  //attachInterrupt(digitalPinToInterrupt(buttonPin), onButton, FALLING);
  //digitalWrite(beepPin, HIGH);
  
  digitalWrite(lockPin, HIGH);

  
#ifdef _SET_TIME_
  //设置时间
  while (!Serial);
  delay(200);
  
  bool parse =false;
  bool config=false;
  
  if (getDate(__DATE__) && getTime(__TIME__)) {
    parse = true;
    // and configure the RTC with this info
    if (RTC.write(tm)) {
      config = true;
    }
  }
  if (parse && config) {
    Serial.print("DS1307 configured Time ok");
  } else if (parse) {
    Serial.println("DS1307 configured Time Errir");
  } else {
    Serial.print("Could not parse info from the compiler");
  }
#endif

  //setup TFT
  TFTscreen.begin();
  TFTscreen.background(0, 0, 0);
  TFTscreen.stroke(255, 0, 0);
  TFTscreen.setTextSize(2);
  TFTscreen.text("www.kiyun.com", 2, 5);

  updateDateTime();
  udpateTemprature();
}


void loop() {

  if(beepIdx>0) {
    playMusic(beepIdx);
    beepIdx = 0;
    updateDateTime();
  }
  
  //更新时间
  tm_count++;
  if(tm_count==58) {
    updateDateTime();
    tm_count = 0;
  }
  
  //更新温度
  //udpateTemprature();

  //人体感应计数
  int humanInState = digitalRead(humanIn);

  
  if (humanInState == HIGH) { //有人
    if(humanLeaveCount>=HUMAN_LIMIT) { //刚回来
      humanActiveCount = 1;
      humanLeaveCount = 0;
      updateDateTime();
      playMusic(1);
    } else {  //一直没离开
      int tick = 1 + humanLeaveCount;
      humanLeaveCount = 0;

      //30分钟提醒
      if(humanActiveCount<=60*30 && humanActiveCount+tick>=60*30) {
        playMusic(2);
      }

      //50分钟提醒
      if(humanActiveCount<=60*50 && humanActiveCount+tick>=60*50) {
        playMusic(3);
      }
      
      humanActiveCount += tick;
    }
  }
  else {
    humanLeaveCount+=1;
    if(humanLeaveCount==HUMAN_LIMIT) {
      humanActiveCount=0;
      updateDateTime();
      playMusic(1);
    }
  }

  //检查锁状态
  checkLock();
  redrawNumber();
  delay(1000);
}

//检查继电器锁
void checkLock() {
  if(isLock && humanActiveCount<60*60) {
    digitalWrite(lockPin, HIGH);
    isLock = false;
  }

  if(!isLock && humanActiveCount>=60*60) {
    digitalWrite(lockPin, LOW);
    isLock = true;
  }
}


//播放音乐
void playMusic(int idx) {

  char length_hlw;
  const int *melody_hlw;
  const float *noteDurations_hlw;

  switch(idx) {
    case 1:
      length_hlw = length_hlw1;
      melody_hlw = melody_hlw1;
      noteDurations_hlw = noteDurations_hlw1;
      break;
    case 2:
      length_hlw = length_hlw2;
      melody_hlw = melody_hlw2;
      noteDurations_hlw = noteDurations_hlw2;
      break;
    case 3:
      length_hlw = length_hlw3;
      melody_hlw = melody_hlw3;
      noteDurations_hlw = noteDurations_hlw3;
      break;
    default:
      return;
  }
  
  for (int thisNote = 0; thisNote < length_hlw; thisNote++) {
    
    int noteDuration = noteDurations_hlw[thisNote]*400;
    tone(beepPin, melody_hlw[thisNote], noteDuration);
    
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    noTone(beepPin);
  }
}

//按钮按下
void onButton() {
  beepIdx += 1;
  if(beepIdx>3) {
    beepIdx=1;
  }
  if(Serial) {
    Serial.println(beepIdx);
  }
}


//更新时间
void updateDateTime() {
  if(RTC.read(tm)) {
    
    char buf_date [12];
    char buf_time [8];
    sprintf(buf_date,"%4d-%02d-%02d", tmYearToCalendar(tm.Year), tm.Month, tm.Day);
    sprintf(buf_time, "%02d:%02d", tm.Hour, tm.Minute);

    TFTscreen.stroke(0,0,0);
    TFTscreen.fill(0,0,0);
    TFTscreen.rect(0, 35, TFTscreen.width(), 65);

    TFTscreen.setTextSize(4);
    if(humanActiveCount>0) {
      TFTscreen.stroke(0, 255, 0);
    } else {
      TFTscreen.stroke(0, 0, 255);
    }
    
    TFTscreen.text(buf_time, 23, 35);

    TFTscreen.setTextSize(2);
    TFTscreen.text(buf_date, 19, 80);
  }
}

//print number
void redrawNumber() {
  TFTscreen.stroke(0,0,0);
  TFTscreen.fill(0,0,0);
  TFTscreen.rect(0, 100, TFTscreen.width(), 25);
    
  char txt_chars[20];

  // print new value
  String txt_leave((int)humanLeaveCount/60);
  String txt_active((int)humanActiveCount/60);
  String txt = txt_active + "  -" + txt_leave;
  txt.toCharArray(txt_chars,10);
  TFTscreen.setTextSize(2);
  TFTscreen.stroke(255, 255, 0);
  TFTscreen.text(txt_chars, 40, 105);
}

//打印温度
void redrawTemprature() {
  TFTscreen.stroke(0,0,0);
  TFTscreen.fill(0,0,0);
  TFTscreen.rect(0, 100, TFTscreen.width(), 25);
    
  char temp_chars[10];

  // print new value
  String temp_text(temperature);
  String temp_unit("c");
  temp_text = temp_text+temp_unit;
  temp_text.toCharArray(temp_chars,10);
  TFTscreen.setTextSize(2);
  TFTscreen.stroke(255, 255, 0);
  TFTscreen.text(temp_chars, 68, 105);
}

//更新温度
void udpateTemprature() {
  long val=analogRead(temperaturePin);
  float temp = (val*5.0/1024*100)-TEMP_DIFF;  //把0~5V对应模拟口读数1~1024; 100=1000/10,1000是毫伏与伏的转换,每10毫伏对应一度温升
  if(tempidx==-1) {
    temperature = temp;
    tempidx = 0;
    temparray[0] = temp;
    redrawTemprature();
  } else {
    temparray[tempidx] = temp;
  }

  if(tempidx==9) {
    tempidx = 0;
    float t=0;
    for(int i=0; i<10; i++) {
      t+=temparray[i];
    }
    float new_temp = t/10;
    if((new_temp>temperature) || (new_temp<temperature)) {
      temperature = new_temp;
      redrawTemprature();
    }
  }
 
  tempidx++;
}

#ifdef _SET_TIME_

//获取时间
bool getTime(const char *str) {
  int Hour, Min, Sec;

  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}

//获取日期
bool getDate(const char *str) {
  char Month[12];
  int Day, Year;
  uint8_t monthIndex;

  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month, monthName[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;
  tm.Day = Day;
  tm.Month = monthIndex + 1;
  tm.Year = CalendarYrToTm(Year);
  return true;
}

#endif
