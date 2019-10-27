
#include <DS1307RTC.h>
#include <Time.h>
#include <Wire.h>

#define HUMAN_LIMIT 60

//#define _SET_TIME_

const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

const int beepPin = 12;
const int buttonPin = 2;
const int humanIn = 7;
const int lockPin = 11;
const int temperaturePin = A6;  

int humanLeaveCount = 0;
int humanActiveCount = 0;
bool beepOn = false;
bool isLock = false;
tmElements_t tm;
float temperature = 0;

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

}


void loop() {
  int humanInState = digitalRead(humanIn);
  if (humanInState == HIGH) {
    if(humanLeaveCount>HUMAN_LIMIT) {
      digitalWrite(beepPin, HIGH);
      delay(300);
      digitalWrite(beepPin, LOW);
    }
    humanActiveCount += 1;
    humanLeaveCount = 0;
    if(Serial) {
      printTime();
      printTemprature();
    }
  }
  else {
    humanLeaveCount+=1;
    if(humanLeaveCount==HUMAN_LIMIT) {
      digitalWrite(beepPin, HIGH);
      delay(300);
      digitalWrite(beepPin, LOW);
      humanActiveCount=0;
    }
  }
  tryLock();
  delay(1000);
}

void onButton() {
  if(beepOn) {
    digitalWrite(beepPin, LOW);
    beepOn = false;
  } else {
    digitalWrite(beepPin, HIGH);
    beepOn = true;
  }
}

void tryLock() {
  if(temperature>25 && !isLock) {
    digitalWrite(lockPin, HIGH);
    isLock = true;
  } else if(temperature<24 && isLock) {
    digitalWrite(lockPin, LOW);
    isLock = false;
  }
}

void printTime() {
  if(RTC.read(tm)) {
    char buffer [128];
    sprintf (buffer,"时间：%4d-%02d-%02d %02d:%02d:%02d", tmYearToCalendar(tm.Year), tm.Month, tm.Day, tm.Hour, tm.Minute, tm.Second);
    Serial.print(buffer);
  }
}

void printTemprature() {
  long val=analogRead(temperaturePin);
  temperature = (val*5.0/1024*100);  //把0~5V对应模拟口读数1~1024; 100=1000/10,1000是毫伏与伏的转换,每10毫伏对应一度温升
  Serial.print("  温度:");
  Serial.print(temperature);
  Serial.println();
}


bool getTime(const char *str) {
  int Hour, Min, Sec;

  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}

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
