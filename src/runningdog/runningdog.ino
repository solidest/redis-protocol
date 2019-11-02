
#include <DS1307RTC.h>
#include <Time.h>
#include <Wire.h>
#include <SD.h>
#include <TFT.h>  // Arduino LCD library
#include <SPI.h>
#include "pitches.h"

// notes in the melody:
const int melody[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
const int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};

#define HUMAN_LIMIT 300

#define cs   10
#define dc   9
#define rst  8

#define _SET_TIME_

const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

TFT TFTscreen = TFT(cs, dc, rst);

const int beepPin = 3; //12;
const int buttonPin = 2;
const int humanIn = 7;
const int lockPin = 11;
const int temperaturePin = A6;  

int humanLeaveCount = 0;
int humanActiveCount = 0;

bool beepOn = false;
bool isLock = false;

tmElements_t tm;
int tm_count = -1;

float temparray[10];
float temperature = 0;
int tempidx = -1;

void setup() {
  pinMode(lockPin, OUTPUT);
  pinMode(beepPin, OUTPUT);
  pinMode(humanIn, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonPin), onButton, RISING);

  Serial.begin(9600);
  
#ifdef _SET_TIME_
  while (!Serial);
  delay(200);
  
  bool parse=false;
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

  setupTFT();

}


void loop() {
  if(tm_count==-1) {
    updateDateTime();
  }
  tm_count++;
  if(tm_count==58) {
    updateDateTime();
    tm_count = 0;
  }

  udpateTemprature();

  int humanInState = digitalRead(humanIn);
  if (humanInState == HIGH) {
    if(humanLeaveCount>HUMAN_LIMIT) {
      playMusic();
      updateDateTime();
    }
    humanActiveCount += 1;
    humanLeaveCount = 0;
  }
  else {
    humanLeaveCount+=1;
    if(humanLeaveCount==HUMAN_LIMIT) {
      playMusic();
      humanActiveCount=0;
      updateDateTime();
    }
  }
  tryLock();
  delay(1000);
}

void setupTFT() {
  TFTscreen.begin();
  TFTscreen.background(0, 0, 0);
  TFTscreen.stroke(255, 0, 0);
  TFTscreen.setTextSize(2);
  TFTscreen.text("www.kiyun.com", 2, 5);
}

//播放音乐
void playMusic() {
  for (int thisNote = 0; thisNote < 8; thisNote++) {

    // to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(beepPin, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(beepPin);
  }
}

//按钮按下
void onButton() {
  if(beepOn) {
    digitalWrite(beepPin, LOW);
    beepOn = false;
  } else {
    digitalWrite(beepPin, HIGH);
    beepOn = true;
  }
}

//尝试锁住继电器
void tryLock() {
  if(temperature>25 && !isLock) {
    digitalWrite(lockPin, HIGH);
    isLock = true;
  } else if(temperature<24 && isLock) {
    digitalWrite(lockPin, LOW);
    isLock = false;
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
  float temp = (val*5.0/1024*100);  //把0~5V对应模拟口读数1~1024; 100=1000/10,1000是毫伏与伏的转换,每10毫伏对应一度温升
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
