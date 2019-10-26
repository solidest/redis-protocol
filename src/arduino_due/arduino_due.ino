/*
  data collection for auduino due 
*/

#include <FreeRTOS_ARM.h>
#include <KrpSender.h>
#include <KrpRecver.h>
#include <vector>

#include "command.h"

using namespace std;

//for hardware
#define ARDUINO_DUE
#define _DEBUG_

#ifdef _DEBUG_
  #define logout(info) if(Serial)Serial.println(info)
#else
  #define logout(info) (void)
#endif

#ifdef ARDUINO_DUE
  #define SERIAL_RX_BUFFER_SIZE 512
  #define SERIAL_TX_BUFFER_SIZE 512
  const char FIFO_SIZE = 200;       //cach length
  const char PIN_COUNT = 80;        //pin count
  auto& sss = Serial;            //Hook serial
#endif

#ifdef ARDUINO_MEGA
  #define SERIAL_RX_BUFFER_SIZE 256
  #define SERIAL_TX_BUFFER_SIZE 256
  const char FIFO_SIZE = 50;        //cach length
  const char PIN_COUNT = 70;        //pin count
  auto& sss = Serial;               //Hook serial
#endif

//queue for capture data
QueueHandle_t fifoData;
QueueHandle_t fifoCach;
SemaphoreHandle_t xSemaphore;

//flow var is used by vWaitCmdTask only
char workMode = 0;                    //work mode, 0: idle;  1: start capture data;
int pinStates[PIN_COUNT];             //states of pin, -1: not initial, 100: other;
vector<TaskHandle_t*> captureTasks(); //for capture task
char comTask[4] = {0};                //for serial which need monitor
char canTask[2] = {0};                //for can whitch need monitor

static void vWaitCmdTask(void *pvParameters);
static void vReportTask(void *pvParameters);

static void CommandDispatcher(vector<sds>& args);
static void ReportBuffer(sds buf);

//for connect to pc
KrpRecver fromPc(CommandDispatcher);
KrpSender toPc(ReportBuffer);

// static void temp1( void *pvParameters ) {
//   for(;;) {
//     sds msg_buf;
//     xQueueReceive(fifoCach, &msg_buf, 0);
//     msg_buf = sdscat(msg_buf, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n");
//     xQueueSendToBack(fifoData, &msg_buf, 0);
//     xSemaphoreGive(xSemaphore);
//     vTaskDelay(500 / portTICK_PERIOD_MS);    
//   }

// }

// static void temp2( void *pvParameters ) {
//   for(;;) {
//     sds msg_buf;
//     xQueueReceive(fifoCach, &msg_buf, 0);
//     msg_buf = sdscat(msg_buf, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\n");
//     xQueueSendToBack(fifoData, &msg_buf, 0);
//     xSemaphoreGive(xSemaphore);
//     vTaskDelay(700 / portTICK_PERIOD_MS);
    
//   }

// }


void setup() {
  //initial serialusb allways
  sss.begin(9600);
  while(!sss) continue;
  for(int i=0; i<PIN_COUNT; i++) {
    pinStates[i]=-1;
  }

  //intial data queue and cach queue 
  fifoData = xQueueCreate(FIFO_SIZE, sizeof(sds*));
  fifoCach = xQueueCreate(FIFO_SIZE, sizeof(sds*));

  //create task 
  xTaskCreate(vWaitCmdTask, "waitcmd", configMINIMAL_STACK_SIZE+500, NULL, 1, NULL);
  xTaskCreate(vReportTask, "report", configMINIMAL_STACK_SIZE+500, NULL, 2, NULL);

  // xTaskCreate(temp1, "temp1", 300, NULL, 1, NULL);
  // xTaskCreate(temp2, "temp2", 300, NULL, 1, NULL);

  //for report event
  xSemaphore = xSemaphoreCreateCounting(FIFO_SIZE, 0);

  //initial cach
  for(int i=0; i<FIFO_SIZE; i++) {
    sds c=sdsempty();
    xQueueSendToBack(fifoCach, &c, 0);
  }

  vTaskStartScheduler();
  for(;;);
}


//task0: wait command from pc
static void vWaitCmdTask( void *pvParameters ) {

  TickType_t ticks = xTaskGetTickCount();
  sds buf=sdsempty();
  for(;;) {
    vTaskDelayUntil(&ticks, 1);
    int len = sss.available();
    if(len==0) {
      taskYIELD();
      continue;
    }
    buf = sdsMakeRoomFor(buf, len);
    sss.readBytes(buf, len);
    sdssetlen(buf, len);
    fromPc.Feed(buf);
    if(sdslen(buf)>1024) {
      sdsfree(buf);
      buf=sdsempty();
    } else {
      sdsclear(buf); 
    }
    taskYIELD();
  }
}


