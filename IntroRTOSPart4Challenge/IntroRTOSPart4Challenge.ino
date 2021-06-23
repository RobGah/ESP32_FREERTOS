/**
 * Digikey Intro to FreeRTOS w/ Shawn Hymel
 * Challenge 4: Character Echo w/ Heap memory
 * Rob Garrone
 * 
 * NOTES:
 * Had to peek at answer - volatile on the flag REALLY matters. 
 * Learned a ton, largely turned into a copy of SH's solution as I debugged. 
 * Its the understanding that counts!
 */

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

//global variables
static volatile bool msgReady = 0; //flag for telling print task we have something to echo
static const int arrsize = 255;
static char *msgptr = NULL;

//Listen Task
void getIncomingSerial(void *parameter)
{
   char c;
   char msgbuf[arrsize];
   uint8_t idx = 0;

   memset(msgbuf, 0, arrsize);


   while(1)
   {
    if(Serial.available()>0)
    {
      c=Serial.read();
      
      if(idx < arrsize-1)
      {
          msgbuf[idx] = c; //place char in buffer
          idx++; //THEN increment
      }
          if (c=='\n')
          {
            msgbuf[idx-1] = '\0'; //null terminate to make it a string
            if(msgReady == 0)
            {
              msgptr = (char *)pvPortMalloc(idx*sizeof(char));
              configASSERT(msgptr);
              
              memcpy(msgptr,msgbuf,idx);
              
              msgReady = 1; //set flag to true
            }
            memset(msgbuf,0,arrsize);//wipe message
            idx=0; //set idx to 0
          }
     }
  }
}

void printMessage(void *parameter)
{
  while(1)
  {
    if(msgReady==1)
    {
      Serial.println(msgptr); //print message

            // Give amount of free heap memory (uncomment if you'd like to see it)
      //Serial.print("Free heap (bytes): ");
      //Serial.println(xPortGetFreeHeapSize());
      
      vPortFree(msgptr);
      msgptr = NULL;
      msgReady = 0; //reset printFlag
      }
  }
}

//setup() and loop() run as their own tasks w/ priority 1 in core 1. 

void setup() 
{
  //start serial
  Serial.begin(115200);
  //wait a second for config
  vTaskDelay(1000/portTICK_PERIOD_MS);
  //show time!
  Serial.println();
  Serial.println("---------------------------------");
  Serial.println("Program will echo what is given:");

  xTaskCreatePinnedToCore(    //use xTaskCreate() in vanilla FREERTOS
    getIncomingSerial,              //our task function
    "Get Incoming Serial",           // name of task
    1024,                   //Stack size (bytes in ESP32,words in FREERTOS)
    NULL,                   //Parameter to pass to function
    1,                      //priority (higher=greater priority, 0 to configMAX_PRIORITIES-1)
    NULL,                   //Task Handle
    app_cpu);               //Run on one core for demo purposes

    xTaskCreatePinnedToCore(    //use xTaskCreate() in vanilla FREERTOS
    printMessage,              //our task function
    "Print Message",           // name of task
    1024,                   //Stack size (bytes in ESP32,words in FREERTOS
    NULL,                   //Parameter to pass to function
    1,                      //priority (higher=greater priority, 0 to configMAX_PRIORITIES-1)
    NULL,                   //Task Handle
    app_cpu);               //Run on one core for demo purposes

    vTaskDelete(NULL); //get rid of "setup and loop" task
}

//NOTE - seems like priotity 1 is the highest priority? I can't get priority 0 to work. 

//IF this were vanilla FREERTOS, you'd need to call the scheduler via vTaskStartScheduler() in main after setting up tasks.
//Its already called for us via arduino. 

void loop() {
  // put your main code here, to run repeatedly:

}
