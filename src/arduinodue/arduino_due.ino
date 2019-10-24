/*
  data collection for auduino due 
*/

#include <FreeRTOS_ARM.h>
#include <KrpSender.h>
#include <KrpRecver.h>
#include <vector>

using namespace std;

//cach length
const char FIFO_SIZE = 10;
const char PIN_COUNT = 80;

//queue for capture data
QueueHandle_t fifoData;
QueueHandle_t fifoCach;

//flow var is used by vWaitCmdTask only
char workMode = 0;                    //work mode, 0: idle;  1: start capture data;
int pinStates[PIN_COUNT];             //states of pin, -1: not initial, 100: other;
vector<TaskHandle_t*> captureTasks(); //for capture task
char comTask[4] = {0};                //for serial which need monitor
char canTask[2] = {0};                //for can whitch need monitor

static void vWaitCmdTask( void *pvParameters );
static void vReportTask( void *pvParameters );

static void CommandDispatcher(vector<sds>& args);
static void ReportBuffer(sds buf);

//for connect to pc
KrpRecver fromPc(CommandDispatcher);
KrpSender toPc(ReportBuffer);

void setup() {
  //initial serialusb allways
  Serial.begin(9600);
  while(!Serial) {}
  for(int i=0; i<PIN_COUNT; i++) {
    pinStates[i]=-1;
  }

  //intial data queue and cach queue 
  fifoData = xQueueCreate(FIFO_SIZE, sizeof(sds));
  fifoCach = xQueueCreate(FIFO_SIZE, sizeof(sds));
  configASSERT(fifoData && fifoCach);

  //create task 
  BaseType_t res1 = xTaskCreate( vWaitCmdTask, "waitcmd", 256, NULL, 2, NULL );
  BaseType_t res2 = xTaskCreate( vReportTask, "report", 32, NULL, 2, NULL );
  configASSERT(res1==pdPASS && res2==pdPASS);

  //initial cach
  for(int i=0; i<FIFO_SIZE; i++) {
    res1 = xQueueSendToBack(fifoCach, sdsnew(NULL), 0);
    configASSERT(res1==pdPASS);
  }
  //Serial.println("111");
  vTaskStartScheduler();
  //Serial.println("222");
  for(;;);
}


//send buf to pc
void ReportBuffer(sds buf) {
  Serial.write(buf, sdslen(buf));
}


//execute command
void CommandDispatcher(vector<sds>& args) {

  int len = args.size();
  configASSERT(len>0);
  sds cmd=args[0];

  //init_dio
  if(strcmp(cmd, "init_dio")==0) {
    configASSERT(workMode==0 && len==3);
    int pinid=atoi(args[1]);
    if(pinid==13 || pinid<0 || pinid>54) {
      toPc.SendError("Pin id error");
      return;
    }
    auto mode=(strcmp(args[2], "out")==0) ? OUTPUT : (strcmp(args[3], "in_pullup")==0 ? INPUT_PULLUP : INPUT);
    if(pinStates[pinid]!=-1) {
      toPc.SendError("Pin conflict");
      return;
    } else {
      pinMode(pinid, mode);
      pinStates[pinid]=mode;
    }

  //init_com
  } else if(strcmp(cmd, "init_com")==0) {
    configASSERT(workMode==0 && len==3);
  }

  //init_can
  else if(strcmp(cmd, "init_can")==0) {
    configASSERT(workMode==0 && len==3);
  } else {
    sds err=sdsempty();
    err=sdscatfmt(err, "Bad command %S", args[0]);
    toPc.SendError(err);
    sdsfree(err);
  }
  
}


//wait command from pc
static void vWaitCmdTask( void *pvParameters ) {
  sds buf = sdsempty();
  for(;;) {
    int len = Serial.available();
    if(len>0) {
      buf = sdsMakeRoomFor(buf, len);
      Serial.readBytes(buf, len);
      sdssetlen(buf, len);
      fromPc.Feed(buf);
      if(sdslen(buf)>600) {
        sdsfree(buf);
        buf=sdsempty();
      } else {
        sdsclear(buf); 
      }
    }
    
    taskYIELD();
  }
}


//report up to pc
static void vReportTask(void *pvParameters) {
  for(;;) {

    
    taskYIELD();
  }

}

// keep empty
void loop() { }