//task1: send fifoData up to pc
static void vReportTask(void *pvParameters) {
  
  for(;;) {
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    sds msg_buf;
    xQueueReceive(fifoData, &msg_buf, 0);
    int pos=0;
    int left_len=sdslen(msg_buf);
    while(left_len>0) {
      int space_len=sss.availableForWrite();
      while(space_len<=0) {
        space_len=sss.availableForWrite();
      }
      int sendlen = min(space_len, left_len);
      sss.write(msg_buf+pos, sendlen);
      pos+=sendlen;
      left_len-=sendlen;
    }
    if(sdslen(msg_buf)>1024) {
      sdsfree(msg_buf);
      msg_buf=sdsempty();
    } else {
      sdsclear(msg_buf);
    }
    xQueueSendToBack(fifoCach, &msg_buf, 0);
  }
}

//send data to fifoData
static void ReportBuffer(sds buf) {
  sds msg_buf;
  if(!xQueueReceive(fifoCach, &msg_buf, 0)) {
    sss.println("cach is empty");
  }
  msg_buf = sdscpylen(msg_buf, buf, sdslen(buf));
  xQueueSendToBack(fifoData, &msg_buf, 0);
  xSemaphoreGive(xSemaphore);
}

//execute command
void CommandDispatcher(vector<sds>& args) {

  int len = args.size();
  configASSERT(len>0);
  sds cmd=args[0];

  //init dio
  if(strcmp(cmd, "dio")==0) {
    configASSERT(workMode==0 && len==3);
    logout("dio command");
    
    int pinid=atoi(args[1]);
    configASSERT(pinid!=13 && pinid>0 && pinid<=54);
    auto mode=(strcmp(args[2], "out")==0) ? OUTPUT : (strcmp(args[2], "in_pullup")==0 ? INPUT_PULLUP : INPUT);
    
    configASSERT(pinStates[pinid]==-1);
    pinStates[pinid]=mode;

  //init serial
  } else if(strcmp(cmd, "serial")==0) {
    configASSERT(workMode==0 && len==3);
    logout("serial command");
    
    int id=atoi(args[1]);
    configASSERT(id>=0 && id<=4);
    auto cfg = args[2];
    switch(id) {
      case 0:
        configASSERT(pinStates[0]==-1 && pinStates[1]==-1);
        pinStates[0]=100;
        pinStates[1]=100;
        configASSERT(set_serial(Serial, cfg));
        break;
      case 1:
        configASSERT(pinStates[19]==-1 && pinStates[18]==-1);
        pinStates[19]=100;
        pinStates[18]=100;
        configASSERT(set_serial(Serial1, cfg));
        break;
      case 2:
        configASSERT(pinStates[17]==-1 && pinStates[16]==-1);
        pinStates[17]=100;
        pinStates[16]=100;
        configASSERT(set_serial(Serial2, cfg));
        break;
      case 3:
        configASSERT(pinStates[15]==-1 && pinStates[14]==-1);
        pinStates[15]=100;
        pinStates[14]=100;
        configASSERT(set_serial(Serial3, cfg));
        break;
    }
  }

  //init can
  else if(strcmp(cmd, "can")==0) {
    configASSERT(workMode==0 && len==3);
    logout("can command");
    int id=atoi(args[1]);
    configASSERT(id==0 || id==1);
    if(id==0) {
      configASSERT(pinStates[23]==-1 && pinStates[24]==-1);
      pinStates[23]=100;
      pinStates[24]=100;
    } else {
      configASSERT(pinStates[53]==-1 && pinStates[66]==-1);
      pinStates[53]=100;
      pinStates[66]=100;
    }
    configASSERT(set_can(id, args[2]));

  //start capture data
  } else if(strcmp(cmd, "start")==0) {
    configASSERT(workMode==0);
    logout("start command");
    
    //TODO start capture data
    workMode = 1;

  //stop capture data
  } else if(strcmp(cmd, "stop")==0) {
    configASSERT(workMode==1);
    logout("stop command");
    
    //TODO stop all capture task
    for(int i=0; i<PIN_COUNT; i++) {
      pinStates[i]=-1;
    }
    workMode = 0;

  //bad command
  } else {
    sds err=sdscatfmt(sdsempty(), "Bad command %S", args[0]);
    logout(err);
    sdsfree(err);
  } 
}



// keep empty
void loop() { }
