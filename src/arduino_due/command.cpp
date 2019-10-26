
#include "HardwareSerial.h"
#include "sds.h"
#include "command.h"
#include "ArduinoJson.h"

bool set_serial(HardwareSerial& com, sds cfg) {

  DynamicJsonDocument doc(800);
    
  sds temp1= sdsnew("{\"sensor\":\"gps\",\"time\":1351824120,\"data\":[48.756080,2.302038]}");
  DeserializationError error = deserializeJson(doc, temp1);
  if(error) {
    if(Serial) {
      Serial.println(error.c_str());
    }
    sdsfree(temp1);
    return false; 
  } else {
    size_t need = measureJson(doc)+1;
    sds temp = sdsnewlen(NULL, need);
    size_t len = serializeJson(doc, temp, need);
    sdssetlen(temp, len);
    //serializeJson(doc, Serial);
    Serial.write(temp, len);
    Serial.println("");
    sdsfree(temp);
  }
  sdsfree(temp1);
  return true;
}

bool set_can(int canid, sds cfg) {
  Serial.write(cfg, sdslen(cfg));
  Serial.println("");
  return true;
}
