/*
  data collection for auduino due 
*/

#include <FreeRTOS_ARM.h>
#include <KrpSender.h>
#include <KrpRecver.h>
#include <vector>

using namespace std;

//for hardware
#define ARDUINO_DUE

#ifdef ARDUINO_DUE
#define SERIAL_RX_BUFFER_SIZE 512
#define SERIAL_TX_BUFFER_SIZE 512
const char FIFO_SIZE = 200;       //cach length
const char PIN_COUNT = 80;        //pin count
auto& sss = SerialUSB;            //Hook serial
#endif

#ifdef ARDUINO_MEGA
#define SERIAL_RX_BUFFER_SIZE 256
#define SERIAL_TX_BUFFER_SIZE 256
const char FIFO_SIZE = 50;        //cach length
const char PIN_COUNT = 60;        //pin count
auto& sss = Serial;               //Hook serial
#endif

//queue for capture data
QueueHandle_t fifoData;
QueueHandle_t fifoCach;

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

void setup() {
  //initial serialusb allways
  sss.begin(9600);
  while(!sss) {}
  for(int i=0; i<PIN_COUNT; i++) {
    pinStates[i]=-1;
  }

  //intial data queue and cach queue 
  fifoData = xQueueCreate(FIFO_SIZE, sizeof(sds*));
  fifoCach = xQueueCreate(FIFO_SIZE, sizeof(sds*));

  //create task 
  BaseType_t res1 = xTaskCreate(vWaitCmdTask, "waitcmd", 512, NULL, 2, NULL);
  BaseType_t res2 = xTaskCreate(vReportTask, "report", 512, NULL, 2, NULL);

  vTaskStartScheduler();
  for(;;);
}


//task0: wait command from pc
static void vWaitCmdTask( void *pvParameters ) {
  //initial cach
  for(int i=0; i<FIFO_SIZE; i++) {
    sds c=sdsempty();
    xQueueSendToBack(fifoCach, &c, 0);
  }
  
  sds buf=sdsempty();
  for(;;) {
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
    sds msg_buf;
    if(xQueueReceive(fifoData, &msg_buf, 0)) {
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
    taskYIELD();
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
    sds err=sdscatfmt(sdsempty(), "Bad command %S", args[0]);
    toPc.SendError(err);
    sdsfree(err);
  } 
}



// keep empty
void loop() { }
